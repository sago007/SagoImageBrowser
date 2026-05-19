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
    QString dimensions;
    QString fileSize;

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
} // namespace ExifReader
