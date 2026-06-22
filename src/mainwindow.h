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

#include <QByteArray>
#include <QMainWindow>

class QTreeView;
class QListView;
class QLabel;
class QStackedWidget;
class QMenuBar;
class QDockWidget;
class FsDirModel;
class QListWidget;
class QTableWidget;
class QLineEdit;
class QPushButton;
class QCompleter;
class QPoint;
class ImageModel;
class ImageViewWidget;
class PathCompleterModel;
class PreferencesDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openPath(const QString &path);

private:
    void setupUi();
    void setupConnections();
    void setupMenuBar();
    void loadPreferences();
    void savePreferences();
    void onPreferencesTriggered();
    void resetLayoutToDefault();
    void navigateToFolder(const QByteArray &path);
    void navigateToFolder(const QString &path);  // convenience: converts via QFile::encodeName
    void selectPreviousFolderIfExists();
    void setFullScreenMode(bool fullScreen);
    void loadVisibleThumbnails();
    void showPreview(const QModelIndex &index);
    void updateExifInfo(const QByteArray &path);
    void enterSingleImageMode(const QModelIndex &index);
    void leaveSingleImageMode();
    void onEditCaption();
    void showImageContextMenu(const QPoint &pos);
    void showImageViewContextMenu(const QPoint &pos);
    void executeContextMenu(const QPoint &globalPos, const QByteArray &path, const QModelIndex &index);
    void moveImageToTrash(const QByteArray &path);
    void moveCurrentImageToTrash();
    void rotateImage(const QModelIndex &index, bool clockwise);
    void onPathEntered();
    void updatePathField(const QByteArray &path);
    QList<QByteArray> computeNeighborPaths(const QModelIndex &index) const;
    QByteArray siblingFolder(int delta) const;
    void navigateToPrevFolder();
    void navigateToNextFolder();

    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    FsDirModel *m_dirModel;
    ImageModel *m_imageModel;

    QTreeView *m_treeView;
    QListWidget *m_rootList;
    QListView *m_listView;
    QLabel *m_previewLabel;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_goButton = nullptr;
    QPushButton *m_prevFolderButton = nullptr;
    QPushButton *m_nextFolderButton = nullptr;
    PathCompleterModel *m_pathCompleterModel = nullptr;
    QCompleter *m_pathCompleter = nullptr;

    QStackedWidget *m_stack;
    QString m_backgroundColorPreference = "black";
    int m_folderCacheSizePreference = 4;
    QString m_thumbnailCacheSizePreference = "normal";
    QWidget *m_browserPage;
    ImageViewWidget *m_imageView;
    bool m_wasMaximized = false;
    bool m_imageViewWasFullScreen = false;
    QByteArray m_previousFolderPath;
    QMenuBar *m_menuBar = nullptr;
    QDockWidget *m_rootDock = nullptr;
    QDockWidget *m_folderDock = nullptr;
    QDockWidget *m_previewDock = nullptr;
    QDockWidget *m_exifDock = nullptr;
    QTableWidget *m_exifTable = nullptr;
    bool m_rootDockWasVisible = true;
    bool m_folderDockWasVisible = true;
    bool m_previewDockWasVisible = true;
    bool m_exifDockWasVisible = true;
    bool m_lockDockingPreference = false;

    void applyDockLocking(bool lock);
};
