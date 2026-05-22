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
// Directory enumeration mirrors FsDirModel::listSubdirs (std::filesystem with
// QFile::decodeName), so non-ASCII names that round-trip through the locale
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
