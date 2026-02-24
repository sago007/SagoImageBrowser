#pragma once

#include <QDialog>

class QComboBox;

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    QString getBackgroundColor() const;
    void setBackgroundColor(const QString &color);

private:
    QComboBox *m_backgroundColorCombo;
};
