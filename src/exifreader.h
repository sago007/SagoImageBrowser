#pragma once

#include <QString>
#include <QList>
#include <QPair>

struct ExifData {
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
ExifData read(const QString &path);
} // namespace ExifReader
