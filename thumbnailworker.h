#pragma once

#include <QRunnable>
#include <QObject>
#include <QImage>
#include <atomic>

class ThumbnailWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    ThumbnailWorker(const QString &path,
                    int row,
                    std::atomic_bool *cancelFlag);

    void run() override;

signals:
    void finished(int row, const QImage &image);

private:
    QString m_path;
    int m_row;
    std::atomic_bool *m_cancelFlag;
};
