#include "preferencesdialog.h"
#include "shortcutmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QKeySequenceEdit>
#include <QHeaderView>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    setModal(true);
    setMinimumWidth(500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QTabWidget *tabs = new QTabWidget(this);
    setupGeneralTab(tabs);
    setupShortcutsTab(tabs);
    mainLayout->addWidget(tabs);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, [this]() {
        if (m_resetLayoutCheck->isChecked())
            emit resetLayoutRequested();
        accept();
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void PreferencesDialog::setupGeneralTab(QTabWidget *tabs)
{
    QWidget *page = new QWidget;
    QVBoxLayout *pageLayout = new QVBoxLayout(page);

    QGroupBox *imageGroup = new QGroupBox(tr("Image Viewer"), page);
    QVBoxLayout *groupLayout = new QVBoxLayout(imageGroup);

    QHBoxLayout *bgColorLayout = new QHBoxLayout;
    QLabel *bgColorLabel = new QLabel(tr("Background Color:"));
    m_backgroundColorCombo = new QComboBox;
    m_backgroundColorCombo->addItem(tr("Black"), "black");
    m_backgroundColorCombo->addItem(tr("White"), "white");
    m_backgroundColorCombo->addItem(tr("Dark Gray"), "darkgray");

    bgColorLayout->addWidget(bgColorLabel);
    bgColorLayout->addWidget(m_backgroundColorCombo);
    bgColorLayout->addStretch();

    groupLayout->addLayout(bgColorLayout);

    QHBoxLayout *cacheSizeLayout = new QHBoxLayout;
    QLabel *cacheSizeLabel = new QLabel(tr("Folder thumbnail cache (folders):"));
    m_cacheSizeSpinBox = new QSpinBox;
    m_cacheSizeSpinBox->setRange(1, 20);
    m_cacheSizeSpinBox->setValue(4);
    cacheSizeLayout->addWidget(cacheSizeLabel);
    cacheSizeLayout->addWidget(m_cacheSizeSpinBox);
    cacheSizeLayout->addStretch();
    groupLayout->addLayout(cacheSizeLayout);

    QHBoxLayout *thumbSizeLayout = new QHBoxLayout;
    QLabel *thumbSizeLabel = new QLabel(tr("Thumbnail cache size:"));
    m_thumbnailSizeCombo = new QComboBox;
    m_thumbnailSizeCombo->addItem(tr("Normal (128 px)"), "normal");
    m_thumbnailSizeCombo->addItem(tr("Large (256 px)"), "large");
    thumbSizeLayout->addWidget(thumbSizeLabel);
    thumbSizeLayout->addWidget(m_thumbnailSizeCombo);
    thumbSizeLayout->addStretch();
    groupLayout->addLayout(thumbSizeLayout);

    pageLayout->addWidget(imageGroup);

    QGroupBox *layoutGroup = new QGroupBox(tr("Layout"), page);
    QVBoxLayout *layoutGroupLayout = new QVBoxLayout(layoutGroup);
    m_lockDockingCheck = new QCheckBox(tr("Lock docking (prevent accidental moves)"));
    layoutGroupLayout->addWidget(m_lockDockingCheck);

    m_resetLayoutCheck = new QCheckBox(tr("Reset layout to default when closing the menu"));
    layoutGroupLayout->addWidget(m_resetLayoutCheck);

    pageLayout->addWidget(layoutGroup);
    pageLayout->addStretch();

    tabs->addTab(page, tr("General"));
}

void PreferencesDialog::setupShortcutsTab(QTabWidget *tabs)
{
    QWidget *page = new QWidget;
    QVBoxLayout *pageLayout = new QVBoxLayout(page);

    // Count configurable actions to size the table (plus 2 section header rows)
    int configurableCount = 0;
    for (int i = 0; i < ShortcutManager::ActionCount; ++i) {
        if (ShortcutManager::actionCategory(static_cast<ShortcutManager::Action>(i))
            != ShortcutManager::CatNotConfigurable)
            ++configurableCount;
    }
    // +2 for section headers (Image Viewer, Browser)
    const int totalRows = configurableCount + 2;

    m_shortcutsTable = new QTableWidget(totalRows, 2, page);
    m_shortcutsTable->setHorizontalHeaderLabels({tr("Action"), tr("Key")});
    m_shortcutsTable->horizontalHeader()->setStretchLastSection(true);
    m_shortcutsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_shortcutsTable->verticalHeader()->hide();
    m_shortcutsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    m_rowToAction.clear();
    const ShortcutManager &mgr = ShortcutManager::instance();
    int row = 0;

    auto addSectionHeader = [&](const QString &title) {
        auto *item = new QTableWidgetItem(title);
        item->setFlags(Qt::NoItemFlags);
        QFont f = item->font();
        f.setBold(true);
        item->setFont(f);
        item->setBackground(palette().alternateBase());
        m_shortcutsTable->setItem(row, 0, item);
        auto *emptyItem = new QTableWidgetItem();
        emptyItem->setFlags(Qt::NoItemFlags);
        emptyItem->setBackground(palette().alternateBase());
        m_shortcutsTable->setItem(row, 1, emptyItem);
        m_rowToAction.append(-1); // -1 = section header
        ++row;
    };

    auto addAction = [&](ShortcutManager::Action action) {
        auto *descItem = new QTableWidgetItem(mgr.description(action));
        descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);
        m_shortcutsTable->setItem(row, 0, descItem);

        auto *edit = new QKeySequenceEdit(mgr.shortcut(action));
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        edit->setMaximumSequenceLength(1);
#endif
        connect(edit, &QKeySequenceEdit::keySequenceChanged, this, [this]() {
            highlightConflicts();
        });
        m_shortcutsTable->setCellWidget(row, 1, edit);
        m_rowToAction.append(static_cast<int>(action));
        ++row;
    };

    // Image Viewer section
    addSectionHeader(tr("Image Viewer"));
    for (int i = 0; i < ShortcutManager::ActionCount; ++i) {
        auto action = static_cast<ShortcutManager::Action>(i);
        if (ShortcutManager::actionCategory(action) == ShortcutManager::CatImageViewer)
            addAction(action);
    }

    // Browser section
    addSectionHeader(tr("Browser"));
    for (int i = 0; i < ShortcutManager::ActionCount; ++i) {
        auto action = static_cast<ShortcutManager::Action>(i);
        if (ShortcutManager::actionCategory(action) == ShortcutManager::CatBrowser)
            addAction(action);
    }

    pageLayout->addWidget(m_shortcutsTable);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *resetBtn = new QPushButton(tr("Reset to Defaults"));
    btnLayout->addStretch();
    btnLayout->addWidget(resetBtn);
    pageLayout->addLayout(btnLayout);

    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        for (int r = 0; r < m_rowToAction.size(); ++r) {
            int actionIdx = m_rowToAction[r];
            if (actionIdx < 0)
                continue;
            auto *edit = qobject_cast<QKeySequenceEdit *>(
                m_shortcutsTable->cellWidget(r, 1));
            if (edit)
                edit->setKeySequence(ShortcutManager::defaultShortcut(
                    static_cast<ShortcutManager::Action>(actionIdx)));
        }
        highlightConflicts();
    });

    tabs->addTab(page, tr("Shortcuts"));
}

void PreferencesDialog::highlightConflicts()
{
    // Group sequences by category, then detect conflicts within each category.
    // key: (category, sequenceString) → list of rows
    QMap<QPair<int,QString>, QList<int>> catSeqToRows;

    for (int r = 0; r < m_rowToAction.size(); ++r) {
        int actionIdx = m_rowToAction[r];
        if (actionIdx < 0)
            continue;
        auto *edit = qobject_cast<QKeySequenceEdit *>(
            m_shortcutsTable->cellWidget(r, 1));
        if (!edit)
            continue;
        QString s = edit->keySequence().toString();
        if (!s.isEmpty()) {
            int cat = ShortcutManager::actionCategory(
                static_cast<ShortcutManager::Action>(actionIdx));
            catSeqToRows[{cat, s}].append(r);
        }
    }

    // Reset all backgrounds
    for (int r = 0; r < m_rowToAction.size(); ++r) {
        if (m_rowToAction[r] < 0)
            continue;
        QTableWidgetItem *item = m_shortcutsTable->item(r, 0);
        if (item)
            item->setBackground(QBrush());
    }

    // Highlight conflicts (only within same category)
    for (auto it = catSeqToRows.cbegin(); it != catSeqToRows.cend(); ++it) {
        if (it.value().size() > 1) {
            for (int r : it.value()) {
                QTableWidgetItem *item = m_shortcutsTable->item(r, 0);
                if (item)
                    item->setBackground(QColor(255, 100, 100, 80));
            }
        }
    }
}

QMap<int, QKeySequence> PreferencesDialog::getShortcuts() const
{
    QMap<int, QKeySequence> map;
    for (int r = 0; r < m_rowToAction.size(); ++r) {
        int actionIdx = m_rowToAction[r];
        if (actionIdx < 0)
            continue;
        auto *edit = qobject_cast<QKeySequenceEdit *>(
            m_shortcutsTable->cellWidget(r, 1));
        if (edit)
            map[actionIdx] = edit->keySequence();
    }
    return map;
}

void PreferencesDialog::setShortcuts(const QMap<int, QKeySequence> &map)
{
    for (int r = 0; r < m_rowToAction.size(); ++r) {
        int actionIdx = m_rowToAction[r];
        if (actionIdx < 0)
            continue;
        if (!map.contains(actionIdx))
            continue;
        auto *edit = qobject_cast<QKeySequenceEdit *>(
            m_shortcutsTable->cellWidget(r, 1));
        if (edit)
            edit->setKeySequence(map[actionIdx]);
    }
    highlightConflicts();
}

QString PreferencesDialog::getBackgroundColor() const
{
    return m_backgroundColorCombo->currentData().toString();
}

void PreferencesDialog::setBackgroundColor(const QString &color)
{
    int index = m_backgroundColorCombo->findData(color);
    if (index >= 0)
        m_backgroundColorCombo->setCurrentIndex(index);
}

int PreferencesDialog::getCacheSize() const
{
    return m_cacheSizeSpinBox->value();
}

void PreferencesDialog::setCacheSize(int n)
{
    m_cacheSizeSpinBox->setValue(n);
}

QString PreferencesDialog::getThumbnailCacheSize() const
{
    return m_thumbnailSizeCombo->currentData().toString();
}

void PreferencesDialog::setThumbnailCacheSize(const QString &size)
{
    int index = m_thumbnailSizeCombo->findData(size);
    if (index >= 0)
        m_thumbnailSizeCombo->setCurrentIndex(index);
}

bool PreferencesDialog::getLockDocking() const
{
    return m_lockDockingCheck->isChecked();
}

void PreferencesDialog::setLockDocking(bool lock)
{
    m_lockDockingCheck->setChecked(lock);
}

bool PreferencesDialog::getResetLayout() const
{
    return m_resetLayoutCheck->isChecked();
}

void PreferencesDialog::setResetLayout(bool reset)
{
    m_resetLayoutCheck->setChecked(reset);
}
