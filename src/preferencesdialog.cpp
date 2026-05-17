#include "preferencesdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    setModal(true);
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *imageGroup = new QGroupBox("Image Viewer", this);
    QVBoxLayout *groupLayout = new QVBoxLayout(imageGroup);

    QHBoxLayout *bgColorLayout = new QHBoxLayout;
    QLabel *bgColorLabel = new QLabel("Background Color:");
    m_backgroundColorCombo = new QComboBox;
    m_backgroundColorCombo->addItem("Black", "black");
    m_backgroundColorCombo->addItem("White", "white");
    m_backgroundColorCombo->addItem("Dark Gray", "darkgray");

    bgColorLayout->addWidget(bgColorLabel);
    bgColorLayout->addWidget(m_backgroundColorCombo);
    bgColorLayout->addStretch();

    groupLayout->addLayout(bgColorLayout);

    QHBoxLayout *cacheSizeLayout = new QHBoxLayout;
    QLabel *cacheSizeLabel = new QLabel("Folder thumbnail cache (folders):");
    m_cacheSizeSpinBox = new QSpinBox;
    m_cacheSizeSpinBox->setRange(1, 20);
    m_cacheSizeSpinBox->setValue(4);
    cacheSizeLayout->addWidget(cacheSizeLabel);
    cacheSizeLayout->addWidget(m_cacheSizeSpinBox);
    cacheSizeLayout->addStretch();
    groupLayout->addLayout(cacheSizeLayout);

    mainLayout->addWidget(imageGroup);

    QGroupBox *layoutGroup = new QGroupBox("Layout", this);
    QVBoxLayout *layoutGroupLayout = new QVBoxLayout(layoutGroup);
    m_lockDockingCheck = new QCheckBox("Lock docking (prevent accidental moves)");
    layoutGroupLayout->addWidget(m_lockDockingCheck);
    
    m_resetLayoutCheck = new QCheckBox("Reset layout to default when closing the menu");
    layoutGroupLayout->addWidget(m_resetLayoutCheck);
    
    mainLayout->addWidget(layoutGroup);

    mainLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

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
