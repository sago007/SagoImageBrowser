#include "thumbnailcache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>

#include <sys/stat.h>

namespace
{
constexpr const char *kSoftware = "SagoImageBrowser";

QFile::Permissions filePerms()
{
    return QFile::ReadOwner | QFile::WriteOwner;
}

QFile::Permissions dirPerms()
{
    return QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
}
} // namespace

QString ThumbnailCache::cacheRoot()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    if (base.isEmpty())
        base = QDir::homePath() + "/.cache";
    return base + "/thumbnails";
}

QString ThumbnailCache::subdirFor(Size size)
{
    const char *name = (size == Size::Large) ? "large" : "normal";
    return cacheRoot() + "/" + name;
}

QByteArray ThumbnailCache::canonicalUri(const QByteArray &absPath)
{
    // Build a correct file:// URI by percent-encoding the raw byte path.
    // Forward slashes are kept unencoded; all other bytes (including non-UTF-8
    // ones such as Latin-1 \xF8) are percent-encoded.
    return "file://" + QUrl::toPercentEncoding(absPath, "/");
}

QString ThumbnailCache::hashedName(const QByteArray &canonicalUri)
{
    QByteArray hash =
        QCryptographicHash::hash(canonicalUri, QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(hash) + ".png";
}

bool ThumbnailCache::isInsideCache(const QByteArray &absPath)
{
    QByteArray root = cacheRoot().toUtf8(); // cache root is always valid UTF-8
    return absPath.startsWith(root + '/') || absPath == root;
}

QImage ThumbnailCache::load(const QByteArray &sourcePath, Size size)
{
    struct ::stat st{};
    if (::stat(sourcePath.constData(), &st) != 0 || !S_ISREG(st.st_mode))
        return {};

    const QByteArray uri = canonicalUri(sourcePath);
    const QString cachePath = subdirFor(size) + "/" + hashedName(uri);

    QImage img;
    if (!img.load(cachePath, "PNG"))
        return {};

    const qint64 mtime = static_cast<qint64>(st.st_mtime);
    bool ok = false;
    const qint64 cachedMTime = img.text("Thumb::MTime").toLongLong(&ok);
    if (!ok || cachedMTime != mtime)
        return {};

    return img;
}

void ThumbnailCache::save(const QByteArray &sourcePath, Size size, const QImage &thumbImage)
{
    if (thumbImage.isNull())
        return;

    struct ::stat st{};
    if (::stat(sourcePath.constData(), &st) != 0 || !S_ISREG(st.st_mode))
        return;

    const QByteArray uri = canonicalUri(sourcePath);
    const QString subdir = subdirFor(size);
    const QString finalPath = subdir + "/" + hashedName(uri);

    QDir dir;
    if (!dir.mkpath(subdir))
        return;
    QFile::setPermissions(subdir, dirPerms());
    QFile::setPermissions(cacheRoot(), dirPerms());

    QImage tagged = thumbImage;
    tagged.setText("Thumb::URI", QString::fromLatin1(uri));
    tagged.setText("Thumb::MTime",
                   QString::number(static_cast<qint64>(st.st_mtime)));
    tagged.setText("Thumb::Size", QString::number(static_cast<qint64>(st.st_size)));
    tagged.setText("Software", QString::fromLatin1(kSoftware));

    QTemporaryFile tmp(subdir + "/tmp_XXXXXX.png");
    tmp.setAutoRemove(false);
    if (!tmp.open())
        return;
    const QString tmpPath = tmp.fileName();
    tmp.close();

    if (!tagged.save(tmpPath, "PNG"))
    {
        QFile::remove(tmpPath);
        return;
    }

    QFile::setPermissions(tmpPath, filePerms());

    // Atomic replace: remove existing target then rename.
    QFile::remove(finalPath);
    if (!QFile::rename(tmpPath, finalPath))
        QFile::remove(tmpPath);
}
