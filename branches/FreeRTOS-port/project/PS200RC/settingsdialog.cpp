#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QLineEdit>

// Declared in mainwindow.cpp
extern QSettings appSettings;


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
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


QSerialPort::Parity SettingsDialog::getParityFromText(const QString &str)
{
    if (str.compare("None",Qt::CaseInsensitive) == 0)
        return QSerialPort::NoParity;
    else if (str.compare("Even",Qt::CaseInsensitive) == 0)
        return QSerialPort::EvenParity;
    else if (str.compare("Odd",Qt::CaseInsensitive) == 0)
        return QSerialPort::OddParity;
    else if (str.compare("Mark",Qt::CaseInsensitive) == 0)
        return QSerialPort::MarkParity;
    else if (str.compare("Space",Qt::CaseInsensitive) == 0)
        return QSerialPort::SpaceParity;
    else
        return QSerialPort::UnknownParity;
}

QSerialPort::DataBits SettingsDialog::getDataBitsFromText(const QString &str)
{
    if (str.compare("5") == 0)
        return QSerialPort::Data5;
    else if (str.compare("6") == 0)
        return QSerialPort::Data6;
    else if (str.compare("7") == 0)
        return QSerialPort::Data7;
    else if (str.compare("8") == 0)
        return QSerialPort::Data8;
    else
        return QSerialPort::UnknownDataBits;
}

QSerialPort::StopBits SettingsDialog::getStopBitsFromText(const QString &str)
{
    if (str.compare("1") == 0)
        return QSerialPort::OneStop;
    else if (str.compare("1.5") == 0)
        return QSerialPort::OneAndHalfStop;
    else if (str.compare("2") == 0)
        return QSerialPort::TwoStop;
    else
        return QSerialPort::UnknownStopBits;
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

    int bRate = appSettings.value("serial/baudrate").toInt();
    checkCustomBaudRatePolicy(ui->baudRateBox->count() - 1);
    ui->baudRateBox->setCurrentText(QString::number(bRate));

    QString dataBitsStr = appSettings.value("serial/databits").toString();
    ui->dataBitsBox->setCurrentText(dataBitsStr);

    QString parityStr = appSettings.value("serial/parity").toString();
    ui->parityBox->setCurrentText(parityStr);

    QString stopBitsStr = appSettings.value("serial/stopbits").toString();
    ui->stopBitsBox->setCurrentText(stopBitsStr);

    QString flowStr = appSettings.value("serial/flowcontrol").toString();
    ui->flowControlBox->setCurrentText(flowStr);
}




