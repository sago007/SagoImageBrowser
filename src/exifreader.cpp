#include "exifreader.h"

#include <QFileInfo>
#include <QImageReader>

#include <exiv2/exiv2.hpp>

bool ExifData::isEmpty() const
{
    return dateTime.isEmpty() && make.isEmpty() && model.isEmpty()
        && exposureTime.isEmpty() && fNumber.isEmpty() && iso.isEmpty()
        && focalLength.isEmpty() && flash.isEmpty()
        && dimensions.isEmpty() && fileSize.isEmpty();
}

QList<QPair<QString, QString>> ExifData::toList() const
{
    QList<QPair<QString, QString>> result;
    auto add = [&](const char *label, const QString &value) {
        if (!value.isEmpty())
            result.append({QString::fromLatin1(label), value});
    };
    add("Date/Time",    dateTime);
    add("Camera Make",  make);
    add("Camera Model", model);
    add("Exposure",     exposureTime);
    add("Aperture",     fNumber);
    add("ISO",          iso);
    add("Focal Length", focalLength);
    add("Flash",        flash);
    add("Dimensions",   dimensions);
    add("File Size",    fileSize);
    return result;
}

ExifData ExifReader::read(const QString &path)
{
    ExifData data;

    QFileInfo fi(path);
    if (!fi.exists() || fi.isDir())
        return data;

    // File size
    {
        qint64 size = fi.size();
        if (size < 1024)
            data.fileSize = QString::number(size) + QLatin1String(" B");
        else if (size < 1024 * 1024)
            data.fileSize = QString::number(size / 1024.0, 'f', 1) + QLatin1String(" KiB");
        else
            data.fileSize = QString::number(size / (1024.0 * 1024.0), 'f', 1) + QLatin1String(" MiB");
    }

    // Image dimensions via Qt (reads only the header, no full decode needed)
    {
        QImageReader reader(path);
        reader.setDecideFormatFromContent(true);
        QSize sz = reader.size();
        if (sz.isValid())
            data.dimensions = QString::fromLatin1("%1 * %2")
                                  .arg(sz.width()).arg(sz.height());
    }

    // EXIF tags via exiv2
    try {
        auto image = Exiv2::ImageFactory::open(path.toStdString());
        image->readMetadata();
        const Exiv2::ExifData &exif = image->exifData();

        auto findTag = [&](const std::string &key) -> QString {
            try {
                auto it = exif.findKey(Exiv2::ExifKey(key));
                if (it != exif.end())
                    return QString::fromStdString(it->print(&exif)).trimmed();
            } catch (...) {}
            return {};
        };

        data.dateTime = findTag("Exif.Photo.DateTimeOriginal");
        if (data.dateTime.isEmpty())
            data.dateTime = findTag("Exif.Image.DateTime");
        data.make         = findTag("Exif.Image.Make");
        data.model        = findTag("Exif.Image.Model");
        data.exposureTime = findTag("Exif.Photo.ExposureTime");
        data.fNumber      = findTag("Exif.Photo.FNumber");
        data.iso          = findTag("Exif.Photo.ISOSpeedRatings");
        data.focalLength  = findTag("Exif.Photo.FocalLength");
        data.flash        = findTag("Exif.Photo.Flash");
    } catch (const Exiv2::Error &) {
        // No EXIF or unsupported format — leave tags empty
    }

    return data;
}
