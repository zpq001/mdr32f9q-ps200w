#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>

// Declared in mainwindow.cpp
extern QSettings appSettings;


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    this->setWindowTitle("Connection settings");

    ui->baudRateBox->setInsertPolicy(QComboBox::NoInsert);
    //connect(ui->baudRateBox, SIGNAL(currentIndexChanged(int)), this, SLOT(checkCustomBaudRatePolicy(int)));
    connect(ui->portNameBox, SIGNAL(currentIndexChanged(int)), this, SLOT(showPortInfo(int)));

    // Fill baud rates
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud9600));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud19200));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud38400));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud57600));
    ui->baudRateBox->addItem(QString::number(QSerialPort::Baud115200));
    ui->baudRateBox->addItem("Custom");

    // Fill data bits
    ui->dataBitsBox->addItem(QString::number(QSerialPort::Data5));
    ui->dataBitsBox->addItem(QString::number(QSerialPort::Data6));
    ui->dataBitsBox->addItem(QString::number(QSerialPort::Data7));
    ui->dataBitsBox->addItem(QString::number(QSerialPort::Data8));

    // Fill parity
    ui->parityBox->addItem("None");
    ui->parityBox->addItem("Even");
    ui->parityBox->addItem("Odd");
    ui->parityBox->addItem("Mark");
    ui->parityBox->addItem("Space");
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
    //updateSettingsFromControls();
    QDialog::accept();
}

void SettingsDialog::reject()
{
    QDialog::reject();
}

QString SettingsDialog::getTextForParity(QSerialPort::Parity p)
{
    QString str;
    switch(p)
    {
        case QSerialPort::NoParity:
            str = "None";   break;
        case QSerialPort::EvenParity:
            str = "Even";   break;
        case QSerialPort::OddParity:
            str = "Odd";    break;
        case QSerialPort::MarkParity:
            str = "Mark";    break;
        case QSerialPort::SpaceParity:
            str = "Space";    break;
        default:
            str = "???";
    }
    return str;
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

// Reading settings file and updating form widgets
void SettingsDialog::applySettingsToControls()
{
    int i;
    QString portName = appSettings.value("serial/port").toString();
    ui->portNameBox->setCurrentText(portName);

    int bRate = appSettings.value("serial/baudrate").toInt();
    //checkCustomBaudRatePolicy(ui->baudRateBox->count() - 1);
    ui->baudRateBox->setCurrentText(QString::number(bRate));

    int dataBits = appSettings.value("serial/databits").toInt();
    ui->dataBitsBox->setCurrentText(QString::number(dataBits));

    QString parityStr = appSettings.value("serial/parity").toString();
    i = ui->parityBox->findText(parityStr);
    if (i != -1)
    {
        ui->parityBox->setCurrentIndex(i);
    }


    QString stopBitsStr = appSettings.value("serial/stopbits").toString();
    ui->stopBitsBox->setCurrentText(stopBitsStr);

    QString flowStr = appSettings.value("serial/flowcontrol").toString();
    ui->flowControlBox->setCurrentText(flowStr);
}




