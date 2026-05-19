#pragma once

#include "thumbnailcache.h"

#include <QByteArray>
#include <QRunnable>
#include <QObject>
#include <QImage>
#include <atomic>

class ThumbnailWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    ThumbnailWorker(const QByteArray &path,
                    int row,
                    ThumbnailCache::Size size,
                    std::atomic_bool *cancelFlag);

    void run() override;

signals:
    void finished(int row, const QImage &image);

private:
    QByteArray m_path;
    int m_row;
    ThumbnailCache::Size m_size;
    std::atomic_bool *m_cancelFlag;
};
