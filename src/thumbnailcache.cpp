#include "thumbnailcache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>

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

QString ThumbnailCache::canonicalUri(const QString &absPath)
{
    // Per the spec, use the absolute file URI (percent-encoded).
    return QUrl::fromLocalFile(absPath).toString(QUrl::FullyEncoded);
}

QString ThumbnailCache::hashedName(const QString &canonicalUri)
{
    QByteArray hash = QCryptographicHash::hash(canonicalUri.toUtf8(),
                                               QCryptographicHash::Md5)
                          .toHex();
    return QString::fromLatin1(hash) + ".png";
}

bool ThumbnailCache::isInsideCache(const QString &absPath)
{
    QString abs = QFileInfo(absPath).absoluteFilePath();
    QString root = QFileInfo(cacheRoot()).absoluteFilePath();
    if (root.isEmpty())
        return false;
    return abs.startsWith(root + "/") || abs == root;
}

QImage ThumbnailCache::load(const QString &sourcePath, Size size)
{
    QFileInfo info(sourcePath);
    if (!info.exists() || !info.isFile())
        return {};

    const QString uri = canonicalUri(info.absoluteFilePath());
    const QString cachePath = subdirFor(size) + "/" + hashedName(uri);

    QImage img;
    if (!img.load(cachePath, "PNG"))
        return {};

    const qint64 mtime = info.lastModified().toSecsSinceEpoch();
    bool ok = false;
    const qint64 cachedMTime = img.text("Thumb::MTime").toLongLong(&ok);
    if (!ok || cachedMTime != mtime)
        return {};

    return img;
}

void ThumbnailCache::save(const QString &sourcePath, Size size, const QImage &thumbImage)
{
    if (thumbImage.isNull())
        return;

    QFileInfo info(sourcePath);
    if (!info.exists() || !info.isFile())
        return;

    const QString uri = canonicalUri(info.absoluteFilePath());
    const QString subdir = subdirFor(size);
    const QString finalPath = subdir + "/" + hashedName(uri);

    QDir dir;
    if (!dir.mkpath(subdir))
        return;
    QFile::setPermissions(subdir, dirPerms());
    QFile::setPermissions(cacheRoot(), dirPerms());

    QImage tagged = thumbImage;
    tagged.setText("Thumb::URI", uri);
    tagged.setText("Thumb::MTime",
                   QString::number(info.lastModified().toSecsSinceEpoch()));
    tagged.setText("Thumb::Size", QString::number(info.size()));
    QMimeDatabase mimeDb;
    QString mime = mimeDb.mimeTypeForFile(info).name();
    if (!mime.isEmpty())
        tagged.setText("Thumb::Mimetype", mime);
    tagged.setText("Thumb::Image::Width", QString::number(thumbImage.width()));
    tagged.setText("Thumb::Image::Height", QString::number(thumbImage.height()));
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
