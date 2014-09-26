#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSettings>


QSettings appSettings("settings.ini", QSettings::IniFormat);



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create instances
    myValueDialog = new SingleValueDialog(this);
    mySettingsDialog = new SettingsDialog(this);
    serialPort = new QSerialPort(this);
    serialStatusLabel = new QLabel(this);
    ui->statusBar->addWidget(serialStatusLabel);

    // Setup signals
    //connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(openSerialPort()));
    //connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(closeSerialPort()));
    connect (ui->actionSettings, SIGNAL(triggered()), mySettingsDialog, SLOT(exec()));

    connect(ui->setVoltageBtn, SIGNAL(clicked()), this, SLOT(on_SetVoltageCommand()));
    connect(ui->setCurrentBtn, SIGNAL(clicked()), this, SLOT(on_SetCurrentCommand()));


    // Sync value cache
    memset(&vcache, 0, sizeof(vcache));
    updateVset(vcache.vset);
    updateVmea(vcache.vmea);
    updateCset(vcache.cset);
    updateCmea(vcache.cmea);

    // Default widgets state
    ui->actionDisconnect->setEnabled(false);
    serialStatusLabel->setText("Disconnected");

    // Make sure all settings are valid
    validateSettings();
}

MainWindow::~MainWindow()
{
    appSettings.sync();
    delete ui;
}

void MainWindow::validateSettings(void)
{
    if (appSettings.contains("serial/port") == false)
        appSettings.setValue("serial/port", "COM9");
    if (appSettings.contains("serial/baudrate") == false)
        appSettings.setValue("serial/baudrate", QSerialPort::Baud115200);
    if (appSettings.contains("serial/databits") == false)
        appSettings.setValue("serial/databits", QSerialPort::Data8);
    if (appSettings.contains("serial/parity") == false)
        appSettings.setValue("serial/parity", "None");
    if (appSettings.contains("serial/stopbits") == false)
        appSettings.setValue("serial/stopbits", "1");
    if (appSettings.contains("serial/flowcontrol") == false)
        appSettings.setValue("serial/flowcontrol", "None");
    // More...
}



//-------------------------------------------------------//
/// @brief Updates measured voltage
/// @param[in] value - voltage [mV]
/// @note Also updates value cache
/// @return none
//-------------------------------------------------------//
void MainWindow::updateVmea(int value)
{
    QString str;
    str.sprintf("%1.2fV", (double)value/1000);
    ui->label_vmea->setText(str);
    vcache.vmea = value;
}

//-------------------------------------------------------//
/// @brief Updates measured current
/// @param[in] value - current [mA]
/// @note Also updates value cache
/// @return none
//-------------------------------------------------------//
void MainWindow::updateCmea(int value)
{
    QString str;
    str.sprintf("%1.2fA", (double)value/1000);
    ui->label_cmea->setText(str);
    vcache.cmea = value;
}

//-------------------------------------------------------//
/// @brief Updates voltage setting
/// @param[in] value - voltage [mV]
/// @note Also updates value cache
/// @return none
//-------------------------------------------------------//
void MainWindow::updateVset(int value)
{
    QString str;
    str.sprintf("%1.2fV", (double)value/1000);
    ui->label_vset->setText(str);
    vcache.vset = value;
}

//-------------------------------------------------------//
/// @brief Updates current setting
/// @param[in] value - current [mA]
/// @note Also updates value cache
/// @return none
//-------------------------------------------------------//
void MainWindow::updateCset(int value)
{
    QString str;
    str.sprintf("%1.2fA", (double)value/1000);
    ui->label_cset->setText(str);
    vcache.cset = value;
}



void MainWindow::on_SetVoltageCommand(void)
{
    myValueDialog->SetText("Set voltage", "New value:", "V");
    myValueDialog->SetMaxMinValues(0, 20000);
    myValueDialog->SetValue(vcache.vset);
    myValueDialog->exec();  // modal
    if (myValueDialog->result() == QDialog::Accepted)
    {
        // Temporary
        updateVset(myValueDialog->GetValue());
        updateVmea(vcache.vset);
    }
}

void MainWindow::on_SetCurrentCommand(void)
{
    myValueDialog->SetText("Set current", "New value:", "A");
    myValueDialog->SetMaxMinValues(0, 40000);
    myValueDialog->SetValue(vcache.cset);
    myValueDialog->exec();  // modal
    if (myValueDialog->result() == QDialog::Accepted)
    {
        // Temporary
        updateCset(myValueDialog->GetValue());
        updateCmea(vcache.cset);
    }
}

