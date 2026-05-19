#include "exifreader.h"

#include <QFile>
#include <QImageReader>

#include <exiv2/exiv2.hpp>

#include <sys/stat.h>
#include <fcntl.h>
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
    addField(result, "Date/Time",    dateTime);
    addField(result, "Camera Make",  make);
    addField(result, "Camera Model", model);
    addField(result, "Exposure",     exposureTime);
    addField(result, "Aperture",     fNumber);
    addField(result, "ISO",          iso);
    addField(result, "Focal Length", focalLength);
    addField(result, "Flash",        flash);
    addField(result, "Dimensions",   dimensions);
    addField(result, "File Size",    fileSize);
    return result;
}

ExifData ExifReader::read(const QByteArray &path)
{
    ExifData data;

    struct ::stat st{};
    if (::stat(path.constData(), &st) != 0 || !S_ISREG(st.st_mode))
        return data;

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
    } catch (const Exiv2::Error &) {
        // No EXIF or unsupported format — leave tags empty
    }

    return data;
}
