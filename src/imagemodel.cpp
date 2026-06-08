#include "imagemodel.h"
#include "thumbnailworker.h"

#include <QImageReader>
#include <QFileIconProvider>

#include <algorithm>
#include <filesystem>
#include <vector>

ImageModel::ImageModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Limit 1 thread for thumbnail loading to avoid excessive disk I/O on harddrives. SSDs are so fast the extra threads don't help much anyway.
    m_threadPool.setMaxThreadCount(1);  //QThread::idealThreadCount()

    m_placeholder = QPixmap(128, 128);
    m_placeholder.fill(Qt::lightGray);

    QFileIconProvider iconProvider;
    m_folderIcon = iconProvider.icon(QFileIconProvider::Folder);
}

ImageModel::~ImageModel()
{
    m_cancelFlag = true;
    m_threadPool.waitForDone();
}

void ImageModel::setCacheMaxSize(int n)
{
    m_cacheMaxSize = std::max(0, n);
    while (m_cacheOrder.size() > m_cacheMaxSize)
        m_folderThumbnailCache.remove(m_cacheOrder.takeFirst());
}

void ImageModel::setThumbnailSize(ThumbnailCache::Size size)
{
    if (size == m_thumbnailSize)
        return;

    m_thumbnailSize = size;

    // Cancel any in-flight workers (they were started with the previous size).
    m_cancelFlag = true;
    m_threadPool.waitForDone();
    m_cancelFlag = false;

    m_pendingRows.clear();

    // The folder cache is keyed by directory and size-agnostic; drop it so the
    // user actually sees the new size.
    m_folderThumbnailCache.clear();
    m_cacheOrder.clear();

    // Mark existing image entries unloaded so they get re-fetched at the new size.
    if (!m_items.isEmpty())
    {
        for (int row = 0; row < m_items.size(); ++row)
        {
            Item &item = m_items[row];
            if (item.isFolder)
                continue;
            item.thumbnail = m_placeholder;
            item.loaded = false;
        }
        emit dataChanged(index(0), index(m_items.size() - 1),
                         {Qt::DecorationRole});
    }
}

void ImageModel::setDirectory(const QString &path)
{
    setDirectory(QFile::encodeName(path));
}

void ImageModel::setDirectory(const QByteArray &path)
{
    m_cancelFlag = true;
    m_threadPool.waitForDone();
    m_cancelFlag = false;

    // Save current folder's thumbnails to cache before discarding them
    if (!m_currentDir.isEmpty()) {
        ThumbnailMap map;
        for (const Item &item : m_items)
            if (item.loaded && !item.isFolder)
                map[item.path] = item.thumbnail;
        if (!map.isEmpty()) {
            if (!m_cacheOrder.contains(m_currentDir)) {
                m_cacheOrder.append(m_currentDir);
                if (m_cacheOrder.size() > m_cacheMaxSize)
                    m_folderThumbnailCache.remove(m_cacheOrder.takeFirst());
            }
            m_folderThumbnailCache[m_currentDir] = std::move(map);
        }
    }

    beginResetModel();
    m_items.clear();
    m_pendingRows.clear();
    m_currentDir = path;

    // Build set of supported image extensions (lower-case, without dot)
    QSet<QByteArray> imageExts;
    for (const QByteArray &fmt : QImageReader::supportedImageFormats())
        imageExts.insert(fmt.toLower());

    // Use std::filesystem::directory_iterator so that directory entries with
    // non-UTF-8 filenames (e.g. Latin-1 encoded names) are not silently
    // skipped or mangled as they would be with QDir::entryInfoList().
    namespace fs = std::filesystem;

    std::vector<std::string> dirs;
    std::vector<std::string> imageFiles;

    try {
        fs::path dirPath(path.toStdString());

        // Add ".." parent entry directly with the literal display name ".."
        // (mirrors QDir::NoDot behaviour)
        fs::path parentPath = dirPath.parent_path();
        if (!parentPath.empty() && parentPath != dirPath) {
            QPixmap folderPixmap = m_folderIcon.pixmap(128, 128);
            m_items.append({QByteArray::fromStdString(parentPath.native()),
                            QStringLiteral(".."), folderPixmap, true, true});
        }

        for (const fs::directory_entry &entry :
             fs::directory_iterator(dirPath,
                 fs::directory_options::skip_permission_denied))
        {
            std::error_code ec;
            const std::string fname = entry.path().filename().native();
            if (fname.empty() || fname[0] == '.')
                continue;   // skip hidden entries

            if (entry.is_directory(ec)) {
                dirs.push_back(entry.path().native());
            } else if (entry.is_regular_file(ec)) {
                // Filter by image extension
                const std::string &nativeExt = entry.path().extension().native();
                if (nativeExt.empty())
                    continue;
                // extension() includes the dot; strip it and lower-case
                QByteArray ext = QByteArray(nativeExt.c_str() + 1,
                                           static_cast<qsizetype>(nativeExt.size()) - 1).toLower();
                if (imageExts.contains(ext))
                    imageFiles.push_back(entry.path().native());
            }
        }
    } catch (const fs::filesystem_error &) {
        // Directory unreadable — show empty listing
    }

    // Sort subdirectories by filename
    std::sort(dirs.begin(), dirs.end(),
              [](const auto &a, const auto &b) {
                  return fs::path(a).filename() < fs::path(b).filename();
              });

    std::sort(imageFiles.begin(), imageFiles.end(),
              [](const auto &a, const auto &b) {
                  return fs::path(a).filename() < fs::path(b).filename();
              });

    QPixmap folderPixmap = m_folderIcon.pixmap(128, 128);
    for (const std::string &nativePath : dirs)
    {
        QByteArray entryPath = QByteArray::fromStdString(nativePath);
        std::string fname = fs::path(nativePath).filename().native();
        QString displayName = QString::fromLocal8Bit(fname.c_str(),
                                                     static_cast<qsizetype>(fname.size()));
        m_items.append({entryPath, displayName, folderPixmap, true, true});
    }

    // Restore cached thumbnails for this folder if available
    const ThumbnailMap *cached = m_folderThumbnailCache.contains(path)
        ? &m_folderThumbnailCache[path]
        : nullptr;

    for (const std::string &nativePath : imageFiles)
    {
        QByteArray entryPath = QByteArray::fromStdString(nativePath);
        std::string fname = fs::path(nativePath).filename().native();
        QString displayName = QString::fromLocal8Bit(fname.c_str(),
                                                     static_cast<qsizetype>(fname.size()));

        QPixmap thumb = m_placeholder;
        bool loaded = false;
        if (cached) {
            auto it = cached->find(entryPath);
            if (it != cached->end()) {
                thumb = it.value();
                loaded = true;
            }
        }
        m_items.append({entryPath, displayName, thumb, loaded, false});
    }

    endResetModel();
}

void ImageModel::requestThumbnails(int firstRow, int lastRow)
{
    if (m_items.isEmpty())
        return;

    firstRow = std::max(0, firstRow);
    lastRow = std::min(lastRow, static_cast<int>(m_items.size()) - 1);

    for (int row = firstRow; row <= lastRow; ++row)
    {
        if (!m_items[row].loaded && !m_items[row].isFolder)
            queueRow(row);
    }

    // Optional: preload nearby rows
    int buffer = 20;
    for (int row = firstRow - buffer; row < firstRow; ++row)
        if (row >= 0 && !m_items[row].loaded && !m_items[row].isFolder)
            queueRow(row);

    for (int row = lastRow + 1; row <= lastRow + buffer; ++row)
        if (row < m_items.size() && !m_items[row].loaded && !m_items[row].isFolder)
            queueRow(row);
}

void ImageModel::refreshThumbnail(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const int row = index.row();
    if (row < 0 || row >= m_items.size() || m_items[row].isFolder)
        return;

    // The on-disk thumbnail cache is keyed by source mtime, so it invalidates
    // itself once the file is rewritten. Drop the in-memory copy and re-decode.
    m_items[row].thumbnail = m_placeholder;
    m_items[row].loaded = false;
    emit dataChanged(index, index, {Qt::DecorationRole});
    queueRow(row);
}

void ImageModel::queueRow(int row)
{
    if (m_pendingRows.contains(row))
        return;

    m_pendingRows.insert(row);

    auto *worker = new ThumbnailWorker(
        m_items[row].path,
        row,
        m_thumbnailSize,
        &m_cancelFlag);

    // THREADING: Explicit QueuedConnection ensures thumbnailReady() is invoked
    // on the main thread even though the signal is emitted from a pool thread.
    // This means all m_items[] mutations happen on the main thread only.
    connect(worker, &ThumbnailWorker::finished,
            this, &ImageModel::thumbnailReady,
            Qt::QueuedConnection);

    m_threadPool.start(worker);
}

QByteArray ImageModel::filePath(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    return m_items[index.row()].path;
}

bool ImageModel::isFolder(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return false;
    return m_items[index.row()].isFolder;
}

QByteArray ImageModel::currentDirectory() const
{
    return m_currentDir;
}

int ImageModel::rowCount(const QModelIndex &) const
{
    return m_items.size();
}

QVariant ImageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const Item &item = m_items[index.row()];

    if (role == Qt::DecorationRole)
        return item.thumbnail;

    if (role == Qt::DisplayRole)
        return item.displayName;

    if (role == Qt::UserRole)
        return item.isFolder;

    return {};
}

void ImageModel::thumbnailReady(int row, const QImage &image)
{
    if (m_cancelFlag)
        return;

    if (row < 0 || row >= m_items.size())
        return;

    m_items[row].thumbnail = QPixmap::fromImage(image);
    m_items[row].loaded = true;
    m_pendingRows.remove(row);

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {Qt::DecorationRole});
}
