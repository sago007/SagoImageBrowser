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
        QImage thumb = image.scaled(128, 128,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
        emit finished(m_row, thumb);
    }
}
