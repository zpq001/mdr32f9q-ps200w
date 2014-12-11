#include "serialtop.h"
#include "settingshelper.h"
#include "logviewer.h"
#include "keywindow.h"
#include "globaldef.h"
#include "uart_proto.h"

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
    connect(worker, SIGNAL(updPmea(int)), this, SLOT(onWorkerUpdPmea(int)));
    connect(worker, SIGNAL(updState(int)), this, SLOT(onWorkerUpdState(int)));
    connect(worker, SIGNAL(updChannel(int)), this, SLOT(onWorkerUpdChannel(int)));
    connect(worker, SIGNAL(updCurrentRange(int,int)), this, SLOT(onWorkerUpdCurrentRange(int,int)));
    connect(worker, SIGNAL(updVset(int,int)), this, SLOT(onWorkerUpdVset(int,int)));
    connect(worker, SIGNAL(updCset(int,int,int)), this, SLOT(onWorkerUpdCset(int,int,int)));

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

            // Update everything
            cache.markUpdate();
            _invokeTaskQueued(&SerialTop::_updCache, 0);
            // Update GUI
            _invokeTaskQueued(&SerialTop::_updateTopGui, (void *)UPD_ALL);
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

// Can be signal-slot or direct call from other thread
void SerialTop::setVerboseLevel(int level)
{
    worker->setVerboseLevel(level);
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



    QString resultCmd = proto.groups.buttons;
    resultCmd.append(proto.spaceSymbol);
    resultCmd.append(proto.keys.btn_event);
    resultCmd.append(proto.spaceSymbol);
    resultCmd.append(keyAction);
    resultCmd.append(proto.spaceSymbol);
    resultCmd.append(proto.keys.btn_code);
    resultCmd.append(proto.spaceSymbol);
    resultCmd.append(keyName);
    resultCmd.append(proto.termSymbol);

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



//-----------------------------------------------------------------//
// Read task


void SerialTop::_updCache(void *arguments)
{
    bool exit = false;
    QEventLoop localLoop;
    connect(worker, SIGNAL(operationDone()), &localLoop, SLOT(quit()));
    // Using loop just for convinient exit
    while (!exit)
    {
        if (cache.state.updReq)
        {
            SerialWorker::argsState_t stateArgs;
            worker->getState(&stateArgs);
            localLoop.exec();
            if (stateArgs.errCode == SerialWorker::noError)
            {
                cache.state.set(stateArgs.result);
            }
            else
            {
                cache.state.setInvalid();
                emit _log("State read error", LogViewer::LogErr);
                exit = true;
                continue;
            }
        }

        if (cache.activeChannel.updReq)
        {
            SerialWorker::argsChannel_t channelArgs;
            worker->getChannel(&channelArgs);
            localLoop.exec();
            if (channelArgs.errCode == SerialWorker::noError)
            {
                cache.activeChannel.set(channelArgs.result);
            }
            else
            {
                cache.activeChannel.setInvalid();
                emit _log("Channel selection read error", LogViewer::LogErr);
                exit = true;
                continue;
            }
        }

        //--------- Channel loop ---------//
        cache_channel_t *c;
        cache_crange_t *r;
        for (int i=0; (i<2) && (!exit); i++)
        {
            c = (i==0) ? &cache.ch5v : &cache.ch12v;
            if (c->vset.updReq)
            {
                SerialWorker::argsVset_t vsetArgs;
                vsetArgs.channel = c->chSpec;
                worker->getVoltageSetting(&vsetArgs);
                localLoop.exec();
                if (vsetArgs.errCode == SerialWorker::noError)
                {
                    c->vset.set(vsetArgs.result);
                }
                else
                {
                    c->vset.setInvalid();
                    emit _log("Voltage setting read error", LogViewer::LogErr);
                    exit = true;
                    continue;
                }
            }

            if (c->activeCrange.updReq)
            {
                SerialWorker::argsCurrentRange_t crangeArgs;
                crangeArgs.channel = c->chSpec;
                worker->getCurrentRange(&crangeArgs);
                localLoop.exec();
                if (crangeArgs.errCode == SerialWorker::noError)
                {
                    c->activeCrange.set(crangeArgs.result);
                }
                else
                {
                    c->activeCrange.setInvalid();
                    emit _log("Current range read error", LogViewer::LogErr);
                    exit = true;
                    continue;
                }
            }

            //--------- CRange loop ---------//
            for (int j=0; (j<2) && (!exit); j++)
            {
                r = (j==0) ? &c->crangeLow : &c->crangeHigh;
                //r->cset.markUpdate();       // get current setting for each current range for channel
                if (r->cset.updReq)
                {
                    SerialWorker::argsCset_t csetArgs;
                    csetArgs.channel = c->chSpec;
                    csetArgs.currentRange = r->rangeSpec;
                    worker->getCurrentSetting(&csetArgs);
                    localLoop.exec();
                    if (csetArgs.errCode == SerialWorker::noError)
                    {
                        r->cset.set(csetArgs.result);
                    }
                    else
                    {
                        r->cset.setInvalid();
                        emit _log("Current setting read error", LogViewer::LogErr);
                        exit = true;
                        continue;
                    }
                }
            }
        }
        exit = true;
    }
}


void SerialTop::_updateTopGui(void *arguments)
{
    int f = (int)arguments;
    cache_channel_t *c = cache.getActiveChannel();
    if (f & UPD_STATE)      emit updState(cache.state.val);
    if (f & UPD_CHANNEL)    emit updChannel(cache.activeChannel.val);
    if (f & UPD_CRANGE)     emit updCurrentRange(c->activeCrange.val);
    if (f & UPD_VSET)       emit updVset(c->vset.val);
    if (f & UPD_CSET)       emit updCset(c->getActiveCrange()->cset.val);
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
        // Save result
        cache.state.set(a->result);
        // Update GUI if required
        _updateTopGui((void *)(UPD_STATE));
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
            // Save result
            cache_channel_t *c = (a->channel == CHANNEL_5V) ? &cache.ch5v : &cache.ch12v;
            c->activeCrange.set(a->result);
            // Update GUI if required
            if (a->channel == cache.activeChannel.val)
            {
                _updateTopGui((void *)(UPD_CRANGE | UPD_CSET));
            }
        }
        else
        {
            emit _log("Set current range error", LogViewer::LogErr);
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
            // Save result
            cache_channel_t *c = (a->channel == CHANNEL_5V) ? &cache.ch5v : &cache.ch12v;
            c->vset.set(a->result);
            // Update GUI if required
            if (a->channel == cache.activeChannel.val)
            {
                _updateTopGui((void *)(UPD_VSET));
            }
        }
        else
        {
            emit _log("Set voltage error", LogViewer::LogErr);
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
            // Save result
            cache_channel_t *c = (a->channel == CHANNEL_5V) ? &cache.ch5v : &cache.ch12v;
            cache_crange_t *r = (a->currentRange == CURRENT_RANGE_LOW) ? &c->crangeLow : &c->crangeHigh;
            r->cset.set(a->result);
            // Update GUI if required
            if ((a->channel == cache.activeChannel.val) && (a->currentRange == cache.getActiveChannel()->activeCrange.val))
            {
                _updateTopGui((void *)(UPD_CSET));
            }
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
    //emit _log("Updating measured voltage", LogViewer::LogInfo);
    emit updVmea(v);
}

void SerialTop::onWorkerUpdCmea(int v)
{
    //emit _log("Updating measured current", LogViewer::LogInfo);
    emit updCmea(v);
}

void SerialTop::onWorkerUpdPmea(int v)
{
    //emit _log("Updating measured instant power", LogViewer::LogInfo);
    emit updPmea(v);
}

void SerialTop::onWorkerUpdState(int state)
{
    //emit _log("Updating state", LogViewer::LogInfo);
    emit updState(state);
}

void SerialTop::onWorkerUpdChannel(int channel)
{
    //emit _log("Updating channel", LogViewer::LogInfo);
    cache.activeChannel.set(channel);
    _updateTopGui((void *)(UPD_CHANNEL | UPD_CRANGE | UPD_VSET | UPD_CSET));
}


void SerialTop::onWorkerUpdCurrentRange(int channel, int currentRange)
{
    cache_channel_t *c = cache.getChannel(channel);
    c->activeCrange.set(currentRange);
    if (channel == cache.activeChannel.val)
    {
        _updateTopGui((void *)(UPD_CRANGE | UPD_CSET));
    }
}

void SerialTop::onWorkerUpdVset(int channel, int value)
{
    cache_channel_t *c = cache.getChannel(channel);
    c->vset.set(value);
    if (channel == cache.activeChannel.val)
    {
        _updateTopGui((void *)(UPD_VSET));
    }
}

void SerialTop::onWorkerUpdCset(int channel, int currentRange, int value)
{
    cache_channel_t *c = cache.getChannel(channel);
    cache_crange_t *r = c->getCrange(currentRange);
    r->cset.set(value);
    if ((channel == cache.activeChannel.val) &&
        (currentRange == cache.getActiveChannel()->activeCrange.val))
    {
        _updateTopGui((void *)(UPD_CSET));
    }
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
            keyName = proto.button_codes.ok;
            break;
        case KeyWindow::key_ESC:
            keyName = proto.button_codes.esc;
            break;
        case KeyWindow::key_RIGHT:
            keyName = proto.button_codes.right;
            break;
        case KeyWindow::key_LEFT:
            keyName = proto.button_codes.left;
            break;
        case KeyWindow::key_ENCODER:
            keyName = proto.button_codes.encoder;
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
            keyAction = proto.button_events.down;
            break;
        case KeyWindow::event_UP:
            keyAction = proto.button_events.up;
            break;
        case KeyWindow::event_UP_SHORT:
            keyAction = proto.button_events.up_short;
            break;
        case KeyWindow::event_UP_LONG:
            keyAction = proto.button_events.up_long;
            break;
        case KeyWindow::event_HOLD:
            keyAction = proto.button_events.hold;
            break;
        default:
            keyAction = QString::number(keyEventType);
    }
    return keyAction;
}







