#include "pathcompletermodel.h"

#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

PathCompleterModel::PathCompleterModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PathCompleterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant PathCompleterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return m_entries.at(index.row());
    return {};
}

void PathCompleterModel::setPrefix(const QString &text)
{
    // Determine the directory whose children we should list.
    QString parentDir;
    if (text.isEmpty()) {
        parentDir = QStringLiteral("/");
    } else if (text.endsWith(QLatin1Char('/'))) {
        // Strip the trailing slash unless we're at filesystem root.
        parentDir = (text.size() == 1) ? text : text.left(text.size() - 1);
    } else {
        QFileInfo info(text);
        parentDir = info.path();
        if (parentDir.isEmpty())
            parentDir = QStringLiteral("/");
    }

    if (m_hasParent && parentDir == m_currentParent)
        return;

    QStringList newEntries;
    try {
        const QByteArray parentNative = QFile::encodeName(parentDir);
        std::vector<std::pair<std::string, std::string>> entries; // {nativePath, filename}
        for (const fs::directory_entry &entry :
             fs::directory_iterator(parentNative.toStdString(),
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

        newEntries.reserve(static_cast<qsizetype>(entries.size()));
        for (const auto &[npath, fname] : entries) {
            newEntries.append(QFile::decodeName(
                QByteArray::fromStdString(npath)));
        }
    } catch (const fs::filesystem_error &) {
        // Inaccessible parent — fall through with an empty list.
    }

    beginResetModel();
    m_entries = std::move(newEntries);
    m_currentParent = parentDir;
    m_hasParent = true;
    endResetModel();
}
