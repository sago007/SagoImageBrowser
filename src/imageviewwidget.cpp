#include "imageviewwidget.h"
#include "shortcutmanager.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QImageReader>
#include <QPalette>
#include <algorithm>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

// --- ImageLoadWorker ---

ImageLoadWorker::ImageLoadWorker(const QByteArray &path, QObject *parent)
    : QThread(parent), m_path(path)
{
}

// THREADING: This function runs on its own QThread, NOT the main thread.
// It must not access any ImageViewWidget member state.  The only data it uses
// is m_path which is immutable after construction.  The emitted imageLoaded()
// signal is delivered to the main thread via Qt::AutoConnection which resolves
// to QueuedConnection at emit time (worker thread != receiver's main-thread
// affinity), so onImageLoaded() always executes on the main thread.
void ImageLoadWorker::run()
{
    int fd = ::open(m_path.constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        emit imageLoaded(m_path, QPixmap{});
        return;
    }

    QFile file;
    if (!file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        ::close(fd);
        emit imageLoaded(m_path, QPixmap{});
        return;
    }

    QImageReader reader(&file);
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);
    reader.setAllocationLimit(0);
    QImage image = reader.read();

    QPixmap pixmap;
    if (!image.isNull()) {
        if (image.format() != QImage::Format_RGB32
            && image.format() != QImage::Format_ARGB32
            && image.format() != QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat(QImage::Format_ARGB32);
        pixmap = QPixmap::fromImage(image);
    }
    emit imageLoaded(m_path, pixmap);
}

// --- ImageViewWidget ---

ImageViewWidget::ImageViewWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(true);
    setBackgroundColor("black");
    setMouseTracking(true);
}

QPixmap ImageViewWidget::loadImageFromDisk(const QByteArray &path)
{
    int fd = ::open(path.constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return {};

    QFile file;
    if (!file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        ::close(fd);
        return {};
    }

    QImageReader reader(&file);
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);
    reader.setAllocationLimit(0);
    QImage image = reader.read();

    if (!image.isNull()) {
        if (image.format() != QImage::Format_RGB32
            && image.format() != QImage::Format_ARGB32
            && image.format() != QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat(QImage::Format_ARGB32);
        return QPixmap::fromImage(image);
    }
    return {};
}

void ImageViewWidget::setImage(const QByteArray &path)
{
    m_currentPath = path;
    m_exifData = ExifData{}; // clear stale EXIF until new data is set

    // Check cache first
    if (m_cache.contains(path)) {
        m_pixmap = m_cache[path];
    } else {
        m_pixmap = loadImageFromDisk(path);
        m_cache[path] = m_pixmap;
    }

    m_zoomMode = FitToScreen;
    m_zoomFactor = 1.0;
    m_offset = QPoint(0, 0);
    update();

    // Trigger prefetch of neighbors if they have been set
    if (!m_neighborPaths.isEmpty()) {
        // Evict entries that are no longer in the neighbor window
        QSet<QByteArray> keep(m_neighborPaths.begin(), m_neighborPaths.end());
        keep.insert(m_currentPath);
        for (auto it = m_cache.begin(); it != m_cache.end(); ) {
            if (!keep.contains(it.key()))
                it = m_cache.erase(it);
            else
                ++it;
        }

        // Prefetch neighbors that are not yet cached
        for (const QByteArray &neighbor : m_neighborPaths) {
            prefetchImage(neighbor);
        }
    }
}

void ImageViewWidget::prefetchImage(const QByteArray &path)
{
    if (m_cache.contains(path) || m_pendingLoads.contains(path))
        return;

    m_pendingLoads.insert(path);
    auto *worker = new ImageLoadWorker(path, this);
    // THREADING: Qt::AutoConnection (default) resolves to QueuedConnection at
    // emit time because ImageLoadWorker::run() emits from its worker thread
    // while 'this' (ImageViewWidget) has main-thread affinity.  This guarantees
    // onImageLoaded() always runs on the main thread — no mutex needed.
    connect(worker, &ImageLoadWorker::imageLoaded, this, &ImageViewWidget::onImageLoaded);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void ImageViewWidget::onImageLoaded(const QByteArray &path, const QPixmap &pixmap)
{
    m_pendingLoads.remove(path);

    // Only cache if path is still relevant (current or a neighbor)
    QSet<QByteArray> keep(m_neighborPaths.begin(), m_neighborPaths.end());
    keep.insert(m_currentPath);
    if (keep.contains(path)) {
        m_cache[path] = pixmap;
    }
}

void ImageViewWidget::setNeighborPaths(const QList<QByteArray> &paths)
{
    m_neighborPaths = paths;
}

void ImageViewWidget::clearCache()
{
    m_cache.clear();
    m_neighborPaths.clear();
}

void ImageViewWidget::invalidateCache(const QByteArray &path)
{
    m_cache.remove(path);
}

void ImageViewWidget::setBackgroundColor(const QString &color)
{
    m_backgroundColor = color;
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(color));
    setPalette(pal);
    update();
}

void ImageViewWidget::paintEvent(QPaintEvent *)
{
    if (m_pixmap.isNull())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPixmap drawn;

    switch (m_zoomMode) {
    case FitToScreen:
        drawn = m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        break;
    case OriginalSize:
        drawn = m_pixmap;
        break;
    case CustomZoom: {
        QSize target(static_cast<int>(m_pixmap.width() * m_zoomFactor),
                     static_cast<int>(m_pixmap.height() * m_zoomFactor));
        drawn = m_pixmap.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        break;
    }
    }

    clampOffset(drawn.width(), drawn.height());

    int x = (width()  - drawn.width())  / 2 + m_offset.x();
    int y = (height() - drawn.height()) / 2 + m_offset.y();
    painter.drawPixmap(x, y, drawn);

    // EXIF overlay — top-right corner, toggled with 'i'
    if (m_showExifOverlay) {
        const auto fields = m_exifData.toList();
        if (!fields.isEmpty()) {
            const int padding = 10;
            const int lineH = 18;

            QFont font = painter.font();
            font.setPointSize(9);
            painter.setFont(font);
            QFontMetrics fm(font);

            int maxW = 0;
            for (const auto &f : fields)
                maxW = qMax(maxW, fm.horizontalAdvance(f.first + QLatin1String(": ") + f.second));

            const int boxW = maxW + padding * 2;
            const int boxH = fields.size() * lineH + padding * 2;
            const int boxX = width() - boxW - padding;
            const int boxY = padding;

            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 160));
            painter.drawRoundedRect(boxX, boxY, boxW, boxH, 8, 8);

            painter.setPen(Qt::white);
            m_osmLinkRect = QRect();
            for (int i = 0; i < fields.size(); ++i) {
                const QString line = fields[i].first + QLatin1String(": ") + fields[i].second;
                if (fields[i].first == "OpenStreetMap") {
                    painter.setPen(Qt::cyan);
                    m_osmLinkRect = QRect(boxX + padding,
                                          boxY + padding + i * lineH,
                                          fm.horizontalAdvance(line),
                                          lineH);
                } else {
                    painter.setPen(Qt::white);
                }
                painter.drawText(boxX + padding,
                                 boxY + padding + fm.ascent() + i * lineH,
                                 line);
            }
            painter.restore();
        }
    }

    // Caption-Abstract overlay — bottom-centre, always shown when non-empty
    if (!m_exifData.captionAbstract.isEmpty()) {
        const int padding  = 10;
        const int margin   = 20;  // distance from bottom edge
        const int maxWidth = width() * 3 / 4;

        QFont captionFont = painter.font();
        captionFont.setPointSize(11);
        painter.setFont(captionFont);
        QFontMetrics cfm(captionFont);

        QRect textBounds = cfm.boundingRect(
            QRect(0, 0, maxWidth, 0),
            Qt::AlignHCenter | Qt::TextWordWrap,
            m_exifData.captionAbstract);

        const int boxW = textBounds.width()  + padding * 2;
        const int boxH = textBounds.height() + padding * 2;
        const int boxX = (width() - boxW) / 2;
        const int boxY = height() - boxH - margin;

        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 180));
        painter.drawRoundedRect(boxX, boxY, boxW, boxH, 8, 8);

        painter.setPen(Qt::white);
        painter.drawText(QRect(boxX + padding, boxY + padding, textBounds.width(), textBounds.height()),
                         Qt::AlignHCenter | Qt::TextWordWrap,
                         m_exifData.captionAbstract);
        painter.restore();
    }

    // Help overlay — centered, toggled with F1
    if (m_showHelpOverlay) {
        const ShortcutManager &sc = ShortcutManager::instance();

        // Build the shortcut table dynamically from current bindings.
        // Some actions are grouped together for display (e.g. pan directions).
        struct HelpEntry { QString key; QString desc; };
        QList<HelpEntry> entries;

        entries.append({sc.displayKey(ShortcutManager::ToggleHelp),
                        sc.description(ShortcutManager::ToggleHelp)});
        entries.append({sc.displayKey(ShortcutManager::CloseViewer)
                        + " / " + sc.displayKey(ShortcutManager::CloseViewerAlt),
                        tr("Close viewer")});
        entries.append({sc.displayKey(ShortcutManager::NextImage)
                        + " / " + sc.displayKey(ShortcutManager::PrevImage),
                        tr("Next / previous image")});
        entries.append({sc.displayKey(ShortcutManager::PanLeft)
                        + " " + sc.displayKey(ShortcutManager::PanRight)
                        + " " + sc.displayKey(ShortcutManager::PanUp)
                        + " " + sc.displayKey(ShortcutManager::PanDown),
                        tr("Pan image")});
        entries.append({sc.displayKey(ShortcutManager::ZoomOriginal),
                        sc.description(ShortcutManager::ZoomOriginal)});
        entries.append({sc.displayKey(ShortcutManager::ZoomFit),
                        sc.description(ShortcutManager::ZoomFit)});
        entries.append({sc.displayKey(ShortcutManager::ZoomIn)
                        + " / " + sc.displayKey(ShortcutManager::ZoomOut),
                        tr("Zoom in / out")});
        entries.append({sc.displayKey(ShortcutManager::ToggleExif),
                        sc.description(ShortcutManager::ToggleExif)});
        entries.append({sc.displayKey(ShortcutManager::EditCaption),
                        sc.description(ShortcutManager::EditCaption)});
        entries.append({sc.displayKey(ShortcutManager::ToggleFullscreen),
                        sc.description(ShortcutManager::ToggleFullscreen)});

        const int count = entries.size();

        const int padding = 16;
        const int lineH   = 22;
        const int colGap  = 20;

        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);
        QFontMetrics fm(font);

        int keyColW  = 0;
        int descColW = 0;
        for (const auto &e : entries) {
            keyColW  = qMax(keyColW,  fm.horizontalAdvance(e.key));
            descColW = qMax(descColW, fm.horizontalAdvance(e.desc));
        }

        const int boxW = padding + keyColW + colGap + descColW + padding;
        const int boxH = count * lineH + padding * 2;
        const int boxX = (width()  - boxW) / 2;
        const int boxY = (height() - boxH) / 2;

        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 200));
        painter.drawRoundedRect(boxX, boxY, boxW, boxH, 10, 10);

        for (int i = 0; i < count; ++i) {
            const int textY = boxY + padding + fm.ascent() + i * lineH;
            // Key column — right-aligned, light lavender
            painter.setPen(QColor(180, 180, 255));
            painter.drawText(QRect(boxX + padding, textY - fm.ascent(), keyColW, lineH),
                             Qt::AlignRight | Qt::AlignVCenter,
                             entries[i].key);
            // Description column — left-aligned, white
            painter.setPen(Qt::white);
            painter.drawText(QRect(boxX + padding + keyColW + colGap,
                                   textY - fm.ascent(), descColW, lineH),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             entries[i].desc);
        }
        painter.restore();
    }
}

void ImageViewWidget::keyPressEvent(QKeyEvent *event)
{
    const ShortcutManager &sc = ShortcutManager::instance();

    if (sc.matches(event, ShortcutManager::ToggleHelp))
    {
        m_showHelpOverlay = !m_showHelpOverlay;
        update();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::CloseViewer)
        || sc.matches(event, ShortcutManager::CloseViewerAlt))
    {
        emit closeRequested();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::NextImage))
    {
        emit nextRequested();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::PrevImage))
    {
        emit previousRequested();
        event->accept();
        return;
    }

    // Arrow keys for panning
    constexpr int panStep = 50;
    if (sc.matches(event, ShortcutManager::PanLeft))
    {
        m_offset.rx() += panStep;
        update();
        event->accept();
        return;
    }
    if (sc.matches(event, ShortcutManager::PanRight))
    {
        m_offset.rx() -= panStep;
        update();
        event->accept();
        return;
    }
    if (sc.matches(event, ShortcutManager::PanUp))
    {
        m_offset.ry() += panStep;
        update();
        event->accept();
        return;
    }
    if (sc.matches(event, ShortcutManager::PanDown))
    {
        m_offset.ry() -= panStep;
        update();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::ZoomOriginal))
    {
        zoomOriginal();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::ZoomFit))
    {
        zoomFitToScreen();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::ZoomIn))
    {
        zoomIn();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::ZoomOut))
    {
        zoomOut();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::ToggleExif))
    {
        toggleExifOverlay();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::EditCaption))
    {
        emit editCaptionRequested();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void ImageViewWidget::setExifData(const ExifData &data)
{
    m_exifData = data;
    if (m_showExifOverlay)
        update();
}

void ImageViewWidget::toggleExifOverlay()
{
    m_showExifOverlay = !m_showExifOverlay;
    update();
}

QByteArray ImageViewWidget::currentPath() const
{
    return m_currentPath;
}

void ImageViewWidget::zoomIn()
{
    if (m_zoomMode == FitToScreen) {
        // Calculate the current effective scale so zooming feels continuous
        double sx = static_cast<double>(width()) / m_pixmap.width();
        double sy = static_cast<double>(height()) / m_pixmap.height();
        m_zoomFactor = std::min(sx, sy);
    }
    m_zoomFactor *= 1.25;
    m_zoomMode = CustomZoom;
    update();
}

void ImageViewWidget::zoomOut()
{
    if (m_zoomMode == FitToScreen) {
        double sx = static_cast<double>(width()) / m_pixmap.width();
        double sy = static_cast<double>(height()) / m_pixmap.height();
        m_zoomFactor = std::min(sx, sy);
    }
    m_zoomFactor /= 1.25;
    if (m_zoomFactor < 0.01)
        m_zoomFactor = 0.01;
    m_zoomMode = CustomZoom;
    update();
}

void ImageViewWidget::zoomOriginal()
{
    m_zoomMode = OriginalSize;
    m_zoomFactor = 1.0;
    update();
}

void ImageViewWidget::zoomFitToScreen()
{
    m_zoomMode = FitToScreen;
    m_offset = QPoint(0, 0);
    update();
}

void ImageViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_showExifOverlay && m_osmLinkRect.contains(event->pos())) {
            QDesktopServices::openUrl(QUrl(m_exifData.osmLink));
            event->accept();
            return;
        }
        m_dragging = true;
        m_dragStart = event->pos();
        m_offsetAtDragStart = m_offset;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
}

void ImageViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        m_offset = m_offsetAtDragStart + (event->pos() - m_dragStart);
        update();
        event->accept();
    } else {
        if (m_showExifOverlay && m_osmLinkRect.contains(event->pos())) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void ImageViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
}

void ImageViewWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit closeRequested();
        event->accept();
    }
}

void ImageViewWidget::clampOffset(int imgW, int imgH)
{
    // Only allow panning when the drawn image exceeds the widget
    int overflowX = imgW - width();
    int overflowY = imgH - height();

    if (overflowX <= 0)
        m_offset.rx() = 0;
    else {
        int limit = overflowX / 2;
        m_offset.rx() = std::clamp(m_offset.x(), -limit, limit);
    }

    if (overflowY <= 0)
        m_offset.ry() = 0;
    else {
        int limit = overflowY / 2;
        m_offset.ry() = std::clamp(m_offset.y(), -limit, limit);
    }
}
