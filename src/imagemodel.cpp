#include "imagemodel.h"
#include "thumbnailworker.h"

#include <QDir>
#include <QImageReader>
#include <QFileInfo>
#include <QFileIconProvider>

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

    QDir dir(path);

    // Add subdirectories first
    dir.setFilter(QDir::AllDirs | QDir::NoDot);
    dir.setSorting(QDir::Name);
    const auto dirs = dir.entryInfoList();
    QPixmap folderPixmap = m_folderIcon.pixmap(128, 128);
    for (const QFileInfo &d : dirs)
    {
        QString name = d.fileName();  // preserves ".." for the parent entry
        m_items.append({d.absoluteFilePath(), name, folderPixmap, true, true});
    }

    // Then add image files
    QStringList filters;

    const auto formats = QImageReader::supportedImageFormats();
    for (const QByteArray &fmt : formats)
        filters << "*." + fmt;

    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    const auto files = dir.entryInfoList();

    // Restore cached thumbnails for this folder if available
    const ThumbnailMap *cached = m_folderThumbnailCache.contains(path)
        ? &m_folderThumbnailCache[path]
        : nullptr;

    for (const QFileInfo &file : files)
    {
        QPixmap thumb = m_placeholder;
        bool loaded = false;
        if (cached) {
            auto it = cached->find(file.absoluteFilePath());
            if (it != cached->end()) {
                thumb = it.value();
                loaded = true;
            }
        }
        m_items.append({file.absoluteFilePath(), file.fileName(), thumb, loaded, false});
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

    connect(worker, &ThumbnailWorker::finished,
            this, &ImageModel::thumbnailReady,
            Qt::QueuedConnection);

    m_threadPool.start(worker);
}

QString ImageModel::filePath(const QModelIndex &index) const
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

QString ImageModel::currentDirectory() const
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
