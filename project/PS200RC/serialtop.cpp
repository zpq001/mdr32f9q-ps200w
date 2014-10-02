#include "serialtop.h"
#include "settingshelper.h"
#include "logviewer.h"

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
        if (worker->openPort() == worker->noError)
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

void SerialTop::sendString(const QString &text)
{
    if (!checkConnected()) return;
    worker->sendString(text);
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







