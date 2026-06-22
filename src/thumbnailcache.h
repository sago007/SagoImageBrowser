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
