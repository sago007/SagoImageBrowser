#pragma once

#include <QAbstractListModel>
#include <QPixmap>
#include <QVector>
#include <QThreadPool>
#include <QSet>
#include <atomic>

class ThumbnailWorker;

class ImageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ImageModel(QObject *parent = nullptr);
    ~ImageModel();

    void setDirectory(const QString &path);
    QString filePath(const QModelIndex &index) const;

    void requestThumbnails(int firstRow, int lastRow);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void thumbnailReady(int row, const QImage &image);

private:
    struct Item
    {
        QString path;
        QPixmap thumbnail;
        bool loaded = false;
    };

    void queueRow(int row);

    QVector<Item> m_items;
    QThreadPool m_threadPool;
    QPixmap m_placeholder;

    QSet<int> m_pendingRows;
    std::atomic_bool m_cancelFlag{false};
};
