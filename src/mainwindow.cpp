#include "mainwindow.h"
#include "imagemodel.h"
#include "imageviewwidget.h"
#include "preferencesdialog.h"

#include <QFileSystemModel>
#include <QTreeView>
#include <QListView>
#include <QLabel>
#include <QStackedWidget>
#include <QDir>
#include <QListWidget>
#include <QStandardPaths>
#include <QImageReader>
#include <QHeaderView>
#include <QScrollBar>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QItemSelectionModel>
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenuBar();
    setupConnections();
    loadPreferences();
}

void MainWindow::setupUi()
{
    // Folder model (left pane)
    m_dirModel = new QFileSystemModel(this);
    m_dirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    m_dirModel->setRootPath(QDir::homePath());

    // Root chooser list
    m_rootList = new QListWidget;
    m_rootList->setSelectionMode(QAbstractItemView::SingleSelection);

    auto addRootItem = [this](const QString &label, const QString &path) {
        QListWidgetItem *item = new QListWidgetItem(label, m_rootList);
        item->setData(Qt::UserRole, path);
    };
    addRootItem("Home", QDir::homePath());
    addRootItem("Pictures", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    addRootItem("File System", QDir::rootPath());
    m_rootList->setCurrentRow(0);

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
    m_previewLabel->setMinimumWidth(200);

    // Single image view
    m_imageView = new ImageViewWidget;

    // Stack to switch between browser and single image
    m_stack = new QStackedWidget;
    m_stack->addWidget(m_listView);
    m_stack->addWidget(m_imageView);
    m_browserPage = m_listView;

    setCentralWidget(m_stack);

    // Dock widgets — can be dragged, floated and stacked by the user
    m_rootDock = new QDockWidget(tr("Roots"), this);
    m_rootDock->setObjectName("RootDock");
    m_rootDock->setWidget(m_rootList);
    addDockWidget(Qt::LeftDockWidgetArea, m_rootDock);

    m_folderDock = new QDockWidget(tr("Folders"), this);
    m_folderDock->setObjectName("FolderDock");
    m_folderDock->setWidget(m_treeView);
    addDockWidget(Qt::LeftDockWidgetArea, m_folderDock);
    splitDockWidget(m_rootDock, m_folderDock, Qt::Vertical);

    m_previewDock = new QDockWidget(tr("Preview"), this);
    m_previewDock->setObjectName("PreviewDock");
    m_previewDock->setWidget(m_previewLabel);
    addDockWidget(Qt::RightDockWidgetArea, m_previewDock);
}

void MainWindow::setupMenuBar()
{
    m_menuBar = menuBar();
    m_menuBar->setStyleSheet(
        "QMenuBar { padding: 1px 2px; }"
        "QMenuBar::item { padding: 2px 6px; }"
    );
    QMenu *viewMenu = m_menuBar->addMenu("&View");
    viewMenu->addAction(m_rootDock->toggleViewAction());
    viewMenu->addAction(m_folderDock->toggleViewAction());
    viewMenu->addAction(m_previewDock->toggleViewAction());

    QMenu *editMenu = m_menuBar->addMenu("&Edit");
    QAction *preferencesAction = editMenu->addAction("&Preferences");
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::onPreferencesTriggered);
}

void MainWindow::navigateToFolder(const QString &path)
{
    m_previousFolderPath = m_imageModel->currentDirectory();
    m_imageModel->setDirectory(path);
    QModelIndex dirIdx = m_dirModel->index(path);
    if (dirIdx.isValid())
        m_treeView->setCurrentIndex(dirIdx);
    QTimer::singleShot(0, this, [this]() {
        selectPreviousFolderIfExists();
        loadVisibleThumbnails();
    });
}

void MainWindow::selectPreviousFolderIfExists()
{
    if (m_previousFolderPath.isEmpty())
        return;

    // Find the previous folder in the current directory's contents
    for (int i = 0; i < m_imageModel->rowCount(); ++i) {
        QModelIndex idx = m_imageModel->index(i);
        if (m_imageModel->isFolder(idx)) {
            QString folderPath = m_imageModel->filePath(idx);
            if (folderPath == m_previousFolderPath) {
                m_listView->setCurrentIndex(idx);
                m_listView->scrollTo(idx);
                return;
            }
        }
    }
}

void MainWindow::setupConnections()
{
    connect(m_rootList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (current == nullptr)
                    return;

                QString rootPath = current->data(Qt::UserRole).toString();
                m_dirModel->setRootPath(rootPath);
                m_treeView->setRootIndex(m_dirModel->index(rootPath));
            });

    connect(m_treeView, &QTreeView::clicked, this,
            [this](const QModelIndex &index)
            {
                QString path = m_dirModel->filePath(index);
                navigateToFolder(path);
            });

    connect(m_listView, &QListView::clicked, this,
            [this](const QModelIndex &index) { showPreview(index); });

    connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) { showPreview(current); });

    connect(m_listView, &QListView::doubleClicked, this,
            [this](const QModelIndex &index) {
                if (m_imageModel->isFolder(index)) {
                    QString path = m_imageModel->filePath(index);
                    navigateToFolder(path);
                } else {
                    enterSingleImageMode(index);
                }
            });

    connect(m_imageView, &ImageViewWidget::closeRequested,
            this, &MainWindow::leaveSingleImageMode);

    connect(m_imageView, &ImageViewWidget::nextRequested, this, [this]() {
        QModelIndex cur = m_listView->currentIndex();
        int next = cur.isValid() ? cur.row() + 1 : 0;
        while (next < m_imageModel->rowCount() && m_imageModel->isFolder(m_imageModel->index(next)))
            ++next;
        if (next < m_imageModel->rowCount())
        {
            QModelIndex idx = m_imageModel->index(next);
            m_listView->setCurrentIndex(idx);
            enterSingleImageMode(idx);
        }
    });

    connect(m_imageView, &ImageViewWidget::previousRequested, this, [this]() {
        QModelIndex cur = m_listView->currentIndex();
        int prev = cur.isValid() ? cur.row() - 1 : 0;
        while (prev >= 0 && m_imageModel->isFolder(m_imageModel->index(prev)))
            --prev;
        if (prev >= 0)
        {
            QModelIndex idx = m_imageModel->index(prev);
            m_listView->setCurrentIndex(idx);
            enterSingleImageMode(idx);
        }
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

void MainWindow::showPreview(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString path = m_imageModel->filePath(index);

    QImageReader reader(path);
    reader.setAutoTransform(true);
    // Scale during decoding for the preview to avoid the 128 MB allocation limit
    QSize fullSize = reader.size();
    if (fullSize.isValid()) {
        QSize target = fullSize.scaled(m_previewLabel->size(), Qt::KeepAspectRatio);
        reader.setScaledSize(target);
    }
    QImage image = reader.read();

    m_previewLabel->setPixmap(
        QPixmap::fromImage(image).scaled(
            m_previewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}

void MainWindow::enterSingleImageMode(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Only save state and hide chrome when transitioning from browse mode
    if (m_stack->currentWidget() != m_imageView) {
        m_rootDockWasVisible = m_rootDock->isVisible();
        m_folderDockWasVisible = m_folderDock->isVisible();
        m_previewDockWasVisible = m_previewDock->isVisible();
        m_rootDock->hide();
        m_folderDock->hide();
        m_previewDock->hide();
        m_menuBar->hide();
    }

    m_imageView->setBackgroundColor(m_backgroundColorPreference);
    QString path = m_imageModel->filePath(index);
    m_imageView->setNeighborPaths(computeNeighborPaths(index));
    m_imageView->setImage(path);
    m_stack->setCurrentWidget(m_imageView);
    m_imageView->setFocus();

    if (m_imageViewWasFullScreen) {
        setFullScreenMode(true);
        m_imageViewWasFullScreen = false;
    }
}

void MainWindow::leaveSingleImageMode()
{
    if (isFullScreen()) {
        m_imageViewWasFullScreen = true;
        setFullScreenMode(false);
    }
    m_imageView->clearCache();
    m_stack->setCurrentWidget(m_browserPage);
    m_menuBar->show();
    if (m_rootDockWasVisible)
        m_rootDock->show();
    if (m_folderDockWasVisible)
        m_folderDock->show();
    if (m_previewDockWasVisible)
        m_previewDock->show();
    m_listView->setFocus();
}

QStringList MainWindow::computeNeighborPaths(const QModelIndex &index) const
{
    QStringList paths;
    int row = index.row();

    // Collect up to CacheKeepBehind non-folder images before current
    int count = 0;
    for (int i = row - 1; i >= 0 && count < ImageViewWidget::CacheKeepBehind; --i) {
        QModelIndex idx = m_imageModel->index(i);
        if (!m_imageModel->isFolder(idx)) {
            paths.prepend(m_imageModel->filePath(idx));
            ++count;
        }
    }

    // Collect up to CacheReadAhead non-folder images after current
    count = 0;
    for (int i = row + 1; i < m_imageModel->rowCount() && count < ImageViewWidget::CacheReadAhead; ++i) {
        QModelIndex idx = m_imageModel->index(i);
        if (!m_imageModel->isFolder(idx)) {
            paths.append(m_imageModel->filePath(idx));
            ++count;
        }
    }

    return paths;
}

void MainWindow::setFullScreenMode(bool fullScreen)
{
    if (fullScreen == isFullScreen())
        return;

    if (fullScreen) {
        m_wasMaximized = isMaximized();
        m_menuBar->hide();
        showFullScreen();
    } else {
        // Only restore menu bar when returning to browse mode
        if (m_stack->currentWidget() != m_imageView)
            m_menuBar->show();
        if (m_wasMaximized)
            showMaximized();
        else
            showNormal();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F11 && m_stack->currentWidget() == m_imageView)
    {
        setFullScreenMode(!isFullScreen());
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Return && m_stack->currentWidget() == m_browserPage)
    {
        QModelIndex current = m_listView->currentIndex();
        if (current.isValid())
        {
            if (m_imageModel->isFolder(current)) {
                QString path = m_imageModel->filePath(current);
                navigateToFolder(path);
            } else {
                enterSingleImageMode(current);
            }
            event->accept();
            return;
        }
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_stack->currentWidget() == m_imageView)
    {
        leaveSingleImageMode();
        event->ignore();
        return;
    }

    savePreferences();
    QMainWindow::closeEvent(event);
}

void MainWindow::applyDockLocking(bool lock)
{
    const QDockWidget::DockWidgetFeatures movableFeatures =
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable;
    const QDockWidget::DockWidgetFeatures baseFeatures =
        QDockWidget::DockWidgetClosable;

    QDockWidget::DockWidgetFeatures features = lock ? baseFeatures
                                                    : (baseFeatures | movableFeatures);
    m_rootDock->setFeatures(features);
    m_folderDock->setFeatures(features);
    m_previewDock->setFeatures(features);
}

void MainWindow::loadPreferences()
{
    QSettings settings("SagoImageBrowser", "SagoImageBrowser");
    m_backgroundColorPreference = settings.value("backgroundColor", "black").toString();
    m_imageView->setBackgroundColor(m_backgroundColorPreference);
    m_lockDockingPreference = settings.value("lockDocking", false).toBool();
    applyDockLocking(m_lockDockingPreference);
    if (settings.contains("windowGeometry"))
        restoreGeometry(settings.value("windowGeometry").toByteArray());
    if (settings.contains("windowState"))
        restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::savePreferences()
{
    QSettings settings("SagoImageBrowser", "SagoImageBrowser");
    settings.setValue("backgroundColor", m_backgroundColorPreference);
    settings.setValue("lockDocking", m_lockDockingPreference);
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::onPreferencesTriggered()
{
    PreferencesDialog dialog(this);
    dialog.setBackgroundColor(m_backgroundColorPreference);
    dialog.setLockDocking(m_lockDockingPreference);

    bool shouldResetLayout = false;
    connect(&dialog, &PreferencesDialog::resetLayoutRequested, this, [&shouldResetLayout]() {
        shouldResetLayout = true;
    });

    if (dialog.exec() == QDialog::Accepted)
    {
        m_backgroundColorPreference = dialog.getBackgroundColor();
        m_imageView->setBackgroundColor(m_backgroundColorPreference);
        m_lockDockingPreference = dialog.getLockDocking();
        applyDockLocking(m_lockDockingPreference);
        savePreferences();
    }
    
    // Apply layout reset immediately after dialog closes if requested
    if (shouldResetLayout) {
        resetLayoutToDefault();
    }
}

void MainWindow::resetLayoutToDefault()
{
    QSettings settings("SagoImageBrowser", "SagoImageBrowser");
    settings.remove("windowGeometry");
    settings.remove("windowState");
    
    // Hide and detach docks completely
    m_rootDock->hide();
    m_folderDock->hide();
    m_previewDock->hide();
    
    m_rootDock->setFloating(false);
    m_folderDock->setFloating(false);
    m_previewDock->setFloating(false);
    
    removeDockWidget(m_rootDock);
    removeDockWidget(m_folderDock);
    removeDockWidget(m_previewDock);
    
    // Re-add in default positions
    addDockWidget(Qt::LeftDockWidgetArea, m_rootDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_folderDock);
    addDockWidget(Qt::RightDockWidgetArea, m_previewDock);
    splitDockWidget(m_rootDock, m_folderDock, Qt::Vertical);
    
    // Show all docks
    m_rootDock->show();
    m_folderDock->show();
    m_previewDock->show();
}
