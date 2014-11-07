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
    connected = false;
    processingTask = false;

    QThread *workerThread = new QThread();
    worker = new SerialWorker();
    worker->moveToThread(workerThread);
    connect(workerThread, SIGNAL(started()), worker, SLOT(init()));

    connect(worker, SIGNAL(updVmea(int)), this, SLOT(onWorkerUpdVmea(int)));
    connect(worker, SIGNAL(updCmea(int)), this, SLOT(onWorkerUpdCmea(int)));
    //connect(worker, SIGNAL(updPmea(int)), this, SIGNAL(up)

    connect(worker, SIGNAL(log(int,QString)), this, SLOT(onWorkerLog(int,QString)), Qt::DirectConnection);
    connect(worker, SIGNAL(logTx(const char*,int)), this, SLOT(onPortTxLog(const char*,int)), Qt::DirectConnection);
    connect(worker, SIGNAL(logRx(const char*,int)), this, SLOT(onPortRxLog(const char*,int)), Qt::DirectConnection);
    connect(worker, SIGNAL(bytesTransmitted(int)), this, SIGNAL(bytesTransmitted(int)), Qt::DirectConnection);
    connect(worker, SIGNAL(bytesReceived(int)), this, SIGNAL(bytesReceived(int)), Qt::DirectConnection);

    emit _log( LogViewer::prefixThreadId("Serial top"), LogViewer::LogThreadId);
    workerThread->start();


    connect(this, SIGNAL(signal_ProcessTaskQueue()), this, SLOT(_processTaskQueue()), Qt::QueuedConnection);

}


//-----------------------------------------------------------------//
// Commands
// Where specified signal-slot connection only - direct call from
// other thread may lead to unpredictable behavior.

// Signal-slot only
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

            emit _log("Reading data from device", LogViewer::LogInfo);
            _invokeTaskQueued(&SerialTop::_initialRead, 0);
/*
            // Read data
            SerialWorker::argsVset_t a;
            a.channel = 0;
            //for (int i = 0; i<20; i++)
            //{
                //worker->getVoltageSetting(&a, false);    // non-blocking request
                worker->getVoltageSetting(&a, true);    // blocking request
            //}
            if (a.errCode == SerialWorker::noError)
                emit updVset(a.result);
            else
                emit _log("Cannot obtain voltage setting from device!", LogViewer::LogInfo); */
        }
    }
}

// Signal-slot only
void SerialTop::disconnectFromDevice(void)
{
    worker->closePort();
    connected = false;
    emit connectedChanged(connected);
}

// Signal-slot only
void SerialTop::sendString(const QString &text)
{
    if (!_checkConnected()) return;
    worker->sendString(text);
}

// Signal-slot only
void SerialTop::keyEvent(int key, int event)
{
    if (event == KeyWindow::event_REPEAT) return;
    if (!_checkConnected()) return;

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


// Can be signal-slot or direct call from other thread
void SerialTop::setVoltage(int channel, int value)
{
    SerialWorker::argsVset_t *args = (SerialWorker::argsVset_t *)malloc(sizeof(SerialWorker::argsVset_t));
    args->channel = channel;
    args->newValue = value;
    _invokeTaskQueued(&SerialTop::_setVoltage, args);
}



void SerialTop::_invokeTaskQueued(TaskPointer fptr, void *arguments)
{
    TaskQueueRecord_t record = {fptr, arguments};
    taskQueueMutex.lock();
    taskQueue.append(record);
    taskQueueMutex.unlock();
    emit signal_ProcessTaskQueue();
}


bool SerialTop::_checkConnected(void)
{
    if (connected != true)
    {
        emit _log("Port is closed", LogViewer::LogErr);
        return false;
    }
    return connected;
}


//-----------------------------------------------------------------//
// Internal queue processing


void SerialTop::_processTaskQueue(void)
{
    TaskQueueRecord_t taskRecord;
    // Protect from re-entrance
    if (!processingTask)
    {
        taskQueueMutex.lock();
        if (taskQueue.count() > 0)
        {
            taskRecord = taskQueue.dequeue();
            processingTask = true;
        }
        taskQueueMutex.unlock();
        if (processingTask)
        {
            // Tasks invoke a local event loop, so this is a blocking call
            (this->*taskRecord.fptr)(taskRecord.arg);
            // Here task is done
            processingTask = false;
            // Call self again until there are no tasks in the queue
            QMetaObject::invokeMethod(this, "_processTaskQueue", Qt::QueuedConnection);
        }
    }
}


void SerialTop::_initialRead(void *arguments)
{
    Q_UNUSED(arguments)     // do not complain
    QEventLoop localLoop;
    connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));

    // First need to read channel!
    SerialWorker::argsChannel_t channelArgs;
    worker->getChannel(&channelArgs);
    localLoop.exec();
    if (channelArgs.errCode != SerialWorker::noError)
    {
        emit _log("Channel selection read error", LogViewer::LogErr);
        return;
    }
    // emit updChannel(a.result);

    SerialWorker::argsVset_t a;
    a.channel = 0;
    worker->getVoltageSetting(&a);
    localLoop.exec();
    if (a.errCode != SerialWorker::noError)
    {
        emit _log("Read voltage setting error", LogViewer::LogErr);
        return;
    }
}



void SerialTop::_setVoltage(void *arguments)
{
    QEventLoop localLoop;
    SerialWorker::argsVset_t *a = (SerialWorker::argsVset_t *)arguments;
    if (!_checkConnected()) return;
    connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
    worker->setVoltageSetting(a);  // non-blocking
    localLoop.exec();
    // Worker has finished (or it's operation was aborted)
    if (a->errCode == SerialWorker::noError)
    {
        // Success!
        emit updVset(a->result);
    }
    else
    {
        // An error happened
        emit _log("Set voltage error", LogViewer::LogErr);
    }

    // Delete arguments
    delete a;
}




//-----------------------------------------------------------------//
// Worker events
// Re-emitting signals guarantees correct time order

void SerialTop::onWorkerUpdVmea(int v)
{
    emit _log("Updating measured voltage", LogViewer::LogInfo);
    emit updVmea(v);
}

void SerialTop::onWorkerUpdCmea(int v)
{
    emit _log("Updating measured current", LogViewer::LogInfo);
    emit updCmea(v);
}

void SerialTop::onWorkerUpdPmea(int v)
{
    emit _log("Updating measured instant power", LogViewer::LogInfo);
    emit updPmea(v);
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



//-----------------------------------------------------------------//
// Misc

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







