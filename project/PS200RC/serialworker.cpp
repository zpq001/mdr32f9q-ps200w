#include <QEventLoop>
#include <QSignalMapper>
#include <QCoreApplication>

#include "serialworker.h"
#include "logviewer.h"
#include "globaldef.h"
#include "serialparser.h"
#include "uart_proto.h"

SerialWorker::SerialWorker()
{
}

void SerialWorker::init()
{
    serialPort = new QSerialPort(this);
    portWriteTimer = new QTimer(this);

    connect(this, SIGNAL(signal_openPort(QSemaphore*,openPortArgs_t*)), this, SLOT(_openPort(QSemaphore*,openPortArgs_t*)));
    connect(this, SIGNAL(signal_closePort(QSemaphore*)), this, SLOT(_closePort(QSemaphore*)));
    connect(this, SIGNAL(signal_setVerboseLevel(int)), this, SLOT(_setVerboseLevel(int)));
    connect(this, SIGNAL(signal_ProcessTaskQueue()), this, SLOT(_processTaskQueue()), Qt::QueuedConnection);
    connect(this, SIGNAL(signal_ForceReadSerialPort()), this, SLOT(_readSerialPort()), Qt::QueuedConnection);
    connect(this, SIGNAL(signal_infoReceived()), this, SLOT(_infoPacketHandler()), Qt::QueuedConnection);
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()), Qt::QueuedConnection);
    connect(serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(_transmitDone(qint64)));
    connect(serialPort, SIGNAL(bytesWritten(qint64)), portWriteTimer, SLOT(stop()));    // stop timer when data is written
    //connect(this, SIGNAL(signal_Terminate()), portWriteTimer, SLOT(stop()));            // stop timer when port is closed
    connect(portWriteTimer, SIGNAL(timeout()), this, SLOT(_portWriteTimeout()));

    verboseLevel = 2;

    portWriteTimer->setSingleShot(true);

    rx.state = RSTATE_READING_NEW_DATA;
    rx.index = 0;
    rx.receiveSlotConnected = true;
    // not expecting ACK
    ackRequired = false;

    log(LogThread, LogViewer::prefixThreadId("Serial worker"));
}

// Blocking port open
int SerialWorker::openPort(void)
{
    QSemaphore sem;
    openPortArgs_t a;
    emit signal_openPort(&sem, &a);
    sem.acquire();
    return a.errCode;
}

// Blocking port close
void SerialWorker::closePort(void)
{
    QSemaphore sem;
    emit signal_closePort(&sem);
    sem.acquire();
}

// Settings write
void SerialWorker::writePortSettings(const QString &name, int baudRate, int dataBits, int parity, int stopBits, int flowControl)
{
    QMutexLocker locker(&portSettingsMutex);
    portSettings.name = name;
    portSettings.baudRate = (QSerialPort::BaudRate) baudRate;
    portSettings.dataBits = (QSerialPort::DataBits) dataBits;
    portSettings.parity = (QSerialPort::Parity) parity;
    portSettings.stopBits = (QSerialPort::StopBits) stopBits;
    portSettings.flowControl = (QSerialPort::FlowControl) flowControl;
}

// Serial port error recovery
int SerialWorker::getPortErrorCode(void)
{
    QMutexLocker locker(&portErrorDataMutex);
    return serialPort->error();
}

QString SerialWorker::getPortErrorString(void)
{
    QMutexLocker locker(&portErrorDataMutex);
    return serialPort->errorString();
}

void SerialWorker::setVerboseLevel(int level)
{
    emit signal_setVerboseLevel(level);
}


//-------------------------------------------------------//
/// @brief Sends a string to the device
/// @param[in] text string
/// @note If call is not blocking, signal operationDone() is
/// emitted when done.
/// @return none
//-------------------------------------------------------//
void SerialWorker::sendString(const QString &text, bool blocking)
{
    sendDataArgs_t *args = (sendDataArgs_t *)malloc(sizeof(sendDataArgs_t));
    args->len = text.length();
    args->data = (char *)malloc(args->len);
    memcpy(args->data, text.toLocal8Bit().data(), args->len);
    _invokeTaskQueued(&SerialWorker::_sendData, args, blocking);
}


void SerialWorker::getState(argsState_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_getState, a, blocking);
}

void SerialWorker::getChannel(argsChannel_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_getChannel, a, blocking);
}

void SerialWorker::getCurrentRange(argsCurrentRange_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_getCurrentRange, a, blocking);
}

void SerialWorker::getVoltageSetting(argsVset_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_getVset, a, blocking);
}

void SerialWorker::getCurrentSetting(argsCset_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_getCset, a, blocking);
}



void SerialWorker::setState(argsState_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_setState, a, blocking);
}

void SerialWorker::setCurrentRange(argsCurrentRange_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_setCurrentRange, a, blocking);
}

void SerialWorker::setVoltageSetting(argsVset_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_setVset, a, blocking);
}

void SerialWorker::setCurrentSetting(argsCset_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_setCset, a, blocking);
}


// Helper function
// Executed from other (not worker's) thread. Reentrant, thread-safe
void SerialWorker::_invokeTaskQueued(TaskPointer fptr, void *arguments, bool blocking)
{
    QSemaphore *doneSem = (blocking) ? new QSemaphore() : 0;
    TaskQueueRecord_t record = {fptr, doneSem, arguments};
    taskQueueMutex.lock();
    taskQueue.append(record);
    taskQueueMutex.unlock();
    emit signal_ProcessTaskQueue();
    if (blocking)
    {
        doneSem->acquire();
        delete doneSem;
    }
}


//===========================================================================//
//===========================================================================//
//===========================================================================//
//===========================================================================//


void SerialWorker::_openPort(QSemaphore *doneSem, openPortArgs_t *a)
{
    int errCode = noError;
    QString errorText;
    QMutexLocker locker(&portSettingsMutex);
    if (serialPort->isOpen() == false)
    {
        serialPort->setPortName(portSettings.name);
        if (serialPort->open(QIODevice::ReadWrite))
        {
            if (serialPort->setBaudRate(portSettings.baudRate)
                && serialPort->setDataBits(portSettings.dataBits)
                && serialPort->setParity(portSettings.parity)
                && serialPort->setStopBits(portSettings.stopBits)
                && serialPort->setFlowControl(portSettings.flowControl))
            {
                serialPort->clear();
                emit log(LogInfo, "Port open OK");
            }
            else
            {
                // Port configure error
                errorText = "Port configure error: ";
                errorText.append(serialPort->errorString());
                _savePortError();
                errCode = errPortConfigure;
                serialPort->close();
            }
        }
        else
        {
            // Port open error
            errorText = "Port open error: ";
            errorText.append(serialPort->errorString());
            _savePortError();
            errCode = errPortCannotOpen;
            serialPort->close();
        }
    }
    else
    {
        errorText = "Port is already open";
        errCode = errPortAlreadyOpen;
    }
    if (errCode != noError)
    {
        emit log(LogErr, errorText);
    }

    a->errCode = errCode;
    doneSem->release();
}

void SerialWorker::_closePort(QSemaphore *doneSem)
{
    serialPort->close();
    _stopReceive();
    emit signal_Terminate();
    emit log(LogInfo, "Port closed");
    doneSem->release();
}

void SerialWorker::_savePortError(void)
{
    QMutexLocker locker(&portErrorDataMutex);
    portErrorCode = serialPort->error();
    portErrorString = serialPort->errorString();
}

void SerialWorker::_setVerboseLevel(int level)
{
    verboseLevel = level;
}

void SerialWorker::_processTaskQueue(void)
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
            (this->*taskRecord.fptr)(taskRecord.doneSem, taskRecord.arg);
            // Here task is done
            processingTask = false;
            // Call self again until there are no tasks in the queue
            QMetaObject::invokeMethod(this, "_processTaskQueue", Qt::QueuedConnection);
        }
    }
}

void SerialWorker::_sendData(QSemaphore *doneSem, void *arguments)
{
    sendDataArgs_t *a = (sendDataArgs_t *)arguments;
    _writeSerialPort(a->data, a->len);
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
    delete a->data;
    delete a;
}

// Helper function
int SerialWorker::_sendCmdWithAcknowledge(const QByteArray &ba)
{
    QEventLoop loop;
    QTimer ackTimer;
    // Write data to port
    int errCode = _writeSerialPort(ba.data(), ba.size());
    if (errCode == noError)
    {
        connect(&ackTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        connect(portWriteTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        connect(this, SIGNAL(signal_Terminate()), &loop, SLOT(quit()));
        connect(this, SIGNAL(signal_ackReceived()), &loop, SLOT(quit()));
        ackTimer.setSingleShot(true);
        ackRequired = true;
        // Wait for acknowledge from device.
        ackTimer.start(ackWaitTimeout);
        //log(LogThread, LogViewer::prefixThreadId("From sendCmdWithAcknowledge"));
        loop.exec();
        // Check whether ACK is received or timeout is reached
        if (ackRequired == false)
        {
            if (verboseLevel > 0)
            {
                QString logText = "Acknowledge received";
                emit log(LogInfo, logText);
            }
        }
        else
        {
            // not expecting ACK any more
            ackRequired = false;
            if (ackTimer.isActive() == false)
            {
                errCode = errNoAck;
                emit log(LogErr, "No acknowledge from device");
            }
            else if (portWriteTimer->isActive() == false)   // FIXME
            {
                errCode = errTimeout;
                //emit log(LogErr, "Port write timeout");
            }
            else
            {
                errCode = errTerminated;
                emit log(LogErr, "Acknowledge waiting terminated");
            }
        }
    }
    return errCode;
}


void SerialWorker::_getState(QSemaphore *doneSem, void *arguments)
{
    argsState_t *a = (argsState_t *)arguments;
    // Create command string
    QByteArray ba = SerialParser::cmd_readState();
    // Send and wait for acknowledge
    a->errCode = _sendCmdWithAcknowledge(ba);
    // Analyze received acknowledge string
    if (a->errCode == noError)
    {
        QStringList list = SerialParser::splitPacket(receiveBuffer);    // FIXME
        if (SerialParser::findKey(list, proto.parameters.state) == 0)
        {
            if (SerialParser::getState(list, &a->result) == 0) {
                a->errCode = noError;
            }
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "State read OK");
            else
                emit log(LogInfo, "State read ERROR");
        }
        _continueProcessingReceivedData();
    }
    // Confirm if slot call is blocking or generate a signal
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}

void SerialWorker::_getChannel(QSemaphore *doneSem, void *arguments)
{
    argsChannel_t *a = (argsChannel_t *)arguments;

    // Create command string
    QByteArray ba = SerialParser::cmd_readChannel();
    // Send and wait for acknowledge
    a->errCode = _sendCmdWithAcknowledge(ba);
    // Analyze received acknowledge string
    if (a->errCode == noError)
    {
        if (SerialParser::findKey(receiveBuffer, proto.flags.ch5v) == 0)
        {
            a->result = CHANNEL_5V;
            a->errCode = noError;
        }
        else if (SerialParser::findKey(receiveBuffer, proto.flags.ch12v) == 0)
        {
            a->result = CHANNEL_12V;
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "Channel read OK");
            else
                emit log(LogInfo, "Channel read ERROR");
        }
        _continueProcessingReceivedData();
    }
    // Confirm if slot call is blocking or generate a signal
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}

void SerialWorker::_getCurrentRange(QSemaphore *doneSem, void *arguments)
{
    argsCurrentRange_t *a = (argsCurrentRange_t *)arguments;
    QByteArray ba = SerialParser::cmd_readCurrentRange(a->channel);
    a->errCode = _sendCmdWithAcknowledge(ba);
    if (a->errCode == noError)
    {
        if (SerialParser::findKey(receiveBuffer, proto.flags.crangeLow) == 0)
        {
            a->result = CURRENT_RANGE_LOW;
            a->errCode = noError;
        }
        else if (SerialParser::findKey(receiveBuffer, proto.flags.crangeHigh) == 0)
        {
            a->result = CURRENT_RANGE_HIGH;
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "Current range read OK");
            else
                emit log(LogInfo, "Current range read ERROR");
        }
        _continueProcessingReceivedData();
    }
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}

void SerialWorker::_getVset(QSemaphore *doneSem, void *arguments)
{
    argsVset_t *a = (argsVset_t *)arguments;
    QByteArray ba = SerialParser::cmd_readVset(a->channel);
    a->errCode = _sendCmdWithAcknowledge(ba);
    if (a->errCode == noError)
    {
        if (SerialParser::getValueForKey(receiveBuffer, proto.keys.vset, &a->result) == 0)
        {
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "Voltage read OK");
            else
                emit log(LogInfo, "Voltage read ERROR");
        }
        _continueProcessingReceivedData();
    }
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}

void SerialWorker::_getCset(QSemaphore *doneSem, void *arguments)
{
    argsCset_t *a = (argsCset_t *)arguments;
    QByteArray ba = SerialParser::cmd_readCset(a->channel, a->currentRange);
    a->errCode = _sendCmdWithAcknowledge(ba);
    if (a->errCode == noError)
    {
        if (SerialParser::getValueForKey(receiveBuffer, proto.keys.cset, &a->result) == 0)
        {
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "Current read OK");
            else
                emit log(LogInfo, "Current read ERROR");
        }
        _continueProcessingReceivedData();
    }
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}


void SerialWorker::_setState(QSemaphore *doneSem, void *arguments)
{
    argsState_t *a = (argsState_t *)arguments;
    QByteArray ba = SerialParser::cmd_writeState(a->newValue);
    a->errCode = _sendCmdWithAcknowledge(ba);
    if (a->errCode == noError)
    {
        QStringList list = SerialParser::splitPacket(receiveBuffer);    // FIXME
        if (SerialParser::findKey(list, proto.parameters.state) == 0)
        {
            if (SerialParser::getState(list, &a->result) == 0)
                a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "State write OK");
            else
                emit log(LogInfo, "State write ERROR");
        }
        _continueProcessingReceivedData();
    }
    // Confirm if slot call is blocking or generate a signal
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}

void SerialWorker::_setCurrentRange(QSemaphore *doneSem, void *arguments)
{
    argsCurrentRange_t *a = (argsCurrentRange_t *)arguments;
    QByteArray ba = SerialParser::cmd_writeCurrentRange(a->channel, a->newValue);
    a->errCode = _sendCmdWithAcknowledge(ba);
    if (a->errCode == noError)
    {
        if (SerialParser::findKey(receiveBuffer, proto.flags.crangeLow) == 0)
        {
            a->result = CURRENT_RANGE_LOW;
            a->errCode = noError;
        }
        else if (SerialParser::findKey(receiveBuffer, proto.flags.crangeHigh) == 0)
        {
            a->result = CURRENT_RANGE_HIGH;
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "Current range write OK");
            else
                emit log(LogInfo, "Current range write ERROR");
        }
        _continueProcessingReceivedData();
    }
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}

void SerialWorker::_setVset(QSemaphore *doneSem, void *arguments)
{
    argsVset_t *a = (argsVset_t *)arguments;
    QByteArray ba = SerialParser::cmd_writeVset(a->channel, a->newValue);
    a->errCode = _sendCmdWithAcknowledge(ba);
    if (a->errCode == noError)
    {
        if (SerialParser::getValueForKey(receiveBuffer, proto.keys.vset, &a->result) == 0)
        {
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "Voltage write OK");
            else
                emit log(LogInfo, "Voltage write ERROR");
        }
        _continueProcessingReceivedData();
    }
    // Confirm if slot call is blocking or generate a signal
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}

void SerialWorker::_setCset(QSemaphore *doneSem, void *arguments)
{
    argsCset_t *a = (argsCset_t *)arguments;
    QByteArray ba = SerialParser::cmd_writeCset(a->channel, a->currentRange, a->newValue);
    a->errCode = _sendCmdWithAcknowledge(ba);
    if (a->errCode == noError)
    {
        if (SerialParser::getValueForKey(receiveBuffer, proto.keys.cset, &a->result) == 0)
        {
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        if (verboseLevel > 0) {
            if (a->errCode == noError)
                emit log(LogInfo, "Current write OK");
            else
                emit log(LogInfo, "Current write ERROR");
        }
        _continueProcessingReceivedData();
    }
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}









void SerialWorker::_infoPacketHandler()
{
    if (verboseLevel > 0)
        log(LogInfo, "Processing INFO packet");
    QStringList list = SerialParser::splitPacket(receiveBuffer);
    int value;
    int channel;
    int currentRange;
    if (SerialParser::getValueForKey(list, proto.keys.vmea, &value) == 0)
    {
        if (verboseLevel > 0)
            log(LogInfo, "Received new vmea");
        emit updVmea(value);
    }
    if (SerialParser::getValueForKey(list, proto.keys.cmea, &value) == 0)
    {
        if (verboseLevel > 0)
            log(LogInfo, "Received new cmea");
        emit updCmea(value);
    }
    if (SerialParser::getValueForKey(list, proto.keys.pmea, &value) == 0)
    {
        if (verboseLevel > 0)
            log(LogInfo, "Received new pmea");
        emit updPmea(value);
    }

    if (SerialParser::findKey(list, proto.parameters.state) == 0)
    {
        if (SerialParser::getState(list, &value) == 0)
            emit updState(value);
    }
    else if (SerialParser::findKey(list, proto.parameters.channel) == 0)
    {
        if (SerialParser::getChannel(list, &value) == 0)
            emit updChannel(value);
    }
    else if (SerialParser::findKey(list, proto.parameters.crange) == 0)
    {
        if ((SerialParser::getChannel(list, &channel) == 0) &&
            (SerialParser::getCurrentRange(list, &value) == 0))
        {
            emit updCurrentRange(channel, value);
        }
    }
    else if (SerialParser::findKey(list, proto.parameters.vset) == 0)
    {
        if ((SerialParser::getChannel(list, &channel) == SerialParser::NO_ERROR) &&
            (SerialParser::getValueForKey(list, proto.keys.vset, &value) == 0))
        {
            emit updVset(channel, value);
        }
    }
    else if (SerialParser::findKey(list, proto.parameters.cset) == 0)
    {
        if ((SerialParser::getChannel(list, &channel) == SerialParser::NO_ERROR) &&
            (SerialParser::getCurrentRange(list, &currentRange) == SerialParser::NO_ERROR) &&
            (SerialParser::getValueForKey(list, proto.keys.cset, &value) == 0))
        {
            emit updCset(channel, currentRange, value);
        }
    }

    _continueProcessingReceivedData();
}





void SerialWorker::_readSerialPort(void)
{
    bool exit = false;
    char c;

    while(!exit)
    {
        switch(rx.state)
        {
            case RSTATE_READING_NEW_DATA:
                rx.len = serialPort->bytesAvailable();
                if (rx.len > 0)
                {
                    //log(LogThread, LogViewer::prefixThreadId("From read serial"));
                    rx.data = new char[rx.len];
                    serialPort->read(rx.data, rx.len);
                    emit logRx(rx.data, rx.len);
                    emit bytesReceived(rx.len);
                    rx.state = RSTATE_PROCESSING_DATA;
                }
                else
                {
                    exit = true;    // no data - nothing to do
                }
                break;
            case RSTATE_PROCESSING_DATA:
                if (rx.index < rx.len)
                {
                    c = rx.data[rx.index++];
                    if (c == proto.termSymbol)
                    {
                        if (verboseLevel > 0)
                            log(LogInfo, "Received complete message");
                        // process
                        int messageType = SerialParser::getMessageType(receiveBuffer);
                        switch (messageType)
                        {
                            case SerialParser::MSG_INFO:
                                if (verboseLevel > 0)
                                    log(LogInfo, "Received INFO packet");
                                emit signal_infoReceived();
                                exit = true;
                                break;
                            case SerialParser::MSG_ACK:
                                if (ackRequired)
                                {
                                    if (verboseLevel > 0)
                                        log(LogInfo, "Received ACK");
                                    ackRequired = false;
                                    emit signal_ackReceived();
                                    exit = true;
                                }
                                else
                                {
                                    if (verboseLevel > 0)
                                        log(LogWarn, "Received ACK that is not required");
                                    receiveBuffer.clear();
                                }
                                break;
                            default:
                                if (verboseLevel > 0)
                                    log(LogInfo, "Received unknown data");
                                receiveBuffer.clear();
                                break;
                        }
                        if (exit)
                        {
                            // A signal to process receive buffer was emitted.
                            // Disconnect from serial port - ensure correct message processing order.
                            // When processed, call _continueProcessingReceivedData()
                            if (rx.receiveSlotConnected)
                            {
                                disconnect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()));
                                rx.receiveSlotConnected = false;
                            }
                        }
                    }
                    else
                    {
                        receiveBuffer.append(c);
                        if (receiveBuffer.size() >= receiveBufferLength)
                        {
                            // Reached maximum message length
                            log(LogWarn, "Reached maximum message length. Buffer will be reset");
                            receiveBuffer.clear();
                        }
                    }
                }
                else
                {
                    // Processed all data
                    rx.state = RSTATE_READING_NEW_DATA;
                    rx.index = 0;
                    delete rx.data;
                    if (rx.receiveSlotConnected == false)
                    {
                        connect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()));
                        rx.receiveSlotConnected = true;
                    }
                    // readyRead() signals may be missed as slot was disconnected -
                    // return to loop and check if new bytes are avaliable
                }
                break;
        }
    }
}


void SerialWorker::_continueProcessingReceivedData()
{
    // Clear buffer and return to further processing
    receiveBuffer.clear();
    // Allow further incoming data processing
    emit signal_ForceReadSerialPort();
}

void SerialWorker::_stopReceive()
{
    // Port must be closed and rx buffer should be flushed
    if (rx.state == RSTATE_PROCESSING_DATA)
    {
        receiveBuffer.clear();
        rx.state = RSTATE_READING_NEW_DATA;
        rx.index = 0;
        delete rx.data;
        if (rx.receiveSlotConnected == false)
        {
            connect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()));
            rx.receiveSlotConnected = true;
        }
    }
}



int SerialWorker::_writeSerialPort(const char* data, int len)
{
    // According to http://qt-project.org/doc/qt-5/qiodevice.html#writeData ,
    // all data is written before return from writeData() function
    int errCode = noError;
    int bytesWritten = serialPort->write(data, len);
    if (bytesWritten == -1)
    {
        QString errorText = "Cannot write to port: ";
        errorText.append(serialPort->errorString());
        _savePortError();
        emit log(LogErr, errorText);
        errCode = errCritical;
    }
    else if (bytesWritten != len)
    {
        // Amount of written bytes should be equal to requested.
        // If program gets here, there is a problem in QSerial.
        QString errorText = "Cannot write to port all data: ";
        errorText.append(serialPort->errorString());
        _savePortError();
        emit log(LogErr, errorText);
        errCode = errCritical;
    }
    else
    {
        // All data is written. Log data
        portWriteTimer->start(portWriteTimeout);
        emit logTx(data, bytesWritten);
    }
    return errCode;
}

//int SerialWorker::_writeSerialPort(const char* data, int len)
//{
//    int bytesWritten;
//    QTimer timer;
//    QEventLoop loop;
//    connect(serialPort, SIGNAL(bytesWritten(qint64)), &loop, SLOT(quit()));
//    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
//    connect(this, SIGNAL(signal_Terminate()), &loop, SLOT(quit()));
//    timer.setSingleShot(true);

//    int errCode = noError;
//    while (len)
//    {
//        bytesWritten = serialPort->write(data, len);
//        if (bytesWritten == -1)
//        {
//            QString errorText = "Cannot write to port: ";
//            errorText.append(serialPort->errorString());
//            _savePortError();
//            emit log(LogErr, errorText);
//            errCode = errCritical;
//            break;
//        }
//        else
//        {
//            // Some data is written. Log data
//            emit logTx(data, bytesWritten);
//            data += bytesWritten;
//            len -= bytesWritten;
//            // Wait until data is actually written through serial port and
//            // keep processing events
//            timer.start(portWriteTimeout);
//            loop.exec();
//            // Check if timeout is reached
//            if (!timer.isActive())
//            {
//                emit log(LogErr, "Port write timeout");
//                errCode = errTimeout;
//                break;
//            }
//            else if (serialPort->isOpen() == false)
//            {
//                emit log(LogErr, "Port write aborted");
//                errCode = errTerminated;
//                break;
//            }
//        }
//    }
//    return errCode;
//}


void SerialWorker::_transmitDone(qint64 bytesWritten)
{
    emit bytesTransmitted(bytesWritten);
    if (verboseLevel > 1)
    {
        // Debug only
        QString text = "Tx done, total ";
        text.append(QString::number(bytesWritten));
        text.append(" bytes");
        emit log(LogInfo, text);
    }
}

void SerialWorker::_portWriteTimeout()
{
    emit log(LogErr, "Port write timeout");
}





