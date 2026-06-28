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

    QStringList getRootLabels() const;
    void setRootLabels(const QStringList &labels);

    QStringList getRootPaths() const;
    void setRootPaths(const QStringList &paths);

signals:
    void resetLayoutRequested();

private:
    void setupGeneralTab(QTabWidget *tabs);
    void setupShortcutsTab(QTabWidget *tabs);
    void setupRootsTab(QTabWidget *tabs);
    void highlightConflicts();

    QComboBox *m_backgroundColorCombo;
    QSpinBox  *m_cacheSizeSpinBox;
    QComboBox *m_thumbnailSizeCombo;
    QCheckBox *m_lockDockingCheck;
    QCheckBox *m_resetLayoutCheck;
    QTableWidget *m_shortcutsTable = nullptr;
    QTableWidget *m_rootsTable = nullptr;
    QList<int> m_rowToAction;  // maps table row → ShortcutManager::Action
};
