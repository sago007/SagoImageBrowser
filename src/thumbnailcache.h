#pragma once

#include <QByteArray>
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
    // at the requested size. sourcePath must be the raw native OS byte string.
    static QImage load(const QByteArray &sourcePath, Size size);

    // Atomically writes thumbImage to the cache with the required Freedesktop
    // tEXt chunks. sourcePath must be the raw native OS byte string.
    static void save(const QByteArray &sourcePath, Size size, const QImage &thumbImage);

    static QString cacheRoot();
    static QString subdirFor(Size size);
    // Canonical file:// URI built from raw byte path (correct percent-encoding)
    static QByteArray canonicalUri(const QByteArray &absPath);
    static QString hashedName(const QByteArray &canonicalUri);
    static bool isInsideCache(const QByteArray &absPath);
};
