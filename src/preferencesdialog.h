#pragma once

#include <QDialog>

class QComboBox;
class QCheckBox;

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    QString getBackgroundColor() const;
    void setBackgroundColor(const QString &color);

    bool getLockDocking() const;
    void setLockDocking(bool lock);

private:
    QComboBox *m_backgroundColorCombo;
    QCheckBox *m_lockDockingCheck;
};
