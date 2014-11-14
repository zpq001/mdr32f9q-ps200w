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
    connect(worker, SIGNAL(updState(int)), this, SLOT(onWorkerUpdState(int)));
    //connect(worker, SIGNAL(updChannel(int)), this, SLOT(onWorkerUpdChannel(int)));
    //connect(worker, SIGNAL(updCurrentRange(int,int)), this, SLOT(onWorkerUpdCurrentRange(int,int)));
    //connect(worker, SIGNAL(updVset(int,int)), this, SLOT(onWorkerUpdVset(int,int)));
    //connect(worker, SIGNAL(updCset(int,int,int)), this, SLOT(onWorkerUpdCset(int,int,int)));

    connect(worker, SIGNAL(log(int,QString)), this, SLOT(onWorkerLog(int,QString)), Qt::DirectConnection);
    connect(worker, SIGNAL(logTx(const char*,int)), this, SLOT(onPortTxLog(const char*,int)), Qt::DirectConnection);
    connect(worker, SIGNAL(logRx(const char*,int)), this, SLOT(onPortRxLog(const char*,int)), Qt::DirectConnection);
    connect(worker, SIGNAL(bytesTransmitted(int)), this, SIGNAL(bytesTransmitted(int)), Qt::DirectConnection);
    connect(worker, SIGNAL(bytesReceived(int)), this, SIGNAL(bytesReceived(int)), Qt::DirectConnection);

    emit _log( LogViewer::prefixThreadId("Serial top"), LogViewer::LogThreadId);
    workerThread->start();


    connect(this, SIGNAL(signal_ProcessTaskQueue()), this, SLOT(_processTaskQueue()), Qt::QueuedConnection);


    vcache.channel = -1;
    vcache.currentRange = -1;
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
            ReadTaskArgs_t *a = (ReadTaskArgs_t *)malloc(sizeof(ReadTaskArgs_t));
            a->state = true;
            a->channel = true;
            a->currentRange = true;
            a->voltageSetting = true;
            a->currentSetting = true;
            _invokeTaskQueued(&SerialTop::_readTask, a);
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
void SerialTop::setState(int state)
{
    SerialWorker::argsState_t *args = (SerialWorker::argsState_t *)malloc(sizeof(SerialWorker::argsState_t));
    args->newValue = state;
    _invokeTaskQueued(&SerialTop::_setState, args);
}

// Can be signal-slot or direct call from other thread
void SerialTop::setCurrentRange(int channel, int value)
{
    SerialWorker::argsCurrentRange_t *args = (SerialWorker::argsCurrentRange_t *)malloc(sizeof(SerialWorker::argsCurrentRange_t));
    args->channel = channel;
    args->newValue = value;
    _invokeTaskQueued(&SerialTop::_setCurrentRange, args);
}

// Can be signal-slot or direct call from other thread
void SerialTop::setVoltage(int channel, int value)
{
    SerialWorker::argsVset_t *args = (SerialWorker::argsVset_t *)malloc(sizeof(SerialWorker::argsVset_t));
    args->channel = channel;
    args->newValue = value;
    _invokeTaskQueued(&SerialTop::_setVoltage, args);
}

// Can be signal-slot or direct call from other thread
void SerialTop::setCurrent(int channel, int currentRange, int value)
{
    SerialWorker::argsCset_t *args = (SerialWorker::argsCset_t *)malloc(sizeof(SerialWorker::argsCset_t));
    args->channel = channel;
    args->currentRange = currentRange;
    args->newValue = value;
    _invokeTaskQueued(&SerialTop::_setCurrent, args);
}





// Helper function
void SerialTop::_invokeTaskQueued(TaskPointer fptr, void *arguments, bool insertToFront)
{
    TaskQueueRecord_t record = {fptr, arguments};
    taskQueueMutex.lock();
    if (insertToFront)
        taskQueue.insert(0,record);
    else
        taskQueue.append(record);
    taskQueueMutex.unlock();
    emit signal_ProcessTaskQueue();
}

// Helper function
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
//-----------------------------------------------------------------//
//-----------------------------------------------------------------//
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

}

//-----------------------------------------------------------------//
// Read task

void SerialTop::_readTask(void *arguments)
{
    QEventLoop localLoop;
    connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
    ReadTaskArgs_t *a = (ReadTaskArgs_t *)arguments;
    // Using loop just for convinient exit
    while (1)
    {
        if (a->state)
        {
            SerialWorker::argsState_t stateArgs;
            worker->getState(&stateArgs);
            localLoop.exec();
            if (stateArgs.errCode != SerialWorker::noError)
            {
                emit _log("State read error", LogViewer::LogErr);
                break;
            }
            emit updState(stateArgs.result);
        }

        if (a->channel)
        {
            SerialWorker::argsChannel_t channelArgs;
            worker->getChannel(&channelArgs);
            localLoop.exec();
            if (channelArgs.errCode != SerialWorker::noError)
            {
                emit _log("Channel selection read error", LogViewer::LogErr);
                break;
            }
            vcache.channel = channelArgs.result;
            emit updChannel(channelArgs.result);
        }

        if (a->currentRange)
        {
            SerialWorker::argsCurrentRange_t crangeArgs;
            crangeArgs.channel = vcache.channel;
            worker->getCurrentRange(&crangeArgs);
            localLoop.exec();
            if (crangeArgs.errCode != SerialWorker::noError)
            {
                emit _log("Current range read error", LogViewer::LogErr);
                break;
            }
            vcache.currentRange = crangeArgs.result;
            emit updCurrentRange(crangeArgs.result);
        }

        if (a->voltageSetting)
        {
            SerialWorker::argsVset_t vsetArgs;
            vsetArgs.channel = vcache.channel;
            worker->getVoltageSetting(&vsetArgs);
            localLoop.exec();
            if (vsetArgs.errCode != SerialWorker::noError)
            {
                emit _log("Voltage setting read error", LogViewer::LogErr);
                break;
            }
            vcache.vset = vsetArgs.result;
            emit updVset(vsetArgs.result);
        }

        if (a->currentSetting)
        {
            SerialWorker::argsCset_t csetArgs;
            csetArgs.channel = vcache.channel;
            csetArgs.currentRange = vcache.currentRange;
            worker->getCurrentSetting(&csetArgs);
            localLoop.exec();
            if (csetArgs.errCode != SerialWorker::noError)
            {
                emit _log("Current setting read error", LogViewer::LogErr);
                break;
            }
            emit updCset(csetArgs.result);
            vcache.cset = csetArgs.result;
        }
        break;
    }
    delete a;
}


//-----------------------------------------------------------------//
// Settings

void SerialTop::_setState(void *arguments)
{
    QEventLoop localLoop;
    SerialWorker::argsState_t *a = (SerialWorker::argsState_t *)arguments;
    if (!_checkConnected()) return;
    connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
    worker->setState(a);
    localLoop.exec();
    if (a->errCode == SerialWorker::noError)
    {
        emit updState(a->result);
    }
    else
    {
        emit _log("Set state error", LogViewer::LogErr);
    }
    delete a;
}

void SerialTop::_setCurrentRange(void *arguments)
{
    QEventLoop localLoop;
    SerialWorker::argsCurrentRange_t *a = (SerialWorker::argsCurrentRange_t *)arguments;
    if (_checkConnected())
    {
        connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
        worker->setCurrentRange(a);
        localLoop.exec();
        if (a->errCode == SerialWorker::noError)
        {
            if (a->channel == vcache.channel)
            {
                if (a->result != vcache.currentRange)
                {
                    vcache.currentRange = a->result;
                    emit updCurrentRange(a->result);
                    // Read everything that is affected by parameter
                    ReadTaskArgs_t *a = (ReadTaskArgs_t *)malloc(sizeof(ReadTaskArgs_t));
                    a->state = false;
                    a->channel = false;
                    a->currentRange = false;
                    a->voltageSetting = false;
                    a->currentSetting = true;
                    _invokeTaskQueued(&SerialTop::_readTask, a, true);  // first item
                }
                else
                {
                    // could not change parameter
                }
            }
        }
        else
        {
            emit _log("Set current range error", LogViewer::LogErr);
            return;
        }
    }
    delete a;
}

void SerialTop::_setVoltage(void *arguments)
{
    QEventLoop localLoop;
    SerialWorker::argsVset_t *a = (SerialWorker::argsVset_t *)arguments;
    if (_checkConnected())
    {
        connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
        worker->setVoltageSetting(a);
        localLoop.exec();
        if (a->errCode == SerialWorker::noError)
        {
            if (a->channel == vcache.channel)
            {
                emit updVset(a->result);
            }
        }
        else
        {
            emit _log("Set voltage error", LogViewer::LogErr);
            return;
        }
    }
    delete a;
}

void SerialTop::_setCurrent(void *arguments)
{
    QEventLoop localLoop;
    SerialWorker::argsCset_t *a = (SerialWorker::argsCset_t *)arguments;
    if (_checkConnected())
    {
        connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
        worker->setCurrentSetting(a);
        localLoop.exec();
        if (a->errCode == SerialWorker::noError)
        {
            if ((a->channel == vcache.channel) && (a->currentRange == vcache.currentRange))
            emit updCset(a->result);
        }
        else
        {
            emit _log("Set current error", LogViewer::LogErr);
        }
    }
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

void SerialTop::onWorkerUpdState(int state)
{
    emit _log("Updating state", LogViewer::LogInfo);
    emit updState(state);
}
/*
void SerialTop::onWorkerUpdChannel(int channel)
{
    QEventLoop localLoop;
    emit _log("Updating channel", LogViewer::LogInfo);

    // Read all necessary data
    vcache.channel = channel;
    emit updChannel(channel);

    // Current range
    SerialWorker::argsCurrentRange_t crangeArgs;
    crangeArgs.channel = vcache.channel;
    worker->getCurrentRange(&crangeArgs);
    localLoop.exec();
    if (crangeArgs.errCode != SerialWorker::noError)
    {
        emit _log("Current range read error", LogViewer::LogErr);
        return;
    }
    vcache.currentRange = crangeArgs.result;
    emit updCurrentRange(crangeArgs.result);

    // Voltage setting
    SerialWorker::argsVset_t vsetArgs;
    vsetArgs.channel = vcache.channel;
    worker->getVoltageSetting(&vsetArgs);
    localLoop.exec();
    if (vsetArgs.errCode != SerialWorker::noError)
    {
        emit _log("Voltage setting read error", LogViewer::LogErr);
        return;
    }
    emit updVset(vsetArgs.result);

    // Current setting
    SerialWorker::argsCset_t csetArgs;
    csetArgs.channel = vcache.channel;
    csetArgs.currentRange = crangeArgs.result;
    worker->getCurrentSetting(&csetArgs);
    localLoop.exec();
    if (csetArgs.errCode != SerialWorker::noError)
    {
        emit _log("Current setting read error", LogViewer::LogErr);
        return;
    }
    emit updCset(csetArgs.result);

    // State
    SerialWorker::argsState_t stateArgs;
    worker->getState(&stateArgs);
    localLoop.exec();
    if (stateArgs.errCode != SerialWorker::noError)
    {
        emit _log("Channel selection read error", LogViewer::LogErr);
        return;
    }
    emit updState(stateArgs.result);

}

void SerialTop::onWorkerUpdCurrentRange(int channel, int currentRange)
{
    QEventLoop localLoop;
    if (channel == vcache.channel)
    {
        emit _log("Updating current range", LogViewer::LogInfo);
        vcache.currentRange = currentRange;
        emit updCurrentRange(currentRange);

        // Current setting
        SerialWorker::argsCset_t csetArgs;
        csetArgs.channel = vcache.channel;
        csetArgs.currentRange = vcache.currentRange;
        worker->getCurrentSetting(&csetArgs);
        localLoop.exec();
        if (csetArgs.errCode != SerialWorker::noError)
        {
            emit _log("Current setting read error", LogViewer::LogErr);
            return;
        }
        emit updCset(csetArgs.result);
    }
}

void SerialTop::onWorkerUpdVset(int channel, int value)
{
    if (channel == vcache.channel)
    {
        emit _log("Updating voltage setting", LogViewer::LogInfo);
        emit updVset(value);
    }
}

void SerialTop::onWorkerUpdCset(int channel, int currentRange, int value)
{
    if ((channel == vcache.channel) && (currentRange == vcache.currentRange))
    {
        emit _log("Updating current setting", LogViewer::LogInfo);
        emit updCset(value);
    }
}

*/


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







