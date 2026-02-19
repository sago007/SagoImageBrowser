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

    QFileSystemModel *m_dirModel;
    ImageModel *m_imageModel;

    QTreeView *m_treeView;
    QListView *m_listView;
    QLabel *m_previewLabel;
};
