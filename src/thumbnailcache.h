#pragma once

#include <QString>
#include <QImage>

class ThumbnailCache
{
public:
    enum class Size
    {
        Normal = 128,
        Large = 256,
    };

    // Returns a non-null QImage if a valid cached thumbnail exists for sourcePath
    // at the requested size. Validity is determined by comparing the original
    // file's mtime to the Thumb::MTime tEXt chunk in the cached PNG.
    static QImage load(const QString &sourcePath, Size size);

    // Atomically writes thumbImage to the cache with the required Freedesktop
    // tEXt chunks. No-op if sourcePath cannot be stat'ed.
    static void save(const QString &sourcePath, Size size, const QImage &thumbImage);

    static QString cacheRoot();
    static QString subdirFor(Size size);
    static QString canonicalUri(const QString &absPath);
    static QString hashedName(const QString &canonicalUri);
    static bool isInsideCache(const QString &absPath);
};
