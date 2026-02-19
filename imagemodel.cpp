#include "imagemodel.h"
#include "thumbnailworker.h"

#include <QDir>
#include <QImageReader>
#include <QFileInfo>

ImageModel::ImageModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount());

    // Simple gray placeholder
    m_placeholder = QPixmap(128, 128);
    m_placeholder.fill(Qt::lightGray);
}

ImageModel::~ImageModel()
{
    m_threadPool.waitForDone();
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

    const auto files = dir.entryInfoList();

    int row = 0;
    for (const QFileInfo &file : files)
    {
        m_items.append({file.absoluteFilePath(), m_placeholder, false});

        // Create worker
        auto *worker = new ThumbnailWorker(file.absoluteFilePath(), row);

        connect(worker, &ThumbnailWorker::finished,
                this, &ImageModel::thumbnailReady,
                Qt::QueuedConnection);

        m_threadPool.start(worker);

        row++;
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

void ImageModel::thumbnailReady(int row, const QPixmap &pixmap)
{
    if (row < 0 || row >= m_items.size())
        return;

    m_items[row].thumbnail = pixmap;
    m_items[row].loaded = true;

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {Qt::DecorationRole});
}
