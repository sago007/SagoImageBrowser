#include "imagemodel.h"

#include <QDir>
#include <QImageReader>
#include <QFileInfo>

ImageModel::ImageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ImageModel::setDirectory(const QString &path)
{
    beginResetModel();
    m_items.clear();

    QDir dir(path);
    QStringList filters;

    const auto formats = QImageReader::supportedImageFormats();
    for (const QByteArray &fmt : formats)
        filters << "*." + fmt;

    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);

    for (const QFileInfo &file : dir.entryInfoList())
    {
        QImageReader reader(file.absoluteFilePath());
        reader.setAutoTransform(true);

        QImage image = reader.read();
        QPixmap thumb = QPixmap::fromImage(
            image.scaled(128, 128, Qt::KeepAspectRatio,
                         Qt::SmoothTransformation));

        m_items.append({file.absoluteFilePath(), thumb});
    }

    endResetModel();
}

QString ImageModel::filePath(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    return m_items[index.row()].path;
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
        return QFileInfo(item.path).fileName();

    return {};
}
