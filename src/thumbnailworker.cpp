/*
MIT License

Copyright (c) 2026 Poul Sander

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "thumbnailworker.h"

#include "thumbnailcache.h"
#include "nativepath.h"

#include <QFile>
#include <QImageReader>

namespace
{
constexpr int kDisplaySize = 128;

// THREADING: Pure function called from a worker thread.  No shared mutable state.
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

ThumbnailWorker::ThumbnailWorker(const QByteArray &path,
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

// THREADING: This function runs on a QThreadPool worker thread, NOT the main
// thread.  It must not access any ImageModel state.  The only shared data it
// touches is the atomic m_cancelFlag (for cooperative cancellation).  All other
// members (m_path, m_row, m_size) are immutable after construction.  Results
// are delivered to the main thread via the finished() signal connected with
// Qt::QueuedConnection.
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

    // Open via the native-path helper so that names the OS's Unicode encoding
    // cannot represent (non-UTF-8 bytes on POSIX, unpaired surrogates on
    // Windows) are handled correctly.
    QFile file;
    if (!nativepath::openNativeRead(m_path, file))
        return;

    QImageReader reader(&file);
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
