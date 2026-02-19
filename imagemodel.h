#pragma once

#include <QAbstractListModel>
#include <QPixmap>
#include <QVector>
#include <QThreadPool>

class ThumbnailWorker;

class ImageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ImageModel(QObject *parent = nullptr);
    ~ImageModel();

    void setDirectory(const QString &path);
    QString filePath(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void thumbnailReady(int row, const QPixmap &pixmap);

private:
    struct Item
    {
        QString path;
        QPixmap thumbnail;
        bool loaded = false;
    };

    QVector<Item> m_items;
    QThreadPool m_threadPool;
    QPixmap m_placeholder;
};
