#pragma once

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QPoint>
#include <QMap>
#include <QStringList>
#include <QThread>
#include "exifreader.h"

class ImageLoadWorker : public QThread
{
    Q_OBJECT
public:
    explicit ImageLoadWorker(const QString &path, QObject *parent = nullptr);
    void run() override;
signals:
    void imageLoaded(const QString &path, const QPixmap &pixmap);
private:
    QString m_path;
};

class ImageViewWidget : public QWidget
{
    Q_OBJECT

public:
    // Number of images to read ahead and keep behind the current image
    static constexpr int CacheReadAhead = 2;
    static constexpr int CacheKeepBehind = 2;

    enum ZoomMode { FitToScreen, OriginalSize, CustomZoom };

    explicit ImageViewWidget(QWidget *parent = nullptr);

    void setImage(const QString &path);
    void setBackgroundColor(const QString &color);
    void zoomIn();
    void zoomOut();
    void zoomOriginal();
    void zoomFitToScreen();

    void prefetchImage(const QString &path);
    void setNeighborPaths(const QStringList &paths);
    void clearCache();

    void setExifData(const ExifData &data);
    void toggleExifOverlay();

signals:
    void closeRequested();
    void nextRequested();
    void previousRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void clampOffset(int imgW, int imgH);
    QPixmap loadImageFromDisk(const QString &path);
    void onImageLoaded(const QString &path, const QPixmap &pixmap);

    QPixmap m_pixmap;
    ZoomMode m_zoomMode = FitToScreen;
    double m_zoomFactor = 1.0;
    QString m_backgroundColor = "black";
    QString m_currentPath;

    // Image cache: path -> pixmap
    QMap<QString, QPixmap> m_cache;
    QStringList m_neighborPaths;       // ordered list of paths around current image
    QSet<QString> m_pendingLoads;      // paths currently being loaded in background

    // Panning state
    QPoint m_offset;        // current pan offset (pixels)
    bool   m_dragging = false;
    QPoint m_dragStart;
    QPoint m_offsetAtDragStart;

    // EXIF overlay
    ExifData m_exifData;
    bool m_showExifOverlay = false;
};
