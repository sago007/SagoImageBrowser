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

#include "pathcompletermodel.h"

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
