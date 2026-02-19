#include "mainwindow.h"
#include "imagemodel.h"

#include <QFileSystemModel>
#include <QTreeView>
#include <QListView>
#include <QLabel>
#include <QSplitter>
#include <QDir>
#include <QImageReader>
#include <QHeaderView>
#include <QScrollBar>
#include <QTimer>
#include <QEvent>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupConnections();
}

void MainWindow::setupUi()
{
    // Folder model (left pane)
    m_dirModel = new QFileSystemModel(this);
    m_dirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    m_dirModel->setRootPath(QDir::homePath());

    m_treeView = new QTreeView;
    m_treeView->setModel(m_dirModel);
    m_treeView->setRootIndex(m_dirModel->index(QDir::homePath()));
    m_treeView->header()->hide();
    m_treeView->setColumnHidden(1, true);
    m_treeView->setColumnHidden(2, true);
    m_treeView->setColumnHidden(3, true);

    // Thumbnail model (center pane)
    m_imageModel = new ImageModel(this);

    m_listView = new QListView;
    m_listView->setModel(m_imageModel);
    m_listView->setViewMode(QListView::IconMode);
    m_listView->setIconSize(QSize(128, 128));
    m_listView->setGridSize(QSize(148, 168));
    m_listView->setResizeMode(QListView::Adjust);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setSpacing(4);
    m_listView->setWordWrap(true);
    m_listView->setTextElideMode(Qt::ElideRight);

    // Preview pane (right)
    m_previewLabel = new QLabel;
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumWidth(300);

    // Layout
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal);
    rightSplitter->addWidget(m_listView);
    rightSplitter->addWidget(m_previewLabel);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 2);

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(m_treeView);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setStretchFactor(1, 1);

    setCentralWidget(mainSplitter);
}

void MainWindow::setupConnections()
{
    connect(m_treeView, &QTreeView::clicked, this,
            [this](const QModelIndex &index)
            {
                QString path = m_dirModel->filePath(index);
                m_imageModel->setDirectory(path);
                // Defer thumbnail load so the view has time to lay out items
                QTimer::singleShot(0, this, &MainWindow::loadVisibleThumbnails);
            });

    connect(m_listView, &QListView::clicked, this,
            [this](const QModelIndex &index)
            {
                QString path = m_imageModel->filePath(index);

                QImageReader reader(path);
                reader.setAutoTransform(true);
                QImage image = reader.read();

                m_previewLabel->setPixmap(
                    QPixmap::fromImage(image).scaled(
                        m_previewLabel->size(),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation));
            });

    connect(m_listView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this]()
            { loadVisibleThumbnails(); });

    // Also catch resize — visible range changes when the viewport is resized
    m_listView->viewport()->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_listView->viewport() && event->type() == QEvent::Resize)
        QTimer::singleShot(0, this, &MainWindow::loadVisibleThumbnails);

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::loadVisibleThumbnails()
{
    QModelIndex topLeft = m_listView->indexAt(QPoint(0, 0));
    QModelIndex bottomRight = m_listView->indexAt(
        QPoint(m_listView->viewport()->width() - 1,
               m_listView->viewport()->height() - 1));

    int first = topLeft.isValid() ? topLeft.row() : 0;
    int last = bottomRight.isValid() ? bottomRight.row()
                                     : m_imageModel->rowCount() - 1;

    if (last < 0)
        return;

    std::cout << "loadVisibleThumbnails: " << first << " - " << last << "\n";
    m_imageModel->requestThumbnails(first, last);
}
