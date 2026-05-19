#pragma once

#include <QAbstractItemModel>
#include <QByteArray>
#include <QIcon>
#include <QList>

// A directory-tree model that uses std::filesystem for enumeration so that
// directory entries with non-UTF-8 filenames (e.g. Latin-1 encoded names on
// a UTF-8 locale) are listed correctly instead of being silently dropped or
// mangled by QDir / QFileSystemModel.
//
// The model is lazily populated: child directories are enumerated only when
// the view first expands a node (via canFetchMore / fetchMore).
class FsDirModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FsDirModel(QObject *parent = nullptr);
    ~FsDirModel() override;

    // Set the directory shown as the tree root.  Call rootIndex() afterwards
    // and pass the result to QTreeView::setRootIndex() so the root node itself
    // is hidden and only its children are displayed as top-level items.
    void setRootPath(const QByteArray &nativePath);
    void setRootPath(const QString &path);   // convenience: converts via QFile::encodeName

    // Model index of the root-path node (the one whose *children* are shown
    // as top-level items when passed to QTreeView::setRootIndex).
    QModelIndex rootIndex() const;

    // Raw native OS byte path for the node at index.
    QByteArray filePath(const QModelIndex &index) const;

    // Walk the tree to find (and lazily populate) the node for nativePath.
    // Returns an invalid index if the path is not under the current root.
    QModelIndex indexForPath(const QByteArray &nativePath);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

private:
    struct Node {
        QByteArray  nativePath;
        QString     displayName;
        Node       *parent   = nullptr;
        QList<Node *> children;
        bool        populated = false;
    };

    // Synchronously populate node->children from the filesystem.
    // Only call this inside fetchMore (with proper begin/endInsertRows around it).
    static QList<Node *> listSubdirs(Node *parent);

    Node *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(Node *node) const;
    static void destroyChildren(Node *node);

    Node   m_virtualRoot;   // hidden root; always has exactly 0 or 1 child
    QIcon  m_folderIcon;
};
