#pragma once

#include <QMainWindow>

class QTreeView;
class QListView;
class QLabel;
class QStackedWidget;
class QMenuBar;
class QDockWidget;
class QFileSystemModel;
class QListWidget;
class ImageModel;
class ImageViewWidget;
class PreferencesDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void setupConnections();
    void setupMenuBar();
    void loadPreferences();
    void savePreferences();
    void onPreferencesTriggered();
    void resetLayoutToDefault();
    void navigateToFolder(const QString &path);
    void selectPreviousFolderIfExists();
    void setFullScreenMode(bool fullScreen);
    void loadVisibleThumbnails();
    void showPreview(const QModelIndex &index);
    void enterSingleImageMode(const QModelIndex &index);
    void leaveSingleImageMode();
    QStringList computeNeighborPaths(const QModelIndex &index) const;

    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    QFileSystemModel *m_dirModel;
    ImageModel *m_imageModel;

    QTreeView *m_treeView;
    QListWidget *m_rootList;
    QListView *m_listView;
    QLabel *m_previewLabel;

    QStackedWidget *m_stack;
    QString m_backgroundColorPreference = "black";
    int m_folderCacheSizePreference = 4;
    QString m_thumbnailCacheSizePreference = "normal";
    QWidget *m_browserPage;
    ImageViewWidget *m_imageView;
    bool m_wasMaximized = false;
    bool m_imageViewWasFullScreen = false;
    QString m_previousFolderPath;
    QMenuBar *m_menuBar = nullptr;
    QDockWidget *m_rootDock = nullptr;
    QDockWidget *m_folderDock = nullptr;
    QDockWidget *m_previewDock = nullptr;
    bool m_rootDockWasVisible = true;
    bool m_folderDockWasVisible = true;
    bool m_previewDockWasVisible = true;
    bool m_lockDockingPreference = false;

    void applyDockLocking(bool lock);
};