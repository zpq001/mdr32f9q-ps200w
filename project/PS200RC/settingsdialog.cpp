#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QLineEdit>

#include "settingshelper.h"

// Declared in mainwindow.cpp
extern QSettings appSettings;


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    int i;

    ui->setupUi(this);
    this->setWindowTitle("Connection settings");

    intValidator = new QIntValidator(0, 4000000, this);

    ui->baudRateBox->setInsertPolicy(QComboBox::NoInsert);
    connect(ui->baudRateBox, SIGNAL(currentIndexChanged(int)), this, SLOT(checkCustomBaudRatePolicy(int)));
    connect(ui->portNameBox, SIGNAL(currentIndexChanged(int)), this, SLOT(showPortInfo(int)));

    // Fill baud rates
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud9600));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud19200));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud38400));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud57600));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud115200));
    ui->baudRateBox->addItem("Custom");

    // Fill data bits
    for(i=0; serial_databits[i].sv!=0; i++)
        ui->dataBitsBox->addItem(serial_databits[i].sv);

    // Fill parity
    for(i=0; serial_parity[i].sv!=0; i++)
        ui->parityBox->addItem(serial_parity[i].sv);

    // Fill stop bits
    for(i=0; serial_stopbits[i].sv!=0; i++)
        ui->stopBitsBox->addItem(serial_stopbits[i].sv);

    // Fill flow control
    for(i=0; serial_flowctrl[i].sv!=0; i++)
        ui->flowControlBox->addItem(serial_flowctrl[i].sv);

}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}


int SettingsDialog::exec()
{
    populatePortList();
    applySettingsToControls();
    return QDialog::exec();
}

void SettingsDialog::accept()
{
    updateSettingsFromControls();
    QDialog::accept();
}

void SettingsDialog::reject()
{
    QDialog::reject();
}




void SettingsDialog::populatePortList()
{
    ui->portNameBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QStringList list;
        list << info.portName()
             << info.description()
             << info.manufacturer()
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString())
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : QString());

        ui->portNameBox->addItem(list.first(), list);
    }
}

void SettingsDialog::showPortInfo(int idx)
{
    if (idx != -1) {
        QStringList list = ui->portNameBox->itemData(idx).toStringList();
        ui->descriptionLabel->setText(tr("Description: %1").arg(list.at(1)));
        ui->manufacturerLabel->setText(tr("Manufacturer: %1").arg(list.at(2)));
        ui->locationLabel->setText(tr("Location: %1").arg(list.at(3)));
        ui->vidLabel->setText(tr("Vendor Identifier: %1").arg(list.at(4)));
        ui->pidLabel->setText(tr("Product Identifier: %1").arg(list.at(5)));
    }
}

void SettingsDialog::checkCustomBaudRatePolicy(int idx)
{
    //bool isCustomBaudRate = !ui->baudRateBox->itemData(idx).isValid();
    bool isCustomBaudRate = (ui->baudRateBox->count() - 1 == idx);
    ui->baudRateBox->setEditable(isCustomBaudRate);
    if (isCustomBaudRate) {
        ui->baudRateBox->clearEditText();
        QLineEdit *edit = ui->baudRateBox->lineEdit();
        edit->setValidator(intValidator);
    }
}

// Reading settings file and updating form widgets
void SettingsDialog::applySettingsToControls()
{
    // If port name from settings is absent, first detected port will be selected
    QString portName = appSettings.value("serial/port").toString();
    ui->portNameBox->setCurrentText(portName);

    // TODO: add messagebox if there is no selected port in system
    int bRate = appSettings.value("serial/baudrate").toInt();
    checkCustomBaudRatePolicy(ui->baudRateBox->count() - 1);
    ui->baudRateBox->setCurrentText(QString::number(bRate));

    QString dataBitsStr = appSettings.value("serial/databits").toString();
    ui->dataBitsBox->setCurrentText(dataBitsStr);

    QString parityStr = appSettings.value("serial/parity").toString();
    ui->parityBox->setCurrentText(parityStr);

    QString stopBitsStr = appSettings.value("serial/stopbits").toString();
    ui->stopBitsBox->setCurrentText(stopBitsStr);

    QString flowStr = appSettings.value("serial/flowctrl").toString();
    ui->flowControlBox->setCurrentText(flowStr);
}


// Reading settings file and updating form widgets
void SettingsDialog::updateSettingsFromControls()
{
    settingsMutex.lock();
    appSettings.setValue("serial/port", ui->portNameBox->currentText());
    appSettings.setValue("serial/baudrate", ui->baudRateBox->currentText().toInt());
    appSettings.setValue("serial/databits", ui->dataBitsBox->currentText());
    appSettings.setValue("serial/parity", ui->parityBox->currentText());
    appSettings.setValue("serial/stopbits", ui->stopBitsBox->currentText());
    appSettings.setValue("serial/flowctrl", ui->flowControlBox->currentText());
    settingsMutex.unlock();
}

