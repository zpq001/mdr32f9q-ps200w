#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingshelper.h"
#include <QSettings>
#include <QMessageBox>
#include <QThread>

//#include "serialworker.h"
#include "serialtop.h"
#include "globaldef.h"

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


/*
 * TODO:
 *  Disable and enable widgets according to connected/disconnected state
 *
 * */



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create instances
    myValueDialog = new SingleValueDialog(this);
    mySettingsDialog = new SettingsDialog(this);
    keyWindow = new KeyWindow(this);
    serialStatusLabel = new QLabel(this);
    serialBytesReceivedLabel = new QLabel(this);
    serialBytesTransmittedLabel = new QLabel(this);

    rxBytesCounter = 0;
    txBytesCounter = 0;

    ui->statusBar->addWidget(serialStatusLabel, 30);
    ui->statusBar->addWidget(serialBytesReceivedLabel, 20);
    ui->statusBar->addWidget(serialBytesTransmittedLabel, 20);

    ui->logViewer->addText(LogViewer::prefixThreadId("Main thread"), LogViewer::LogThreadId);

    // Create top-level device controller and move it to another thread
    QThread *newThread = new QThread;
    topController = new SerialTop();


    // Setup signals
    connect(newThread, SIGNAL(started()), topController, SLOT(init()));
    connect(topController, SIGNAL(connectedChanged(bool)), this, SLOT(on_ConnectedChanged(bool)));
    connect(topController, SIGNAL(_log(QString,int)), ui->logViewer, SLOT(addText(QString,int)));
    connect(topController, SIGNAL(bytesReceived(int)), this, SLOT(onBytesReceived(int)));
    connect(topController, SIGNAL(bytesTransmitted(int)), this, SLOT(onBytesTransmitted(int)));
    connect(topController, SIGNAL(updVmea(int)), this, SLOT(updateVmea(int)));
    connect(topController, SIGNAL(updCmea(int)), this, SLOT(updateCmea(int)));
    connect(topController, SIGNAL(updPmea(int)), this, SLOT(updatePmea(int)));
    connect(topController, SIGNAL(updState(int)), this, SLOT(updateState(int)));
    connect(topController, SIGNAL(updChannel(int)), this, SLOT(updateChannel(int)));
    connect(topController, SIGNAL(updCurrentRange(int)), this, SLOT(updateCurrentRange(int)));
    connect(topController, SIGNAL(updVset(int)), this, SLOT(updateVset(int)));
    connect(topController, SIGNAL(updCset(int)), this, SLOT(updateCset(int)));

    connect(ui->actionConnect, SIGNAL(triggered()), topController, SLOT(connectToDevice()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), topController, SLOT(disconnectFromDevice()));
    connect(this, SIGNAL(sendString(QString)), topController, SLOT(sendString(QString)));
    connect(keyWindow, SIGNAL(KeyEvent(int,int)), topController, SLOT(keyEvent(int,int)));
    connect(ui->comboBox_Verbose, SIGNAL(currentIndexChanged(int)), topController, SLOT(setVerboseLevel(int)));

    topController->moveToThread(newThread);
    // Start second thread
    newThread->start();

    connect (ui->actionSettings, SIGNAL(triggered()), mySettingsDialog, SLOT(exec()));
    connect (ui->actionKeyboard, SIGNAL(triggered()), this, SLOT(showKeyWindow()));
    connect(ui->stateOnBtn, SIGNAL(clicked()), this, SLOT(on_SetStateOnCommand()));
    connect(ui->stateOffBtn, SIGNAL(clicked()), this, SLOT(on_SetStateOffCommand()));
    connect(ui->setCurrentRangeBtn, SIGNAL(clicked()), this, SLOT(on_SetCurrentRangeCommand()));
    connect(ui->setVoltageBtn, SIGNAL(clicked()), this, SLOT(on_SetVoltageCommand()));
    connect(ui->setCurrentBtn, SIGNAL(clicked()), this, SLOT(on_SetCurrentCommand()));
    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendTxWindowData()));


    ui->sendButton->setShortcut(Qt::Key_Return | Qt::ControlModifier);

    // Sync value cache
    memset(&vcache, 0, sizeof(vcache));

    updateVmea(vcache.vmea);
    updateCmea(vcache.cmea);

    updateState(CONVERTER_OFF);
    updateChannel(CHANNEL_5V);
    updateCurrentRange(CURRENT_RANGE_LOW);
    updateVset(0);
    updateCset(0);


    // Default widgets state
    ui->actionDisconnect->setEnabled(false);
    on_ConnectedChanged(false);

    onBytesReceived(0);
    onBytesTransmitted(0);

    // Make sure all settings are valid
    if (SettingsHelper::validateSettings() == false)
    {
        // Show messagebox!
        QMessageBox::information(this, "Information", "Some setting values are missing or corrupted. Defaults are loaded.", QMessageBox::Ok);
    }

    QThread::sleep(20);
    topController->setVerboseLevel(ui->comboBox_Verbose->currentIndex());
    ui->logViewer->addText("Started!", 0);

}

MainWindow::~MainWindow()
{
    appSettings.sync();
    delete ui;
}


void MainWindow::onBytesReceived(int count)
{
    rxBytesCounter += count;
    serialBytesReceivedLabel->setText("Rx: ");
    serialBytesReceivedLabel->setText( serialBytesReceivedLabel->text().append(QString::number(rxBytesCounter)) );
}

void MainWindow::onBytesTransmitted(int count)
{
    txBytesCounter += count;
    serialBytesTransmittedLabel->setText("Tx: ");
    serialBytesTransmittedLabel->setText( serialBytesTransmittedLabel->text().append(QString::number(txBytesCounter)) );
}




void MainWindow::updateVmea(int value)
{
    QString str;
    str.sprintf("%1.2fV", (double)value/1000);
    ui->label_vmea->setText(str);
    vcache.vmea = value;
}


void MainWindow::updateCmea(int value)
{
    QString str;
    str.sprintf("%1.2fA", (double)value/1000);
    ui->label_cmea->setText(str);
    vcache.cmea = value;
}

void MainWindow::updatePmea(int value)
{
    QString str;
    str.sprintf("%1.2fW", (double)value/1000);
    ui->label_pmea->setText(str);
    vcache.pmea = value;
}


/*
void MainWindow::updateState(int state)
{
    ui->label_state->setText((state == CONVERTER_ON) ? "On" : "Off");
}

void MainWindow::updateChannel(int channel)
{
    vcache.channel = channel;
    // invalidate_controls(); -TODO
    ui->label_channel->setText((channel == CHANNEL_5V) ? "5V" : "12V");
}

void MainWindow::updateCurrentRange(int channel, int currentRange)
{
    vcache.currentRange = currentRange;
    ui->label_crange->setText((currentRange == CURRENT_RANGE_LOW) ? "20A" : "40A");
}

void MainWindow::updateVset(int channel, int value)
{
    QString str;
    str.sprintf("%1.2fV", (double)value/1000);
    ui->label_vset->setText(str);
    vcache.vset = value;
}

void MainWindow::updateCset(int channel, int currentRange, int value)
{

    QString str;
    str.sprintf("%1.2fA", (double)value/1000);
    ui->label_cset->setText(str);
    vcache.cset = value;
}
*/

void MainWindow::updateState(int state)
{
    ui->label_state->setText((state == CONVERTER_ON) ? "On" : "Off");
}

void MainWindow::updateChannel(int channel)
{
    vcache.channel = channel;
    // invalidate_controls(); -TODO
    ui->label_channel->setText((channel == CHANNEL_5V) ? "5V" : "12V");
}

void MainWindow::updateCurrentRange(int currentRange)
{
    vcache.currentRange = currentRange;
    ui->label_crange->setText((currentRange == CURRENT_RANGE_LOW) ? "20A" : "40A");
}

void MainWindow::updateVset(int value)
{
    QString str;
    str.sprintf("%1.2fV", (double)value/1000);
    ui->label_vset->setText(str);
    vcache.vset = value;
}

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

void MainWindow::sendTxWindowData()
{
    QString text = ui->sendTextEdit->toPlainText();
    if (!text.isEmpty())
    {
        if (ui->sendCRLFCheckBox->isChecked())
            text.append("\r");
        //for (int i=0; i<9; i++)
            emit sendString(text);
        ui->sendTextEdit->clearAndAddToHistory();
    }
}

// Virtual keyboard window
void MainWindow::showKeyWindow(void)
{
    if (keyWindow->isVisible() == false)
        keyWindow->show();
    else
        keyWindow->activateWindow();
}


void MainWindow::on_SetStateOnCommand(void)
{
    topController->setState(CONVERTER_ON);
}

void MainWindow::on_SetStateOffCommand(void)
{
    topController->setState(CONVERTER_OFF);
}

void MainWindow::on_SetCurrentRangeCommand(void)
{
    // CHECKME - use cache for additional params to prevent their change while dialog is active
    // TODO: dialog
    if (vcache.currentRange == CURRENT_RANGE_LOW)
        topController->setCurrentRange(vcache.channel, CURRENT_RANGE_HIGH);
    else
        topController->setCurrentRange(vcache.channel, CURRENT_RANGE_LOW);
}

void MainWindow::on_SetVoltageCommand(void)
{
    myValueDialog->SetText("Set voltage", "New value:", "V");
    myValueDialog->SetMaxMinValues(0, 20000);
    myValueDialog->SetValue(vcache.vset);
    myValueDialog->exec();  // modal
    if (myValueDialog->result() == QDialog::Accepted)
    {
        // CHECKME - use cache for additional params to prevent their change while dialog is active
        topController->setVoltage(vcache.channel, myValueDialog->GetValue());
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
        // CHECKME - use cache for additional params to prevent their change while dialog is active
        topController->setCurrent(vcache.channel, vcache.currentRange, myValueDialog->GetValue());
    }
}



