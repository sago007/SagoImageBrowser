#pragma once

#include <QMainWindow>

class QTreeView;
class QListView;
class QLabel;
class QStackedWidget;
class QFileSystemModel;
class QComboBox;
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
    void navigateToFolder(const QString &path);
    void selectPreviousFolderIfExists();
    void loadVisibleThumbnails();
    void showPreview(const QModelIndex &index);
    void enterSingleImageMode(const QModelIndex &index);
    void leaveSingleImageMode();

    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    QFileSystemModel *m_dirModel;
    ImageModel *m_imageModel;

    QTreeView *m_treeView;
    QComboBox *m_rootCombo;
    QListView *m_listView;
    QLabel *m_previewLabel;

    QStackedWidget *m_stack;
    QString m_backgroundColorPreference = "black";
    QWidget *m_browserPage;
    ImageViewWidget *m_imageView;
    bool m_wasFullScreen = false;
    bool m_wasMaximized = false;
    QString m_previousFolderPath;
};
