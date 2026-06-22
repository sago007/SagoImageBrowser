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
    // Discard the cached thumbnail for a row and re-decode it from disk.
    void refreshThumbnail(const QModelIndex &index);
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
