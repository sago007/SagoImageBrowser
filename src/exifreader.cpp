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


#include "exifreader.h"
#include "nativepath.h"

#include <QFile>
#include <QImageReader>
#include <QCoreApplication>

#include <exiv2/exiv2.hpp>

// exiv2 0.28 returns a std::unique_ptr (Image::UniquePtr); 0.27 uses Image::AutoPtr
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
using Exiv2ImagePtr = Exiv2::Image::UniquePtr;
#else
using Exiv2ImagePtr = Exiv2::Image::AutoPtr;
#endif

// exiv2 cannot represent an arbitrary Windows name through a narrow std::string;
// on Windows use its wide-path overload fed from our lossless WTF-8 handle.
static Exiv2ImagePtr openExiv2(const QByteArray &path)
{
#ifdef _WIN32
    return Exiv2::ImageFactory::open(nativepath::wideFromNative(path));
#else
    return Exiv2::ImageFactory::open(path.toStdString());
#endif
}

// Raw EXIF date format is "YYYY:MM:DD HH:MM:SS"; reformat date separators to dashes
static QString formatDate(const QString &raw)
{
    if (raw.size() >= 10 && raw[4] == u':' && raw[7] == u':')
        return raw.left(4) + u'-' + raw.mid(5, 2) + u'-' + raw.mid(8, 2) + raw.mid(10);
    return raw;
}

static QString findTag(const Exiv2::ExifData &exif, const std::string &key)
{
    try {
        Exiv2::ExifData::const_iterator it = exif.findKey(Exiv2::ExifKey(key));
        if (it != exif.end())
            return QString::fromStdString(it->print(&exif)).trimmed();
    } catch (...) {}
    return {};
}

static void addField(QList<QPair<QString, QString>> &result, const char *label, const QString &value)
{
    if (!value.isEmpty())
        result.append({QCoreApplication::translate("ExifReader", label), value});
}

static double parseGpsCoordinate(const Exiv2::ExifData &exif, const std::string &key)
{
    try {
        Exiv2::ExifData::const_iterator it = exif.findKey(Exiv2::ExifKey(key));
        if (it != exif.end() && it->count() >= 3) {
            double d = it->toRational(0).first / static_cast<double>(it->toRational(0).second);
            double m = it->toRational(1).first / static_cast<double>(it->toRational(1).second);
            double s = it->toRational(2).first / static_cast<double>(it->toRational(2).second);
            return d + m / 60.0 + s / 3600.0;
        }
    } catch (...) {}
    return 0.0;
}

// EXIF orientation values run 1-8 and may encode mirroring. A 90-degree
// rotation maps each value to another; the tables below cover all eight so
// mirrored images stay correct. Unknown values are treated as 1 (normal).
static int orientationRotatedClockwise(int orientation)
{
    switch (orientation) {
    case 1: return 6;
    case 2: return 7;
    case 3: return 8;
    case 4: return 5;
    case 5: return 2;
    case 6: return 3;
    case 7: return 4;
    case 8: return 1;
    default: return 6;
    }
}

static int orientationRotatedCounterClockwise(int orientation)
{
    switch (orientation) {
    case 1: return 8;
    case 2: return 5;
    case 3: return 6;
    case 4: return 7;
    case 5: return 4;
    case 6: return 1;
    case 7: return 2;
    case 8: return 3;
    default: return 8;
    }
}

bool ExifData::isEmpty() const
{
    return captionAbstract.isEmpty() && description.isEmpty()
        && dateTime.isEmpty() && make.isEmpty() && model.isEmpty()
        && exposureTime.isEmpty() && fNumber.isEmpty() && iso.isEmpty()
        && focalLength.isEmpty() && flash.isEmpty()
        && filename.isEmpty() && dimensions.isEmpty() && fileSize.isEmpty()
        && orientation.isEmpty() && latitude.isEmpty() && longitude.isEmpty()
        && osmLink.isEmpty();
}

QList<QPair<QString, QString>> ExifData::toList() const
{
    QList<QPair<QString, QString>> result;
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Caption"),      captionAbstract);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Description"),  description);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Date/Time"),    dateTime);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Camera Make"),  make);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Camera Model"), model);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Exposure"),     exposureTime);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Aperture"),     fNumber);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "ISO"),          iso);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Focal Length"), focalLength);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Flash"),        flash);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Filename"),     filename);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Dimensions"),   dimensions);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "File Size"),    fileSize);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Orientation"),  orientation);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Latitude"),     latitude);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "Longitude"),    longitude);
    addField(result, QT_TRANSLATE_NOOP("ExifReader", "OpenStreetMap"), osmLink);
    return result;
}

ExifData ExifReader::read(const QByteArray &path)
{
    ExifData data;

    const nativepath::NativeStat st = nativepath::nativeStat(path);
    if (!st.isRegular)
        return data;

    // Filename
    {
        const QByteArray nativeName =
            nativepath::nativeFromPath(nativepath::pathFromNative(path).filename());
        data.filename = nativepath::displayFromNative(nativeName);
    }

    // File size
    {
        qint64 size = st.size;
        if (size < 1024)
            data.fileSize = QString::number(size) + QLatin1String(" B");
        else if (size < 1024 * 1024)
            data.fileSize = QString::number(size / 1024.0, 'f', 1) + QLatin1String(" KiB");
        else
            data.fileSize = QString::number(size / (1024.0 * 1024.0), 'f', 1) + QLatin1String(" MiB");
    }

    // Image dimensions via Qt (reads only the header, no full decode needed)
    {
        QFile file;
        if (nativepath::openNativeRead(path, file)) {
            QImageReader reader(&file);
            reader.setDecideFormatFromContent(true);
            QSize sz = reader.size();
            if (sz.isValid())
                data.dimensions = QString::fromLatin1("%1 * %2")
                                      .arg(sz.width()).arg(sz.height());
        }
    }

    // EXIF tags via exiv2
    try {
        Exiv2ImagePtr image = openExiv2(path);
        image->readMetadata();
        const Exiv2::ExifData &exif = image->exifData();

        data.dateTime = formatDate(findTag(exif, "Exif.Photo.DateTimeOriginal"));
        if (data.dateTime.isEmpty())
            data.dateTime = formatDate(findTag(exif, "Exif.Image.DateTime"));
        data.make         = findTag(exif, "Exif.Image.Make");
        data.model        = findTag(exif, "Exif.Image.Model");
        data.exposureTime = findTag(exif, "Exif.Photo.ExposureTime");
        data.fNumber      = findTag(exif, "Exif.Photo.FNumber");
        data.iso          = findTag(exif, "Exif.Photo.ISOSpeedRatings");
        data.focalLength  = findTag(exif, "Exif.Photo.FocalLength");
        data.flash        = findTag(exif, "Exif.Photo.Flash");
        data.description  = findTag(exif, "Exif.Image.ImageDescription");
        data.orientation  = findTag(exif, "Exif.Image.Orientation");

        // GPS
        QString latStr = findTag(exif, "Exif.GPSInfo.GPSLatitude");
        QString latRef = findTag(exif, "Exif.GPSInfo.GPSLatitudeRef");
        if (!latStr.isEmpty() && !latRef.isEmpty()) {
            data.latitude = latStr + " " + latRef;
        }

        QString lonStr = findTag(exif, "Exif.GPSInfo.GPSLongitude");
        QString lonRef = findTag(exif, "Exif.GPSInfo.GPSLongitudeRef");
        if (!lonStr.isEmpty() && !lonRef.isEmpty()) {
            data.longitude = lonStr + " " + lonRef;
        }

        if (!data.latitude.isEmpty() && !data.longitude.isEmpty()) {
            double lat = parseGpsCoordinate(exif, "Exif.GPSInfo.GPSLatitude");
            if (latRef == "S") lat = -lat;
            double lon = parseGpsCoordinate(exif, "Exif.GPSInfo.GPSLongitude");
            if (lonRef == "W") lon = -lon;
            data.osmLink = QString::fromLatin1("https://www.openstreetmap.org/?mlat=%1&mlon=%2#map=16/%1/%2")
                               .arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6);
        }

        // IPTC Caption-Abstract
        const Exiv2::IptcData &iptc = image->iptcData();
        auto iptcIt = iptc.findKey(Exiv2::IptcKey("Iptc.Application2.Caption"));
        if (iptcIt != iptc.end())
            data.captionAbstract = QString::fromStdString(iptcIt->toString()).trimmed();
    } catch (const Exiv2::Error &) {
        // No EXIF or unsupported format — leave tags empty
    }

    return data;
}

bool ExifReader::saveCaption(const QByteArray &path,
                             const QString    &caption,
                             const ExifData   &oldData)
{
    try {
        Exiv2ImagePtr image = openExiv2(path);
        image->readMetadata();

        // Erase all existing Caption-Abstract entries then add the new one
        Exiv2::IptcData &iptc = image->iptcData();
        {
            Exiv2::IptcKey key("Iptc.Application2.Caption");
            auto it = iptc.findKey(key);
            while (it != iptc.end()) {
                it = iptc.erase(it);
                it = iptc.findKey(key);
            }
        }
        Exiv2::Iptcdatum datum(Exiv2::IptcKey("Iptc.Application2.Caption"));
        datum.setValue(caption.toStdString());
        iptc.add(datum);

        // If Caption-Abstract and Description were previously in sync, keep them in sync
        if (oldData.captionAbstract == oldData.description) {
            image->exifData()["Exif.Image.ImageDescription"] = caption.toStdString();
        }

        image->writeMetadata();
        return true;
    } catch (const Exiv2::Error &) {
        return false;
    }
}

bool ExifReader::rotate(const QByteArray &path, bool clockwise)
{
    try {
        Exiv2ImagePtr image = openExiv2(path);
        image->readMetadata();
        Exiv2::ExifData &exif = image->exifData();

        int current = 1; // EXIF default when the tag is absent
        auto it = exif.findKey(Exiv2::ExifKey("Exif.Image.Orientation"));
        if (it != exif.end())
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0,28,0)
            current = static_cast<int>(it->toInt64());
#else
            current = static_cast<int>(it->toLong());
#endif

        const int updated = clockwise ? orientationRotatedClockwise(current)
                                      : orientationRotatedCounterClockwise(current);
        exif["Exif.Image.Orientation"] = static_cast<uint16_t>(updated);

        image->writeMetadata();
        return true;
    } catch (const Exiv2::Error &) {
        return false;
    }
}
