#include "fsdirmodel.h"

#include <QFile>
#include <QFileIconProvider>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

QList<FsDirModel::Node *> FsDirModel::listSubdirs(Node *parentNode)
{
    QList<Node *> result;
    try {
        std::vector<std::pair<std::string, std::string>> entries; // {nativePath, filename}

        for (const fs::directory_entry &entry :
             fs::directory_iterator(parentNode->nativePath.toStdString(),
                                    fs::directory_options::skip_permission_denied))
        {
            std::error_code ec;
            const std::string fname = entry.path().filename().native();
            if (fname.empty() || fname[0] == '.')
                continue;
            if (!entry.is_directory(ec))
                continue;
            entries.push_back({entry.path().native(), fname});
        }

        std::sort(entries.begin(), entries.end(),
                  [](const auto &a, const auto &b) { return a.second < b.second; });

        result.reserve(static_cast<qsizetype>(entries.size()));
        for (const auto &[npath, fname] : entries) {
            auto *child = new Node;
            child->nativePath  = QByteArray::fromStdString(npath);
            child->displayName = QString::fromLocal8Bit(fname.c_str(),
                                                        static_cast<qsizetype>(fname.size()));
            child->parent      = parentNode;
            result.append(child);
        }
    } catch (const fs::filesystem_error &) {}

    return result;
}

void FsDirModel::destroyChildren(Node *node)
{
    for (Node *child : node->children) {
        destroyChildren(child);
        delete child;
    }
    node->children.clear();
    node->populated = false;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

FsDirModel::FsDirModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_virtualRoot.populated = true; // virtual root is always "populated"

    QFileIconProvider iconProvider;
    m_folderIcon = iconProvider.icon(QFileIconProvider::Folder);
}

FsDirModel::~FsDirModel()
{
    destroyChildren(&m_virtualRoot);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void FsDirModel::setRootPath(const QString &path)
{
    setRootPath(QFile::encodeName(path));
}

void FsDirModel::setRootPath(const QByteArray &nativePath)
{
    beginResetModel();
    destroyChildren(&m_virtualRoot);

    if (!nativePath.isEmpty()) {
        auto *node = new Node;
        node->nativePath  = nativePath;
        // Display name = last path component (empty for "/", use "/" in that case)
        std::string fname = fs::path(nativePath.toStdString()).filename().native();
        node->displayName = fname.empty()
            ? QFile::decodeName(nativePath)
            : QString::fromLocal8Bit(fname.c_str(), static_cast<qsizetype>(fname.size()));
        node->parent      = &m_virtualRoot;
        m_virtualRoot.children.append(node);
    }

    endResetModel();
}

QModelIndex FsDirModel::rootIndex() const
{
    if (m_virtualRoot.children.isEmpty())
        return {};
    return createIndex(0, 0, m_virtualRoot.children.first());
}

QByteArray FsDirModel::filePath(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    return nodeForIndex(index)->nativePath;
}

QModelIndex FsDirModel::indexForPath(const QByteArray &nativePath)
{
    if (nativePath.isEmpty() || m_virtualRoot.children.isEmpty())
        return {};

    const fs::path target(nativePath.toStdString());
    const fs::path root(m_virtualRoot.children.first()->nativePath.toStdString());

    // Collect the ancestry chain from root down to target
    std::vector<fs::path> chain;
    fs::path cur = target;
    while (cur != root) {
        chain.push_back(cur);
        fs::path up = cur.parent_path();
        if (up == cur)
            return {}; // reached filesystem root without finding model root
        cur = up;
    }
    // chain is [target, …, root_child]; reverse so we walk root→target
    std::reverse(chain.begin(), chain.end());

    // Start at the root path node
    Node *node = m_virtualRoot.children.first();
    QModelIndex idx = createIndex(0, 0, node);

    for (const fs::path &step : chain) {
        // Ensure this node's children are loaded
        if (!node->populated) {
            QList<Node *> children = listSubdirs(node);
            if (!children.isEmpty()) {
                beginInsertRows(idx, 0, children.size() - 1);
                node->children = children;
                node->populated = true;
                endInsertRows();
            } else {
                node->populated = true;
            }
        }

        // Find the child whose nativePath matches step
        const std::string stepStr = step.native();
        bool found = false;
        for (int i = 0; i < node->children.size(); ++i) {
            if (node->children[i]->nativePath.toStdString() == stepStr) {
                node = node->children[i];
                idx  = createIndex(i, 0, node);
                found = true;
                break;
            }
        }
        if (!found)
            return {};
    }

    return idx;
}

// ---------------------------------------------------------------------------
// QAbstractItemModel interface
// ---------------------------------------------------------------------------

FsDirModel::Node *FsDirModel::nodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return const_cast<Node *>(&m_virtualRoot);
    return static_cast<Node *>(index.internalPointer());
}

QModelIndex FsDirModel::indexForNode(Node *node) const
{
    if (!node || node == &m_virtualRoot)
        return {};
    Node *parentNode = node->parent;
    int row = parentNode->children.indexOf(node);
    if (row < 0)
        return {};
    return createIndex(row, 0, node);
}

QModelIndex FsDirModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return {};

    Node *parentNode = nodeForIndex(parent);
    if (row < 0 || row >= parentNode->children.size())
        return {};

    return createIndex(row, 0, parentNode->children[row]);
}

QModelIndex FsDirModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    Node *node       = static_cast<Node *>(index.internalPointer());
    Node *parentNode = node->parent;

    if (!parentNode || parentNode == &m_virtualRoot)
        return {};

    return indexForNode(parentNode);
}

int FsDirModel::rowCount(const QModelIndex &parent) const
{
    return nodeForIndex(parent)->children.size();
}

int FsDirModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant FsDirModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const Node *node = static_cast<const Node *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        return node->displayName;
    case Qt::DecorationRole:
        return m_folderIcon;
    default:
        return {};
    }
}

bool FsDirModel::hasChildren(const QModelIndex &parent) const
{
    const Node *node = nodeForIndex(parent);
    if (!node->populated)
        return true; // optimistic: show expand arrow until we actually look
    return !node->children.isEmpty();
}

bool FsDirModel::canFetchMore(const QModelIndex &parent) const
{
    return !nodeForIndex(parent)->populated;
}

void FsDirModel::fetchMore(const QModelIndex &parent)
{
    Node *node = nodeForIndex(parent);
    if (node->populated)
        return;

    QList<Node *> children = listSubdirs(node);
    node->populated = true;

    if (children.isEmpty())
        return;

    beginInsertRows(parent, 0, children.size() - 1);
    node->children = children;
    endInsertRows();
}
