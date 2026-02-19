#pragma once

#include <QAbstractListModel>
#include <QPixmap>
#include <QVector>

class ImageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ImageModel(QObject *parent = nullptr);

    void setDirectory(const QString &path);
    QString filePath(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    struct Item
    {
        QString path;
        QPixmap thumbnail;
    };

    QVector<Item> m_items;
};
