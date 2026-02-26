#include "imageviewwidget.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QImageReader>
#include <QPalette>
#include <algorithm>

ImageViewWidget::ImageViewWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(true);
    setBackgroundColor("black");
}

void ImageViewWidget::setImage(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);
    // Disable the allocation limit so very large images can be viewed.
    // The default 128 MB limit rejects high-resolution photos.
    reader.setAllocationLimit(0);
    QImage image = reader.read();

    if (!image.isNull()) {
        // Ensure the image is in a high-quality format
        if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32 && image.format() != QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat(QImage::Format_ARGB32);
        m_pixmap = QPixmap::fromImage(image);
    }
    else
        m_pixmap = QPixmap();

    m_zoomMode = FitToScreen;
    m_zoomFactor = 1.0;
    m_offset = QPoint(0, 0);
    update();
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
}

void ImageViewWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit closeRequested();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_PageDown)
    {
        emit nextRequested();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_PageUp)
    {
        emit previousRequested();
        event->accept();
        return;
    }

    // Arrow keys for panning
    constexpr int panStep = 50;
    if (event->key() == Qt::Key_Left)
    {
        m_offset.rx() += panStep;
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right)
    {
        m_offset.rx() -= panStep;
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Up)
    {
        m_offset.ry() += panStep;
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Down)
    {
        m_offset.ry() -= panStep;
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Slash)
    {
        zoomOriginal();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Asterisk)
    {
        zoomFitToScreen();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Plus)
    {
        zoomIn();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Minus)
    {
        zoomOut();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
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
