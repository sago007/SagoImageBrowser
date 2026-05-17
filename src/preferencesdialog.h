#pragma once

#include <QDialog>

class QComboBox;
class QCheckBox;
class QSpinBox;

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    QString getBackgroundColor() const;
    void setBackgroundColor(const QString &color);

    int getCacheSize() const;
    void setCacheSize(int n);

    bool getLockDocking() const;
    void setLockDocking(bool lock);

    bool getResetLayout() const;
    void setResetLayout(bool reset);

signals:
    void resetLayoutRequested();

private:
    QComboBox *m_backgroundColorCombo;
    QSpinBox  *m_cacheSizeSpinBox;
    QCheckBox *m_lockDockingCheck;
    QCheckBox *m_resetLayoutCheck;
};
