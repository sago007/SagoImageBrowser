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
