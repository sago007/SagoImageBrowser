#include "imageviewwidget.h"

#include <QPainter>
#include <QKeyEvent>
#include <QImageReader>
#include <algorithm>

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

    m_zoomMode = FitToScreen;
    m_zoomFactor = 1.0;
    update();
}

void ImageViewWidget::paintEvent(QPaintEvent *)
{
    if (m_pixmap.isNull())
        return;

    QPainter painter(this);
    QPixmap drawn;

    switch (m_zoomMode) {
    case FitToScreen:
        drawn = m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        break;
    case OriginalSize:
        drawn = m_pixmap;
        break;
    case CustomZoom: {
        QSize target(static_cast<int>(m_pixmap.width() * m_zoomFactor),
                     static_cast<int>(m_pixmap.height() * m_zoomFactor));
        drawn = m_pixmap.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        break;
    }
    }

    int x = (width()  - drawn.width())  / 2;
    int y = (height() - drawn.height()) / 2;
    painter.drawPixmap(x, y, drawn);
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

    if (event->key() == Qt::Key_Slash)
    {
        zoomOriginal();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Asterisk)
    {
        zoomFitToScreen();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Plus)
    {
        zoomIn();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Minus)
    {
        zoomOut();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void ImageViewWidget::zoomIn()
{
    if (m_zoomMode == FitToScreen) {
        // Calculate the current effective scale so zooming feels continuous
        double sx = static_cast<double>(width()) / m_pixmap.width();
        double sy = static_cast<double>(height()) / m_pixmap.height();
        m_zoomFactor = std::min(sx, sy);
    }
    m_zoomFactor *= 1.25;
    m_zoomMode = CustomZoom;
    update();
}

void ImageViewWidget::zoomOut()
{
    if (m_zoomMode == FitToScreen) {
        double sx = static_cast<double>(width()) / m_pixmap.width();
        double sy = static_cast<double>(height()) / m_pixmap.height();
        m_zoomFactor = std::min(sx, sy);
    }
    m_zoomFactor /= 1.25;
    if (m_zoomFactor < 0.01)
        m_zoomFactor = 0.01;
    m_zoomMode = CustomZoom;
    update();
}

void ImageViewWidget::zoomOriginal()
{
    m_zoomMode = OriginalSize;
    m_zoomFactor = 1.0;
    update();
}

void ImageViewWidget::zoomFitToScreen()
{
    m_zoomMode = FitToScreen;
    update();
}
