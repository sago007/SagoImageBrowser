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
    m_listView->setResizeMode(QListView::Adjust);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setSpacing(10);

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
}
