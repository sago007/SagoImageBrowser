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

    bool getResetLayout() const;
    void setResetLayout(bool reset);

signals:
    void resetLayoutRequested();

private:
    QComboBox *m_backgroundColorCombo;
    QCheckBox *m_lockDockingCheck;
    QCheckBox *m_resetLayoutCheck;
};
