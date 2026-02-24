#include "preferencesdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
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
    mainLayout->addWidget(imageGroup);

    mainLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
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
