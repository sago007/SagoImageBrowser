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

#include "mainwindow.h"
#include "imagemodel.h"
#include "imageviewwidget.h"
#include "preferencesdialog.h"
#include "shortcutmanager.h"
#include "thumbnailcache.h"
#include "exifreader.h"
#include "fsdirmodel.h"
#include "pathcompletermodel.h"
#include "thumbnaildelegate.h"

#include <QTreeView>
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
#include <QItemSelectionModel>
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QTableWidget>
#include <QDesktopServices>
#include <QUrl>
#include <QDialog>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCompleter>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QVBoxLayout>
#include <iostream>

#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
ThumbnailCache::Size thumbnailSizeFromString(const QString &s)
{
    return (s == "large") ? ThumbnailCache::Size::Large
                          : ThumbnailCache::Size::Normal;
}

QString displayPath(const QByteArray &path)
{
    return QString::fromLocal8Bit(path.constData(), path.size());
}

QString filenameFromPath(const QByteArray &path)
{
    const std::filesystem::path fsPath(path.toStdString());
    const std::string nativeName = fsPath.filename().native();
    return QString::fromLocal8Bit(nativeName.data(),
                                  static_cast<qsizetype>(nativeName.size()));
}
} // namespace

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
    // Folder model (left pane) — backed by std::filesystem so directories with
    // non-UTF-8 filenames are listed correctly.
    m_dirModel = new FsDirModel(this);
    m_dirModel->setRootPath(QDir::homePath());

    // Root chooser list
    m_rootList = new QListWidget;
    m_rootList->setSelectionMode(QAbstractItemView::SingleSelection);

    m_treeView = new QTreeView;
    m_treeView->setModel(m_dirModel);
    m_treeView->setRootIndex(m_dirModel->rootIndex());
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
    m_listView->setItemDelegate(new ThumbnailDelegate(this));
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);

    // Preview pane (right)
    m_previewLabel = new QLabel;
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumWidth(200);

    // Single image view
    m_imageView = new ImageViewWidget;
    m_imageView->setContextMenuPolicy(Qt::CustomContextMenu);

    // Path bar: editable absolute path with ajax-style folder completion and a Go button
    m_pathEdit = new QLineEdit;
    m_goButton = new QPushButton(tr("Go"));
    m_prevFolderButton = new QPushButton(tr("Prev"));
    m_nextFolderButton = new QPushButton(tr("Next"));
    m_pathCompleterModel = new PathCompleterModel(this);
    m_pathCompleter = new QCompleter(m_pathCompleterModel, this);
    m_pathCompleter->setCaseSensitivity(Qt::CaseSensitive);
    m_pathCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_pathCompleter->setFilterMode(Qt::MatchStartsWith);
    m_pathEdit->setCompleter(m_pathCompleter);

    auto *pathRow = new QHBoxLayout;
    pathRow->setContentsMargins(4, 4, 4, 0);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(m_goButton);
    pathRow->addWidget(m_prevFolderButton);
    pathRow->addWidget(m_nextFolderButton);

    auto *browserPage = new QWidget;
    auto *vbox = new QVBoxLayout(browserPage);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    vbox->addLayout(pathRow);
    vbox->addWidget(m_listView, 1);

    // Stack to switch between browser and single image
    m_stack = new QStackedWidget;
    m_stack->addWidget(browserPage);
    m_stack->addWidget(m_imageView);
    m_browserPage = browserPage;

    setCentralWidget(m_stack);

    updatePathField(QFile::encodeName(QDir::homePath()));

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

    // EXIF info dock — placed below the preview dock by default
    m_exifTable = new QTableWidget(0, 2);
    m_exifTable->setHorizontalHeaderLabels({tr("Field"), tr("Value")});
    m_exifTable->horizontalHeader()->setStretchLastSection(true);
    m_exifTable->verticalHeader()->hide();
    m_exifTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_exifTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_exifTable->setShowGrid(false);
    m_exifTable->setAlternatingRowColors(true);
    m_exifTable->setMinimumWidth(200);

    m_exifDock = new QDockWidget(tr("EXIF Info"), this);
    m_exifDock->setObjectName("ExifDock");
    m_exifDock->setWidget(m_exifTable);
    addDockWidget(Qt::RightDockWidgetArea, m_exifDock);
    splitDockWidget(m_previewDock, m_exifDock, Qt::Vertical);
}

void MainWindow::setupMenuBar()
{
    m_menuBar = menuBar();
    m_menuBar->setStyleSheet(
        "QMenuBar { padding: 1px 2px; }"
        "QMenuBar::item { padding: 2px 6px; }"
    );
    QMenu *viewMenu = m_menuBar->addMenu(tr("&View"));
    viewMenu->addAction(m_rootDock->toggleViewAction());
    viewMenu->addAction(m_folderDock->toggleViewAction());
    viewMenu->addAction(m_previewDock->toggleViewAction());
    viewMenu->addAction(m_exifDock->toggleViewAction());

    QMenu *editMenu = m_menuBar->addMenu(tr("&Edit"));
    QAction *preferencesAction = editMenu->addAction(tr("&Preferences"));
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::onPreferencesTriggered);
}

void MainWindow::navigateToFolder(const QString &path)
{
    navigateToFolder(QFile::encodeName(path));
}

QByteArray MainWindow::siblingFolder(int delta) const
{
    QByteArray currentDir = m_imageModel->currentDirectory();
    if (currentDir.isEmpty())
        return {};

    namespace fs = std::filesystem;
    fs::path current(currentDir.toStdString());
    fs::path parent = current.parent_path();
    if (parent.empty() || parent == current)
        return {};

    QByteArray parentPath = QByteArray::fromStdString(parent.native());
    QModelIndex parentIdx = m_dirModel->indexForPath(parentPath);
    if (!parentIdx.isValid())
        return {};

    int count = m_dirModel->rowCount(parentIdx);
    int currentRow = -1;
    for (int i = 0; i < count; ++i) {
        QModelIndex idx = m_dirModel->index(i, 0, parentIdx);
        if (m_dirModel->filePath(idx) == currentDir) {
            currentRow = i;
            break;
        }
    }

    if (currentRow < 0)
        return {};

    int targetRow = currentRow + delta;
    if (targetRow < 0 || targetRow >= count)
        return {};

    QModelIndex targetIdx = m_dirModel->index(targetRow, 0, parentIdx);
    return m_dirModel->filePath(targetIdx);
}

void MainWindow::navigateToPrevFolder()
{
    QByteArray path = siblingFolder(-1);
    if (!path.isEmpty())
        navigateToFolder(path);
}

void MainWindow::navigateToNextFolder()
{
    QByteArray path = siblingFolder(+1);
    if (!path.isEmpty())
        navigateToFolder(path);
}

void MainWindow::navigateToFolder(const QByteArray &path)
{
    m_previousFolderPath = m_imageModel->currentDirectory();
    m_imageModel->setDirectory(path);
    QModelIndex dirIdx = m_dirModel->indexForPath(path);
    if (dirIdx.isValid())
        m_treeView->setCurrentIndex(dirIdx);
    updatePathField(path);
    QTimer::singleShot(0, this, [this]() {
        selectPreviousFolderIfExists();
        loadVisibleThumbnails();
    });
}

void MainWindow::updatePathField(const QByteArray &path)
{
    if (!m_pathEdit)
        return;
    const QString text = QFile::decodeName(path);
    if (m_pathEdit->text() != text)
        m_pathEdit->setText(text);
}

void MainWindow::onPathEntered()
{
    const QString text = m_pathEdit->text().trimmed();
    if (text.isEmpty())
        return;
    openPath(text);
}

void MainWindow::openPath(const QString &path)
{
    QFileInfo info(path);
    if (!info.exists())
        return;

    if (info.isDir()) {
        navigateToFolder(QFile::encodeName(info.absoluteFilePath()));
    } else if (info.isFile()) {
        navigateToFolder(QFile::encodeName(info.absolutePath()));

        // ImageModel::setDirectory is synchronous, so items are ready now.
        // Find the file in the model and open it.
        QByteArray target = QFile::encodeName(info.absoluteFilePath());
        for (int i = 0; i < m_imageModel->rowCount(); ++i) {
            QModelIndex idx = m_imageModel->index(i);
            if (m_imageModel->filePath(idx) == target) {
                m_listView->setCurrentIndex(idx);
                m_listView->scrollTo(idx);
                showPreview(idx);
                if (!m_imageModel->isFolder(idx))
                    enterSingleImageMode(idx);
                break;
            }
        }
    }
}

void MainWindow::selectPreviousFolderIfExists()
{
    if (m_previousFolderPath.isEmpty())
        return;

    // Find the previous folder in the current directory's contents
    for (int i = 0; i < m_imageModel->rowCount(); ++i) {
        QModelIndex idx = m_imageModel->index(i);
        if (m_imageModel->isFolder(idx)) {
            QByteArray folderPath = m_imageModel->filePath(idx);
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
    connect(m_pathEdit, &QLineEdit::returnPressed,
            this, &MainWindow::onPathEntered);
    connect(m_goButton, &QPushButton::clicked,
            this, &MainWindow::onPathEntered);
    connect(m_prevFolderButton, &QPushButton::clicked,
            this, &MainWindow::navigateToPrevFolder);
    connect(m_nextFolderButton, &QPushButton::clicked,
            this, &MainWindow::navigateToNextFolder);
    connect(m_pathEdit, &QLineEdit::textEdited,
            m_pathCompleterModel, &PathCompleterModel::setPrefix);

    connect(m_rootList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (current == nullptr)
                    return;

                QString rootPath = current->data(Qt::UserRole).toString();
                m_dirModel->setRootPath(rootPath);
                m_treeView->setRootIndex(m_dirModel->rootIndex());
            });

    connect(m_treeView, &QTreeView::clicked, this,
            [this](const QModelIndex &index)
            {
                QByteArray path = m_dirModel->filePath(index);
                navigateToFolder(path);
            });

    connect(m_listView, &QListView::clicked, this,
            [this](const QModelIndex &index) { showPreview(index); });

    connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) { showPreview(current); });

    connect(m_listView, &QListView::doubleClicked, this,
            [this](const QModelIndex &index) {
                if (m_imageModel->isFolder(index)) {
                    QByteArray path = m_imageModel->filePath(index);
                    navigateToFolder(path);
                } else {
                    enterSingleImageMode(index);
                }
            });

    connect(m_listView, &QListView::customContextMenuRequested,
            this, &MainWindow::showImageContextMenu);

    connect(m_imageView, &ImageViewWidget::customContextMenuRequested,
            this, &MainWindow::showImageViewContextMenu);

    connect(m_imageView, &ImageViewWidget::closeRequested,
            this, &MainWindow::leaveSingleImageMode);
    connect(m_imageView, &ImageViewWidget::editCaptionRequested,
        this, &MainWindow::onEditCaption);
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

    // The folder list and tree views consume Ctrl+Left/Ctrl+Right for their own
    // item navigation, so those keys never reach MainWindow::keyPressEvent.
    // Filter them here so PrevFolder/NextFolder shortcuts work while a view has focus.
    m_listView->installEventFilter(this);
    m_treeView->installEventFilter(this);

    connect(m_exifTable, &QTableWidget::cellClicked, this, [this](int row, int column) {
        if (column == 1) {
            QTableWidgetItem *fieldItem = m_exifTable->item(row, 0);
            if (fieldItem && fieldItem->text() == "OpenStreetMap") {
                QTableWidgetItem *valueItem = m_exifTable->item(row, 1);
                if (valueItem && !valueItem->text().isEmpty()) {
                    QDesktopServices::openUrl(QUrl(valueItem->text()));
                }
            }
        }
    });
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_listView->viewport() && event->type() == QEvent::Resize)
        QTimer::singleShot(0, this, &MainWindow::loadVisibleThumbnails);

    if ((obj == m_listView || obj == m_treeView)
        && event->type() == QEvent::KeyPress
        && m_stack->currentWidget() == m_browserPage)
    {
        const ShortcutManager &sc = ShortcutManager::instance();
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (sc.matches(keyEvent, ShortcutManager::PrevFolder)) {
            navigateToPrevFolder();
            return true;
        }
        if (sc.matches(keyEvent, ShortcutManager::NextFolder)) {
            navigateToNextFolder();
            return true;
        }
        if (obj == m_listView
            && sc.matches(keyEvent, ShortcutManager::MoveToTrash)) {
            moveCurrentImageToTrash();
            return true;
        }
    }

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

    QByteArray path = m_imageModel->filePath(index);

    int fd = ::open(path.constData(), O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        QFile file;
        if (file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
            QImageReader reader(&file);
            reader.setAutoTransform(true);
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
    }

    updateExifInfo(path);
}

void MainWindow::updateExifInfo(const QByteArray &path)
{
    // Check if this is a folder by attempting stat
    struct ::stat st{};
    if (::stat(path.constData(), &st) != 0 || S_ISDIR(st.st_mode)) {
        m_exifTable->setRowCount(0);
        return;
    }
    const ExifData data = ExifReader::read(path);
    const auto fields = data.toList();
    m_exifTable->setRowCount(fields.size());
    for (int i = 0; i < fields.size(); ++i) {
        m_exifTable->setItem(i, 0, new QTableWidgetItem(fields[i].first));
        auto *item = new QTableWidgetItem(fields[i].second);
        if (fields[i].first == "OpenStreetMap") {
            item->setForeground(Qt::blue);
            QFont font = item->font();
            font.setUnderline(true);
            item->setFont(font);
            item->setToolTip(tr("Click to open in browser"));
        }
        m_exifTable->setItem(i, 1, item);
    }
    m_exifTable->resizeColumnToContents(0);
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
        m_exifDockWasVisible = m_exifDock->isVisible();
        m_rootDock->hide();
        m_folderDock->hide();
        m_previewDock->hide();
        m_exifDock->hide();
        m_menuBar->hide();
    }

    m_imageView->setBackgroundColor(m_backgroundColorPreference);
    QByteArray path = m_imageModel->filePath(index);
    m_imageView->setNeighborPaths(computeNeighborPaths(index));
    m_imageView->setImage(path);
    m_imageView->setExifData(ExifReader::read(path));
    updateExifInfo(path);
    m_stack->setCurrentWidget(m_imageView);
    m_imageView->setFocus();

    if (m_imageViewWasFullScreen) {
        setFullScreenMode(true);
        m_imageViewWasFullScreen = false;
    }
}

void MainWindow::onEditCaption()
{
    QByteArray path;
    if (m_stack->currentWidget() == m_imageView) {
        path = m_imageView->currentPath();
    } else {
        const QModelIndex index = m_listView->currentIndex();
        if (index.isValid() && !m_imageModel->isFolder(index))
            path = m_imageModel->filePath(index);
    }

    if (path.isEmpty())
        return;

    if (::access(path.constData(), W_OK) != 0) {
        QMessageBox::warning(this, tr("Cannot Edit Caption"),
            tr("The file is write-protected and cannot be edited.\n\n%1")
                .arg(displayPath(path)));
        return;
    }

    ExifData data = ExifReader::read(path);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Edit Caption"));
    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(tr("Caption-Abstract:"), &dlg));
    auto *edit = new QPlainTextEdit(&dlg);
    edit->setPlainText(data.captionAbstract);
    edit->setMinimumWidth(400);
    layout->addWidget(edit);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString newCaption = edit->toPlainText();
    if (ExifReader::saveCaption(path, newCaption, data)) {
        ExifData updated = ExifReader::read(path);
        if (m_imageView->currentPath() == path)
            m_imageView->setExifData(updated);
        updateExifInfo(path);
    }
}

void MainWindow::showImageContextMenu(const QPoint &pos)
{
    const QModelIndex index = m_listView->indexAt(pos);
    if (!index.isValid() || m_imageModel->isFolder(index))
        return;

    m_listView->setCurrentIndex(index);
    showPreview(index);

    const QByteArray path = m_imageModel->filePath(index);
    executeContextMenu(m_listView->viewport()->mapToGlobal(pos), path, index);
}

void MainWindow::showImageViewContextMenu(const QPoint &pos)
{
    const QByteArray path = m_imageView->currentPath();
    if (path.isEmpty())
        return;

    const QModelIndex index = m_listView->currentIndex();
    executeContextMenu(m_imageView->mapToGlobal(pos), path, index);
}

void MainWindow::executeContextMenu(const QPoint &globalPos, const QByteArray &path, const QModelIndex &index)
{
    QMenu menu(this);
    QAction *copyFilenameAction = menu.addAction(tr("Copy filename"));
    QAction *copyFullPathAction = menu.addAction(tr("Copy full path"));
    menu.addSeparator();

    QFileInfo info(QFile::decodeName(path));
    QString suffix = info.suffix().toLower();
    bool isJpg = (suffix == "jpg" || suffix == "jpeg");

    QAction *rotateLeftAction = nullptr;
    QAction *rotateRightAction = nullptr;
    if (isJpg) {
        rotateLeftAction = menu.addAction(tr("Rotate Left"));
        rotateRightAction = menu.addAction(tr("Rotate Right"));
        menu.addSeparator();
    }

    QAction *editDescriptionAction = menu.addAction(tr("Edit description"));
    QAction *moveToTrashAction = menu.addAction(tr("Move to trash"));
    moveToTrashAction->setShortcut(
        ShortcutManager::instance().shortcut(ShortcutManager::MoveToTrash));

    QAction *chosen = menu.exec(globalPos);
    if (chosen == copyFilenameAction) {
        QGuiApplication::clipboard()->setText(filenameFromPath(path));
    } else if (chosen == copyFullPathAction) {
        QGuiApplication::clipboard()->setText(displayPath(path));
    } else if (rotateLeftAction && chosen == rotateLeftAction) {
        rotateImage(index, false);
    } else if (rotateRightAction && chosen == rotateRightAction) {
        rotateImage(index, true);
    } else if (chosen == editDescriptionAction) {
        onEditCaption();
    } else if (chosen == moveToTrashAction) {
        moveImageToTrash(path);
    }
}

void MainWindow::rotateImage(const QModelIndex &index, bool clockwise)
{
    if (!index.isValid() || m_imageModel->isFolder(index))
        return;

    const QByteArray path = m_imageModel->filePath(index);

    if (::access(path.constData(), W_OK) != 0) {
        QMessageBox::warning(this, tr("Cannot Rotate Image"),
            tr("The file is write-protected and cannot be rotated.\n\n%1")
                .arg(displayPath(path)));
        return;
    }

    if (!ExifReader::rotate(path, clockwise)) {
        QMessageBox::warning(this, tr("Cannot Rotate Image"),
            tr("The image orientation could not be changed. The format may not "
               "support EXIF orientation.\n\n%1")
                .arg(displayPath(path)));
        return;
    }

    m_imageModel->refreshThumbnail(index);
    showPreview(index);

    if (m_imageView->currentPath() == path) {
        m_imageView->invalidateCache(path);
        m_imageView->setImage(path);
        m_imageView->setExifData(ExifReader::read(path));
    }
}

void MainWindow::moveImageToTrash(const QByteArray &path)
{
    if (path.isEmpty())
        return;

    const QString fileName = filenameFromPath(path);
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("Move to Trash"),
        tr("Move \"%1\" to the trash?").arg(fileName),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;

    if (!QFile::moveToTrash(displayPath(path))) {
        QMessageBox::warning(this, tr("Cannot Move to Trash"),
            tr("The file could not be moved to the trash.\n\n%1")
                .arg(displayPath(path)));
        return;
    }

    const QByteArray currentDir = m_imageModel->currentDirectory();
    m_previewLabel->clear();
    m_exifTable->setRowCount(0);
    m_imageModel->setDirectory(currentDir);
    QTimer::singleShot(0, this, &MainWindow::loadVisibleThumbnails);
}

void MainWindow::moveCurrentImageToTrash()
{
    const QModelIndex index = m_listView->currentIndex();
    if (!index.isValid() || m_imageModel->isFolder(index))
        return;

    moveImageToTrash(m_imageModel->filePath(index));
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
    if (m_exifDockWasVisible)
        m_exifDock->show();
    m_listView->setFocus();
}

QList<QByteArray> MainWindow::computeNeighborPaths(const QModelIndex &index) const
{
    QList<QByteArray> paths;
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
    // THREADING: No race condition with F11 fullscreen toggle.  All cross-thread
    // signal deliveries (thumbnailReady via explicit QueuedConnection,
    // onImageLoaded via AutoConnection resolving to QueuedConnection) execute on
    // the main thread's event loop.  The event loop processes events one at a
    // time, so this handler and any signal slot can never interleave.  All widget
    // state (m_wasMaximized, m_stack, fullscreen flag) is only accessed from the
    // main thread.
    const ShortcutManager &sc = ShortcutManager::instance();

    if (sc.matches(event, ShortcutManager::ToggleFullscreen)
        && m_stack->currentWidget() == m_imageView)
    {
        setFullScreenMode(!isFullScreen());
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::ActivateEntry)
        && m_stack->currentWidget() == m_browserPage)
    {
        QModelIndex current = m_listView->currentIndex();
        if (current.isValid())
        {
            if (m_imageModel->isFolder(current)) {
                QByteArray path = m_imageModel->filePath(current);
                navigateToFolder(path);
            } else {
                enterSingleImageMode(current);
            }
            event->accept();
            return;
        }
    }

    if (sc.matches(event, ShortcutManager::PrevFolder)
        && m_stack->currentWidget() == m_browserPage)
    {
        navigateToPrevFolder();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::NextFolder)
        && m_stack->currentWidget() == m_browserPage)
    {
        navigateToNextFolder();
        event->accept();
        return;
    }

    if (sc.matches(event, ShortcutManager::MoveToTrash)
        && m_stack->currentWidget() == m_browserPage)
    {
        moveCurrentImageToTrash();
        event->accept();
        return;
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
    m_exifDock->setFeatures(features);
}

void MainWindow::loadPreferences()
{
    QSettings settings("SagoImageBrowser", "SagoImageBrowser");
    m_backgroundColorPreference = settings.value("backgroundColor", "black").toString();
    m_imageView->setBackgroundColor(m_backgroundColorPreference);
    m_lockDockingPreference = settings.value("lockDocking", false).toBool();
    applyDockLocking(m_lockDockingPreference);
    m_folderCacheSizePreference = settings.value("folderCacheSize", 4).toInt();
    m_imageModel->setCacheMaxSize(m_folderCacheSizePreference);
    m_thumbnailCacheSizePreference = settings.value("thumbnailCacheSize", "normal").toString();
    m_imageModel->setThumbnailSize(thumbnailSizeFromString(m_thumbnailCacheSizePreference));
    m_rootLabelsPreference = settings.value("rootLabels").toStringList();
    m_rootPathsPreference = settings.value("rootPaths").toStringList();
    if (m_rootLabelsPreference.isEmpty() || m_rootPathsPreference.isEmpty()) {
        m_rootLabelsPreference = {tr("Home"), tr("Pictures"), tr("File System")};
        m_rootPathsPreference = {QDir::homePath(), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), QDir::rootPath()};
    }
    updateRootList();

    ShortcutManager::instance().load(settings);
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
    settings.setValue("folderCacheSize", m_folderCacheSizePreference);
    settings.setValue("thumbnailCacheSize", m_thumbnailCacheSizePreference);
    settings.setValue("rootLabels", m_rootLabelsPreference);
    settings.setValue("rootPaths", m_rootPathsPreference);
    ShortcutManager::instance().save(settings);
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::onPreferencesTriggered()
{
    PreferencesDialog dialog(this);
    dialog.setBackgroundColor(m_backgroundColorPreference);
    dialog.setLockDocking(m_lockDockingPreference);
    dialog.setCacheSize(m_folderCacheSizePreference);
    dialog.setThumbnailCacheSize(m_thumbnailCacheSizePreference);
    dialog.setRootLabels(m_rootLabelsPreference);
    dialog.setRootPaths(m_rootPathsPreference);

    // Populate shortcuts tab from current bindings
    {
        QMap<int, QKeySequence> scMap;
        for (int i = 0; i < ShortcutManager::ActionCount; ++i)
            scMap[i] = ShortcutManager::instance().shortcut(
                static_cast<ShortcutManager::Action>(i));
        dialog.setShortcuts(scMap);
    }

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
        m_folderCacheSizePreference = dialog.getCacheSize();
        m_imageModel->setCacheMaxSize(m_folderCacheSizePreference);
        QString newThumbSize = dialog.getThumbnailCacheSize();
        if (newThumbSize != m_thumbnailCacheSizePreference)
        {
            m_thumbnailCacheSizePreference = newThumbSize;
            m_imageModel->setThumbnailSize(thumbnailSizeFromString(m_thumbnailCacheSizePreference));
            loadVisibleThumbnails();
        }

        m_rootLabelsPreference = dialog.getRootLabels();
        m_rootPathsPreference = dialog.getRootPaths();
        updateRootList();

        // Apply shortcut changes
        QMap<int, QKeySequence> scMap = dialog.getShortcuts();
        for (auto it = scMap.cbegin(); it != scMap.cend(); ++it)
            ShortcutManager::instance().setShortcut(
                static_cast<ShortcutManager::Action>(it.key()), it.value());

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

void MainWindow::updateRootList()
{
    m_rootList->clear();
    for (int i = 0; i < m_rootLabelsPreference.size() && i < m_rootPathsPreference.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(m_rootLabelsPreference[i], m_rootList);
        item->setData(Qt::UserRole, m_rootPathsPreference[i]);
    }
    if (m_rootList->count() > 0)
        m_rootList->setCurrentRow(0);
}
