#pragma once

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QString>

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

private:
    QPixmap m_pixmap;
    ZoomMode m_zoomMode = FitToScreen;
    double m_zoomFactor = 1.0;
};
