#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingshelper.h"
#include <QSettings>
#include <QMessageBox>
#include <QThread>

#include "serialworker.h"
#include "serialtop.h"

/*
 * http://qt-project.org/wiki/Threads_Events_QObjects
 * http://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
 * http://habrahabr.ru/post/150274/
 * http://we.easyelectronics.ru/electro-and-pc/qthread-qserialport-krutim-v-otdelnom-potoke-rabotu-s-som-portom.html
 * http://qt-project.org/doc/qt-4.8/threads-qobject.html#signals-and-slots-across-threads
 * http://habrahabr.ru/post/202312/
 * http://habrahabr.ru/post/203254/
 * http://stackoverflow.com/questions/1764831/c-object-without-new
 */



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create instances
    myValueDialog = new SingleValueDialog(this);
    mySettingsDialog = new SettingsDialog(this);
    serialStatusLabel = new QLabel(this);
    ui->statusBar->addWidget(serialStatusLabel);

    // Create top-level device controller and move it to another thread
    QThread *newThread = new QThread;
    topController = new SerialTop();
    topController->moveToThread(newThread);

    // Setup signals
    connect(newThread, SIGNAL(started()), topController, SLOT(init()));
    connect(topController, SIGNAL(connectedChanged(bool)), this, SLOT(on_ConnectedChanged(bool)));
    connect(topController, SIGNAL(_log(QString,int)), ui->logViewer, SLOT(addText(QString,int)));
    // Start second thread
    newThread->start();

    // crash!
    //connect(topController->worker, SIGNAL(_log(QString,int)), ui->logViewer, SLOT(addText(QString,int)));
    connect(topController, SIGNAL(initDone()), this, SLOT(otherThreadStarted()));

    connect(ui->actionConnect, SIGNAL(triggered()), topController, SLOT(connectToDevice()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), topController, SLOT(disconnectFromDevice()));
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
    on_ConnectedChanged(false);

    // Make sure all settings are valid
    if (SettingsHelper::validateSettings() == false)
    {
        // Show messagebox!
        QMessageBox::information(this, "Information", "Some setting values are missing or corrupted. Defaults are loaded.", QMessageBox::Ok);
    }



    ui->logViewer->addText("Started!", 0);
}

MainWindow::~MainWindow()
{
    appSettings.sync();
    delete ui;
}

void MainWindow::otherThreadStarted(void)
{
    connect(topController->worker, SIGNAL(_log(QString,int)), ui->logViewer, SLOT(addText(QString,int)));
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



void MainWindow::on_Error(QString msg)
{
     QMessageBox::critical(this, "Error", msg, QMessageBox::Ok);
}

void MainWindow::on_ConnectedChanged(bool isConnected)
{
    if (isConnected)
    {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        serialStatusLabel->setText("Connected");
    }
    else
    {
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        serialStatusLabel->setText("Disconnected");
    }
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

