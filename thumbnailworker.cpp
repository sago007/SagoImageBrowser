#include "thumbnailworker.h"
#include <QImageReader>

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

    QImage image = reader.read();

    if (*m_cancelFlag)
        return;

    if (!image.isNull())
    {
        // Scale to cover the 128x128 area, then center-crop
        QImage scaled = image.scaled(128, 128,
                                     Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation);
        int x = (scaled.width()  - 128) / 2;
        int y = (scaled.height() - 128) / 2;
        QImage thumb = scaled.copy(x, y, 128, 128);
        emit finished(m_row, thumb);
    }
}
