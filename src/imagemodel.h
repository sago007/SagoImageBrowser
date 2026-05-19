#pragma once

#include "thumbnailcache.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QPixmap>
#include <QVector>
#include <QThreadPool>
#include <QSet>
#include <QHash>
#include <QList>
#include <atomic>
#include <QIcon>

class ThumbnailWorker;

class ImageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ImageModel(QObject *parent = nullptr);
    ~ImageModel();

    // path is the raw native OS byte string (handles non-UTF-8 filenames)
    void setDirectory(const QByteArray &path);
    void setDirectory(const QString &path);   // convenience: converts via QFile::encodeName
    QByteArray filePath(const QModelIndex &index) const;
    bool isFolder(const QModelIndex &index) const;
    QByteArray currentDirectory() const;

    void requestThumbnails(int firstRow, int lastRow);
    void setCacheMaxSize(int n);
    void setThumbnailSize(ThumbnailCache::Size size);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void thumbnailReady(int row, const QImage &image);

private:
    struct Item
    {
        QByteArray path;      // raw native OS bytes — safe for non-UTF-8 filenames
        QString displayName;
        QPixmap thumbnail;
        bool loaded = false;
        bool isFolder = false;
    };

    void queueRow(int row);

    using ThumbnailMap = QHash<QByteArray, QPixmap>;

    QVector<Item> m_items;
    QThreadPool m_threadPool;
    QPixmap m_placeholder;

    QSet<int> m_pendingRows;
    std::atomic_bool m_cancelFlag{false};
    QIcon m_folderIcon;
    QByteArray m_currentDir;

    // Folder thumbnail cache (last N visited directories)
    QHash<QByteArray, ThumbnailMap> m_folderThumbnailCache;
    QList<QByteArray> m_cacheOrder;
    int m_cacheMaxSize = 4;

    ThumbnailCache::Size m_thumbnailSize = ThumbnailCache::Size::Normal;
};
