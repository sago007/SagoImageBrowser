#pragma once

#include <QMainWindow>

class QTreeView;
class QListView;
class QLabel;
class QFileSystemModel;
class ImageModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void setupConnections();
    void loadVisibleThumbnails();

    bool eventFilter(QObject *obj, QEvent *event) override;

    QFileSystemModel *m_dirModel;
    ImageModel *m_imageModel;

    QTreeView *m_treeView;
    QListView *m_listView;
    QLabel *m_previewLabel;
};
