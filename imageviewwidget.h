#pragma once

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QPoint>

class ImageViewWidget : public QWidget
{
    Q_OBJECT

public:
    enum ZoomMode { FitToScreen, OriginalSize, CustomZoom };

    explicit ImageViewWidget(QWidget *parent = nullptr);

    void setImage(const QString &path);
    void zoomIn();
    void zoomOut();
    void zoomOriginal();
    void zoomFitToScreen();

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

private:
    void clampOffset(int imgW, int imgH);

    QPixmap m_pixmap;
    ZoomMode m_zoomMode = FitToScreen;
    double m_zoomFactor = 1.0;

    // Panning state
    QPoint m_offset;        // current pan offset (pixels)
    bool   m_dragging = false;
    QPoint m_dragStart;
    QPoint m_offsetAtDragStart;
};
