#include "thumbnailworker.h"

#include "thumbnailcache.h"

#include <QImageReader>

namespace
{
constexpr int kDisplaySize = 128;

QImage downscaleForDisplay(const QImage &src)
{
    if (src.isNull())
        return src;
    if (src.width() <= kDisplaySize && src.height() <= kDisplaySize)
        return src;
    return src.scaled(kDisplaySize, kDisplaySize,
                      Qt::KeepAspectRatio,
                      Qt::SmoothTransformation);
}
} // namespace

ThumbnailWorker::ThumbnailWorker(const QString &path,
                                 int row,
                                 ThumbnailCache::Size size,
                                 std::atomic_bool *cancelFlag)
    : m_path(path),
      m_row(row),
      m_size(size),
      m_cancelFlag(cancelFlag)
{
    setAutoDelete(true);
}

void ThumbnailWorker::run()
{
    if (*m_cancelFlag)
        return;

    // Per the spec, never thumbnail files that live inside the cache itself.
    if (ThumbnailCache::isInsideCache(m_path))
        return;

    const int target = static_cast<int>(m_size);

    QImage cached = ThumbnailCache::load(m_path, m_size);
    if (*m_cancelFlag)
        return;

    if (!cached.isNull())
    {
        emit finished(m_row, downscaleForDisplay(cached));
        return;
    }

    QImageReader reader(m_path);
    reader.setAutoTransform(true);

    // Scale during decoding to avoid hitting the allocation limit
    // on very large images (Qt 6 defaults to 128 MB).
    QSize fullSize = reader.size();
    if (fullSize.isValid())
    {
        QSize scaled = fullSize.scaled(target, target, Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    QImage image = reader.read();

    if (*m_cancelFlag)
        return;

    if (image.isNull())
        return;

    // Ensure the longest side is exactly the target (the decoder hint may have
    // been ignored for some formats). Aspect ratio preserved, no padding canvas.
    QImage thumb;
    if (image.width() > target || image.height() > target)
        thumb = image.scaled(target, target,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    else
        thumb = image;

    ThumbnailCache::save(m_path, m_size, thumb);

    if (*m_cancelFlag)
        return;

    emit finished(m_row, downscaleForDisplay(thumb));
}
