#include "exifreader.h"

#include <QFile>
#include <QImageReader>

#include <exiv2/exiv2.hpp>

#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>

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
        result.append({QString::fromLatin1(label), value});
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
    addField(result, "Caption",      captionAbstract);
    addField(result, "Description",  description);
    addField(result, "Date/Time",    dateTime);
    addField(result, "Camera Make",  make);
    addField(result, "Camera Model", model);
    addField(result, "Exposure",     exposureTime);
    addField(result, "Aperture",     fNumber);
    addField(result, "ISO",          iso);
    addField(result, "Focal Length", focalLength);
    addField(result, "Flash",        flash);
    addField(result, "Filename",     filename);
    addField(result, "Dimensions",   dimensions);
    addField(result, "File Size",    fileSize);
    addField(result, "Orientation",  orientation);
    addField(result, "Latitude",     latitude);
    addField(result, "Longitude",    longitude);
    addField(result, "OpenStreetMap", osmLink);
    return result;
}

ExifData ExifReader::read(const QByteArray &path)
{
    ExifData data;

    struct ::stat st{};
    if (::stat(path.constData(), &st) != 0 || !S_ISREG(st.st_mode))
        return data;

    // Filename
    {
        const std::filesystem::path fsPath(path.toStdString());
        const std::string nativeName = fsPath.filename().native();
        data.filename = QString::fromLocal8Bit(
            nativeName.data(), static_cast<qsizetype>(nativeName.size()));
    }

    // File size
    {
        qint64 size = static_cast<qint64>(st.st_size);
        if (size < 1024)
            data.fileSize = QString::number(size) + QLatin1String(" B");
        else if (size < 1024 * 1024)
            data.fileSize = QString::number(size / 1024.0, 'f', 1) + QLatin1String(" KiB");
        else
            data.fileSize = QString::number(size / (1024.0 * 1024.0), 'f', 1) + QLatin1String(" MiB");
    }

    // Image dimensions via Qt (reads only the header, no full decode needed)
    {
        int fd = ::open(path.constData(), O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            QFile file;
            if (file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
                QImageReader reader(&file);
                reader.setDecideFormatFromContent(true);
                QSize sz = reader.size();
                        if (sz.isValid())
                    data.dimensions = QString::fromLatin1("%1 * %2")
                                          .arg(sz.width()).arg(sz.height());
            } else {
                ::close(fd);
            }
        }
    }

    // EXIF tags via exiv2
    try {
        std::unique_ptr<Exiv2::Image> image =
            Exiv2::ImageFactory::open(path.toStdString());
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
            data.latitude = latStr + u" " + latRef;
        }

        QString lonStr = findTag(exif, "Exif.GPSInfo.GPSLongitude");
        QString lonRef = findTag(exif, "Exif.GPSInfo.GPSLongitudeRef");
        if (!lonStr.isEmpty() && !lonRef.isEmpty()) {
            data.longitude = lonStr + u" " + lonRef;
        }

        if (!data.latitude.isEmpty() && !data.longitude.isEmpty()) {
            double lat = parseGpsCoordinate(exif, "Exif.GPSInfo.GPSLatitude");
            if (latRef == u"S") lat = -lat;
            double lon = parseGpsCoordinate(exif, "Exif.GPSInfo.GPSLongitude");
            if (lonRef == u"W") lon = -lon;
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
        std::unique_ptr<Exiv2::Image> image =
            Exiv2::ImageFactory::open(path.toStdString());
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
        std::unique_ptr<Exiv2::Image> image =
            Exiv2::ImageFactory::open(path.toStdString());
        image->readMetadata();
        Exiv2::ExifData &exif = image->exifData();

        int current = 1; // EXIF default when the tag is absent
        auto it = exif.findKey(Exiv2::ExifKey("Exif.Image.Orientation"));
        if (it != exif.end())
            current = static_cast<int>(it->toInt64());

        const int updated = clockwise ? orientationRotatedClockwise(current)
                                      : orientationRotatedCounterClockwise(current);
        exif["Exif.Image.Orientation"] = static_cast<uint16_t>(updated);

        image->writeMetadata();
        return true;
    } catch (const Exiv2::Error &) {
        return false;
    }
}
