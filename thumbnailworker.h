#pragma once

#include <QRunnable>
#include <QObject>
#include <QPixmap>

class ThumbnailWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    ThumbnailWorker(const QString &path, int row);

    void run() override;

signals:
    void finished(int row, const QPixmap &pixmap);

private:
    QString m_path;
    int m_row;
};
