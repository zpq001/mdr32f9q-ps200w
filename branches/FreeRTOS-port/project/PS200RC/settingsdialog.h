#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QIntValidator>
#include <QtSerialPort/QSerialPort>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();
    QSerialPort::Parity getParityFromText(const QString &parityStr);
    QSerialPort::DataBits getDataBitsFromText(const QString &str);
    QSerialPort::StopBits getStopBitsFromText(const QString &str);

public slots:
    void accept();
    void reject();
    int exec();
private slots:
    void showPortInfo(int idx);
    void checkCustomBaudRatePolicy(int idx);

private:
    Ui::SettingsDialog *ui;
    QIntValidator *intValidator;
    void populatePortList();
    void applySettingsToControls();
    void updateSettingsFromControls();


};

#endif // SETTINGSDIALOG_H
