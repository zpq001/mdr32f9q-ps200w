#include "serialtop.h"
#include "settingshelper.h"
#include "logviewer.h"
#include "keywindow.h"





SerialTop::SerialTop(QObject *parent) :
    QObject(parent)
{
    // Not using constructor because this class is intended to run
    // in separate thread
}


void SerialTop::requestForParam1(int arg1, int arg2)
{
    // Called from other thread
    emit _log( LogViewer::prefixThreadId("aaa"), LogViewer::LogThreadId);

    //Some_fnc_ptr fnc_ptr = &SerialTop::getParam1;
    emit sig_execute(/*fnc_ptr,*/ arg1, arg2);
}



void SerialTop::getParam1(int arg1, int arg2)
{
    emit _log( LogViewer::prefixThreadId("ccc"), LogViewer::LogThreadId);
}


void SerialTop::execute(int arg1, int arg2)
{
    emit _log( LogViewer::prefixThreadId("bbb"), LogViewer::LogThreadId);
    //(this->*ptr)(arg1, arg2);
}







void SerialTop::init(void)
{
    // Here all initialization is done
    connected = false;
    busy = false;

    QThread *workerThread = new QThread();
    worker = new SerialWorker();
    worker->moveToThread(workerThread);
    connect(workerThread, SIGNAL(started()), worker, SLOT(init()));

    connect(worker, SIGNAL(updVmea(int)), this, SIGNAL(updVmea(int)));
    connect(worker, SIGNAL(updCmea(int)), this, SIGNAL(updCmea(int)));
    //connect(worker, SIGNAL(updPmea(int)), this, SIGNAL(up)

    connect(worker, SIGNAL(log(int,QString)), this, SLOT(onWorkerLog(int,QString)), Qt::DirectConnection);
    connect(worker, SIGNAL(logTx(const char*,int)), this, SLOT(onPortTxLog(const char*,int)), Qt::DirectConnection);
    connect(worker, SIGNAL(logRx(const char*,int)), this, SLOT(onPortRxLog(const char*,int)), Qt::DirectConnection);
    connect(worker, SIGNAL(bytesTransmitted(int)), this, SIGNAL(bytesTransmitted(int)), Qt::DirectConnection);
    connect(worker, SIGNAL(bytesReceived(int)), this, SIGNAL(bytesReceived(int)), Qt::DirectConnection);

    emit _log( LogViewer::prefixThreadId("Serial top"), LogViewer::LogThreadId);
    workerThread->start();

    //connect(this, SIGNAL(sig_execute(Some_fnc_ptr,int,int)), this, SLOT(execute(Some_fnc_ptr,int,int)));
    connect(this, SIGNAL(sig_execute(int,int)), this, SLOT(execute(int,int)));

    //requestForParam1(1,2);
}


void SerialTop::connectToDevice(void)
{
    if (connected == false)
    {
        worker->writePortSettings(
                    appSettings.value("serial/port").toString(),
                    appSettings.value("serial/baudrate").toInt(),
                    SettingsHelper::getValue(serial_databits, "serial/databits").toInt(),
                    SettingsHelper::getValue(serial_parity, "serial/parity").toInt(),
                    SettingsHelper::getValue(serial_stopbits, "serial/stopbits").toInt(),
                    SettingsHelper::getValue(serial_flowctrl, "serial/flowctrl").toInt() );
        if (worker->openPort() == SerialWorker::noError)
        {
            connected = true;
            emit connectedChanged(connected);
            //sendString("Hi man!\r");

            // Read data
            SerialWorker::getVoltageSettingArgs_t a;
            a.channel = 0;
            //for (int i = 0; i<20; i++)
            //{
                //worker->getVoltageSetting(&a, false);    // non-blocking request
                worker->getVoltageSetting(&a, true);    // blocking request
            //}
            if (a.errCode == SerialWorker::noError)
                emit updVset(a.result);
            else
                emit _log("Cannot obtain voltage setting from device!", LogViewer::LogInfo);
        }

    }

}


void SerialTop::disconnectFromDevice(void)
{
    worker->closePort();
    connected = false;
    emit connectedChanged(connected);
}

bool SerialTop::checkConnected(void)
{
    if (connected != true)
    {
        emit _log("Port is closed", LogViewer::LogErr);
        return false;
    }
    return connected;
}





//-----------------------------------------------------------------//
// Commands

void SerialTop::setVoltage(int channel, int value)
{
    QEventLoop localLoop;
    SerialWorker::setVoltageSettingArgs_t a;
    if (!connected)
    {
        emit _log("Port is closed", LogViewer::LogErr);
        return;
    }
    if (busy)
    {
        emit _log("Other command in progress", LogViewer::LogErr);
        return;
    }
    busy = true;
    connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
    a.channel = channel;
    a.newValue = value;
    worker->setVoltageSetting(&a);  // non-blocking
    localLoop.exec();
    // Worker has finished (or it's operation was aborted)
    if (a.errCode == SerialWorker::noError)
    {
        // Success!
        emit updVset(a.resultValue);
    }
    else
    {
        // An error happened
        emit _log("Set voltage error", LogViewer::LogErr);
    }
    busy = false;
}












//-----------------------------------------------------------------//
// Misc commands
void SerialTop::sendString(const QString &text)
{
    if (!checkConnected()) return;
    worker->sendString(text);
}

QString SerialTop::getKeyName(int keyId)
{
    QString keyName;
    switch (keyId)
    {
        case KeyWindow::key_OK:
            keyName = "btn_ok";
            break;
        case KeyWindow::key_ESC:
            keyName = "btn_esc";
            break;
        case KeyWindow::key_RIGHT:
            keyName = "btn_right";
            break;
        case KeyWindow::key_LEFT:
            keyName = "btn_left";
            break;
        case KeyWindow::key_ENCODER:
            keyName = "btn_encoder";
            break;
        default:
            keyName = QString::number(keyId);
    }
    return keyName;
}

QString SerialTop::getKeyEventType(int keyEventType)
{
    QString keyAction;
    switch (keyEventType)
    {
        case KeyWindow::event_DOWN:
            keyAction = "down";
            break;
        case KeyWindow::event_UP:
            keyAction = "up";
            break;
        case KeyWindow::event_UP_SHORT:
            keyAction = "up_short";
            break;
        case KeyWindow::event_UP_LONG:
            keyAction = "up_long";
            break;
        case KeyWindow::event_HOLD:
            keyAction = "hold";
            break;
        default:
            keyAction = QString::number(keyEventType);
    }
    return keyAction;
}


void SerialTop::keyEvent(int key, int event)
{
    if (event == KeyWindow::event_REPEAT) return;
    if (!checkConnected()) return;

    QString keyName;
    QString keyAction;

    keyName = getKeyName(key);
    keyAction = getKeyEventType(event);

    QString resultCmd = "key ";
    resultCmd.append(keyAction);
    resultCmd.append(" ");
    resultCmd.append(keyName);
    resultCmd.append('\r');

    worker->sendString(resultCmd);
}


//-----------------------------------------------------------------//
// Slots for worker signals



//-----------------------------------------------------------------//
// Logging

void SerialTop::onWorkerLog(int type, QString text)
{
    int logType;
    switch (type)   // Translate log types
    {
        case SerialWorker::LogErr:
            logType = LogViewer::LogErr;    break;
        case SerialWorker::LogWarn:
            logType = LogViewer::LogWarn;   break;
        case SerialWorker::LogThread:
            logType = LogViewer::LogThreadId;   break;
        default:
            logType = LogViewer::LogInfo;
    }
    emit _log(text, logType);
}

void SerialTop::onPortTxLog(const char *data, int len)
{
    QString text = QString::fromLocal8Bit(data, len);
    emit _log(text, LogViewer::LogTx);
}

void SerialTop::onPortRxLog(const char *data, int len)
{
    QString text = QString::fromLocal8Bit(data, len);
    emit _log(text, LogViewer::LogRx);
}












