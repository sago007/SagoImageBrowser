#pragma once

#include <QDialog>
#include <QKeySequence>
#include <QMap>

class QComboBox;
class QCheckBox;
class QSpinBox;
class QTabWidget;
class QTableWidget;

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    QString getBackgroundColor() const;
    void setBackgroundColor(const QString &color);

    int getCacheSize() const;
    void setCacheSize(int n);

    QString getThumbnailCacheSize() const;
    void setThumbnailCacheSize(const QString &size);

    bool getLockDocking() const;
    void setLockDocking(bool lock);

    bool getResetLayout() const;
    void setResetLayout(bool reset);

    QMap<int, QKeySequence> getShortcuts() const;
    void setShortcuts(const QMap<int, QKeySequence> &map);

signals:
    void resetLayoutRequested();

private:
    void setupGeneralTab(QTabWidget *tabs);
    void setupShortcutsTab(QTabWidget *tabs);
    void highlightConflicts();

    QComboBox *m_backgroundColorCombo;
    QSpinBox  *m_cacheSizeSpinBox;
    QComboBox *m_thumbnailSizeCombo;
    QCheckBox *m_lockDockingCheck;
    QCheckBox *m_resetLayoutCheck;
    QTableWidget *m_shortcutsTable = nullptr;
    QList<int> m_rowToAction;  // maps table row → ShortcutManager::Action
};
