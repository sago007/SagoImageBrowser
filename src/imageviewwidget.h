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

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QSet>
#include <QThread>
#include "exifreader.h"

class ImageLoadWorker : public QThread
{
    Q_OBJECT
public:
    explicit ImageLoadWorker(const QByteArray &path, QObject *parent = nullptr);
    void run() override;
signals:
    void imageLoaded(const QByteArray &path, const QPixmap &pixmap);
private:
    QByteArray m_path;
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

    void setImage(const QByteArray &path);
    void setBackgroundColor(const QString &color);
    void zoomIn();
    void zoomOut();
    void zoomOriginal();
    void zoomFitToScreen();

    void prefetchImage(const QByteArray &path);
    void setNeighborPaths(const QList<QByteArray> &paths);
    void clearCache();
    // Drop any cached pixmap for path so it is re-decoded from disk next time.
    void invalidateCache(const QByteArray &path);

    void setExifData(const ExifData &data);
    void toggleExifOverlay();
    QByteArray currentPath() const;

signals:
    void closeRequested();
    void nextRequested();
    void previousRequested();
    void editCaptionRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void clampOffset(int imgW, int imgH);
    QPixmap loadImageFromDisk(const QByteArray &path);
    void onImageLoaded(const QByteArray &path, const QPixmap &pixmap);

    QPixmap m_pixmap;
    ZoomMode m_zoomMode = FitToScreen;
    double m_zoomFactor = 1.0;
    QString m_backgroundColor = "black";
    QByteArray m_currentPath;

    // Image cache: path -> pixmap
    QMap<QByteArray, QPixmap> m_cache;
    QList<QByteArray> m_neighborPaths;    // ordered list of paths around current image
    QSet<QByteArray> m_pendingLoads;      // paths currently being loaded in background

    // Panning state
    QPoint m_offset;        // current pan offset (pixels)
    bool   m_dragging = false;
    QPoint m_dragStart;
    QPoint m_offsetAtDragStart;

    // EXIF overlay
    ExifData m_exifData;
    bool m_showExifOverlay = false;

    // Help overlay
    bool m_showHelpOverlay = false;

    // Clickable link areas
    QRect m_osmLinkRect;
};
