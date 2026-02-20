#include "imageviewwidget.h"

#include <QPainter>
#include <QKeyEvent>
#include <QImageReader>

ImageViewWidget::ImageViewWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: black;");
}

void ImageViewWidget::setImage(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();

    if (!image.isNull())
        m_pixmap = QPixmap::fromImage(image);
    else
        m_pixmap = QPixmap();

    update();
}

void ImageViewWidget::paintEvent(QPaintEvent *)
{
    if (m_pixmap.isNull())
        return;

    QPainter painter(this);
    QPixmap scaled = m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (width()  - scaled.width())  / 2;
    int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
}

void ImageViewWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit closeRequested();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_PageDown)
    {
        emit nextRequested();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_PageUp)
    {
        emit previousRequested();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}
