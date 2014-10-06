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

void SerialTop::init(void)
{
    // Here all initialization is done
    worker = new SerialWorker();
    connect(worker, SIGNAL(_log(int,QString)), this, SLOT(onWorkerLog(int,QString)));
    connect(worker, SIGNAL(_logTx(const char*,int)), this, SLOT(onPortTxLog(const char*,int)));
    connect(worker, SIGNAL(_logRx(const char*,int)), this, SLOT(onPortRxLog(const char*,int)));
}


void SerialTop::connectToDevice(void)
{
    if (worker->connected == false)
    {
        settingsMutex.lock();
        worker->writePortSettings(
                    appSettings.value("serial/port").toString(),
                    appSettings.value("serial/baudrate").toInt(),
                    SettingsHelper::getValue(serial_databits, "serial/databits").toInt(),
                    SettingsHelper::getValue(serial_parity, "serial/parity").toInt(),
                    SettingsHelper::getValue(serial_stopbits, "serial/stopbits").toInt(),
                    SettingsHelper::getValue(serial_flowctrl, "serial/flowctrl").toInt() );
        settingsMutex.unlock();
        if (worker->openPort() == SerialWorker::noError)
        {
            emit connectedChanged(true);
        }
        // Read data
        //sendString("Hi man!\r\n");
    }
}


void SerialTop::disconnectFromDevice(void)
{
    if (worker->connected == true)
    {
        worker->closePort();
        emit connectedChanged(false);
    }
}

bool SerialTop::checkConnected(void)
{
    if (worker->connected != true)
    {
        emit _log("Port is closed", LogViewer::LogErr);
        return false;
    }
    return true;
}

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

//-----------------------------------------------------------------//
// Commands
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














