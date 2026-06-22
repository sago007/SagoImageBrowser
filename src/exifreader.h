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
#include <QList>
#include <QPair>

struct ExifData {
    QString captionAbstract;  // IPTC Caption-Abstract (Iptc.Application2.Caption)
    QString description;      // Exif.Image.ImageDescription
    QString dateTime;
    QString make;
    QString model;
    QString exposureTime;
    QString fNumber;
    QString iso;
    QString focalLength;
    QString flash;
    QString filename;
    QString dimensions;
    QString fileSize;
    QString orientation;
    QString latitude;
    QString longitude;
    QString osmLink;

    bool isEmpty() const;
    QList<QPair<QString, QString>> toList() const;
};

namespace ExifReader {
ExifData read(const QByteArray &path);

// Write caption to IPTC Caption-Abstract.
// If oldData.captionAbstract == oldData.description they were in sync;
// in that case also update Exif.Image.ImageDescription to match.
// Returns true on success.
bool saveCaption(const QByteArray &path,
                 const QString    &caption,
                 const ExifData   &oldData);

// Rotate the image 90 degrees by updating the Exif.Image.Orientation tag only
// (lossless — pixel data is left untouched). clockwise=true rotates right.
// Returns true on success.
bool rotate(const QByteArray &path, bool clockwise);
} // namespace ExifReader
