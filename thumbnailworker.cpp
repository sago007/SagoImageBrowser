#include "thumbnailworker.h"
#include <QImageReader>

ThumbnailWorker::ThumbnailWorker(const QString &path, int row)
    : m_path(path), m_row(row)
{
    setAutoDelete(true);
}

void ThumbnailWorker::run()
{
    QImageReader reader(m_path);
    reader.setAutoTransform(true);

    QImage image = reader.read();
    QPixmap thumb;

    if (!image.isNull())
    {
        thumb = QPixmap::fromImage(
            image.scaled(128, 128,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation));
    }

    emit finished(m_row, thumb);
}
