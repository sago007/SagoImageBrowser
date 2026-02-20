#pragma once

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QString>

class ImageViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewWidget(QWidget *parent = nullptr);

    void setImage(const QString &path);

signals:
    void closeRequested();
    void nextRequested();
    void previousRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPixmap m_pixmap;
};
