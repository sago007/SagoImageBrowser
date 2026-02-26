#include "thumbnailworker.h"
#include <QImageReader>
#include <QPainter>

ThumbnailWorker::ThumbnailWorker(const QString &path,
                                 int row,
                                 std::atomic_bool *cancelFlag)
    : m_path(path),
      m_row(row),
      m_cancelFlag(cancelFlag)
{
    setAutoDelete(true);
}

void ThumbnailWorker::run()
{
    if (*m_cancelFlag)
        return;

    QImageReader reader(m_path);
    reader.setAutoTransform(true);

    // Scale during decoding to avoid hitting the allocation limit
    // on very large images (Qt 6 defaults to 128 MB).
    QSize fullSize = reader.size();
    if (fullSize.isValid()) {
        QSize target = fullSize.scaled(128, 128, Qt::KeepAspectRatio);
        reader.setScaledSize(target);
    }

    QImage image = reader.read();

    if (*m_cancelFlag)
        return;

    if (!image.isNull())
    {
        // Scale to fit within 128x128 keeping aspect ratio
        // (may already be the right size if setScaledSize succeeded)
        QImage scaled = image.scaled(128, 128,
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
        // Center on a 128x128 canvas with transparent/white fill
        QImage thumb(128, 128, QImage::Format_ARGB32_Premultiplied);
        thumb.fill(Qt::transparent);
        int x = (128 - scaled.width())  / 2;
        int y = (128 - scaled.height()) / 2;
        QPainter painter(&thumb);
        painter.drawImage(x, y, scaled);
        painter.end();
        emit finished(m_row, thumb);
    }
}
