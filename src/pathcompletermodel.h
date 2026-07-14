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

#include <QAbstractListModel>
#include <QString>
#include <QStringList>

// Lightweight ajax-style completion model that exposes the immediate
// subdirectories of whatever parent directory the user is typing into the
// path field. Refreshed on each text change via setPrefix(); we only re-scan
// when the parent directory actually changes, so per-keystroke cost is just
// the QCompleter filter pass over an already-loaded list.
//
// Directory enumeration mirrors FsDirModel::listSubdirs (std::filesystem via
// the nativepath helpers), so names the OS's Unicode encoding cannot represent
// are listed correctly.
class PathCompleterModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PathCompleterModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    // Update entries based on the current text in the path field.
    void setPrefix(const QString &text);

private:
    QStringList m_entries;
    QString m_currentParent;
    bool m_hasParent = false;
};
