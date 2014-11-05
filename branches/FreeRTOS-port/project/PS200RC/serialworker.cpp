#include <QEventLoop>
#include <QSignalMapper>
#include <QCoreApplication>

#include "serialworker.h"
#include "serialparser.h"
#include "logviewer.h"

SerialWorker::SerialWorker()
{
}

void SerialWorker::init()
{
    serialPort = new QSerialPort;   // no "this" parent - thread object itself lives in master thread

    connect(this, SIGNAL(signal_openPort(QSemaphore*,openPortArgs_t*)), this, SLOT(_openPort(QSemaphore*,openPortArgs_t*)));
    connect(this, SIGNAL(signal_closePort(QSemaphore*)), this, SLOT(_closePort(QSemaphore*)));
    connect(this, SIGNAL(signal_ProcessTaskQueue()), this, SLOT(_processTaskQueue()));
    connect(this, SIGNAL(signal_ForceReadSerialPort()), this, SLOT(_readSerialPort()), Qt::QueuedConnection);
    connect(this, SIGNAL(signal_infoReceived()), this, SLOT(_infoPacketHandler()), Qt::QueuedConnection);
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()));
    connect(serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(_transmitDone(qint64)));

    verboseLevel = 1;

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

// Blocking settings write
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



//-------------------------------------------------------//
/// @brief Sends a string to the device
/// @param[in] text string
/// @note If call is not blocking, signal operationDone() is
/// emitted when done.
/// @return none
//-------------------------------------------------------//
void SerialWorker::sendString(const QString &text, bool blocking)
{
    sendStringArgs_t *args = (sendStringArgs_t *)malloc(sizeof(sendStringArgs_t));
    args->len = text.length();
    args->data = (char *)malloc(args->len);
    memcpy(args->data, text.toLocal8Bit().data(), args->len);
    _invokeTaskQueued(&SerialWorker::_sendString, args, blocking);
}


//-------------------------------------------------------//
/// @brief Reads voltage setting for a channel
/// @param[in]
/// @note
/// @return
//-------------------------------------------------------//
void SerialWorker::getVoltageSetting(getVoltageSettingArgs_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_getVoltageSetting, a, blocking);
}


//-------------------------------------------------------//
/// @brief Sets voltage setting for a channel
/// @param[in]
/// @note
/// @return
//-------------------------------------------------------//
void SerialWorker::setVoltageSetting(setVoltageSettingArgs_t *a, bool blocking)
{
    _invokeTaskQueued(&SerialWorker::_setVoltageSetting, a, blocking);
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
    serialPort->clear();
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



void SerialWorker::_sendString(QSemaphore *doneSem, void *args)
{
    sendStringArgs_t *a = (sendStringArgs_t *)args;
    _writeSerialPort(a->data, a->len);
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
    delete a->data;
    delete a;
}



void SerialWorker::_getVoltageSetting(QSemaphore *doneSem, void *arguments)
{
    getVoltageSettingArgs_t *a = (getVoltageSettingArgs_t *)arguments;

    // Create command string
    QByteArray ba = parser.cmd_readVset(0);
    // Send and wait for acknowledge
    a->errCode = _sendDataWithAcknowledge(ba);
    // Analyze received acknowledge string
    if (a->errCode == noError)
    {
        if (parser.getAckData_Vset(receiveBuffer, &a->result) == 0)
        {
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        emit log(LogInfo, "Voltage read OK");
        _continueProcessingReceivedData();
    }
    // Confirm if slot call is blocking or generate a signal
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}


void SerialWorker::_setVoltageSetting(QSemaphore *doneSem, void *arguments)
{
    setVoltageSettingArgs_t *a = (setVoltageSettingArgs_t *)arguments;

    // Create command string
    QByteArray ba = parser.cmd_writeVset(a->channel, a->newValue);
    // Send and wait for acknowledge
    a->errCode = _sendDataWithAcknowledge(ba);
    // Analyze received acknowledge string
    if (a->errCode == noError)
    {
        if (parser.getAckData_Vset(receiveBuffer, &a->resultValue) == 0)
        {
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        emit log(LogInfo, "Voltage write OK");
        _continueProcessingReceivedData();
    }
    // Confirm if slot call is blocking or generate a signal
    if (doneSem)
        doneSem->release();
    else
        emit operationDone();
}





int SerialWorker::_sendDataWithAcknowledge(const QByteArray &ba)
{
    // Write data to port
    int errCode = _writeSerialPort(ba.data(), ba.size());
    if (errCode == noError)
    {
        // Wait for acknowledge from device.
        QEventLoop loop;
        QTimer ackTimer;
        connect(this, SIGNAL(signal_ackReceived()), &loop, SLOT(quit()));
        connect(&ackTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        connect(this, SIGNAL(signal_Terminate()), &loop, SLOT(quit()));
        ackTimer.setSingleShot(true);
        ackTimer.start(ackWaitTimeout);
        ackRequired = true;
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
            else
            {
                errCode = errTerminated;
                emit log(LogErr, "Acknowledge waiting terminated");
            }
        }
    }
    return errCode;
}




void SerialWorker::_infoPacketHandler()
{
    log(LogInfo, "Processing INFO packet");
    QString valueStr;
    if (parser._getValueForKey(receiveBuffer, "-vm", valueStr) == 0)
    {
        log(LogInfo, "Updating measured voltage");
        emit updVmea(valueStr.toInt());
    }
    if (parser._getValueForKey(receiveBuffer, "-cm", valueStr) == 0)
    {
        log(LogInfo, "Updating measured current");
        emit updCmea(valueStr.toInt());
    }
    if (parser._getValueForKey(receiveBuffer, "-pm", valueStr) == 0)
    {
        log(LogInfo, "Updating measured power");
        emit updPmea(valueStr.toInt());
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
                    receiveBuffer.append(c);
                    if (c == SerialParser::termSymbol)
                    {
                        if (verboseLevel > 0)
                        {
                            log(LogInfo, "Received complete message");
                        }
                        // process
                        int messageType = parser.getMessageType(receiveBuffer);
                        switch (messageType)
                        {
                            case SerialParser::MSG_INFO:
                                log(LogInfo, "Received INFO packet");
                                emit signal_infoReceived();
                                exit = true;
                                break;
                            case SerialParser::MSG_ACK:
                                if (ackRequired)
                                {
                                    log(LogInfo, "Received ACK");
                                    ackRequired = false;
                                    emit signal_ackReceived();
                                    exit = true;
                                }
                                else
                                {
                                    log(LogWarn, "Received ACK that is not required");
                                    receiveBuffer.clear();
                                }
                                break;
                            default:
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
    int bytesWritten;
    QTimer timer;

    int errCode = noError;
    while (len)
    {
        bytesWritten = serialPort->write(data, len);
        if (bytesWritten == -1)
        {
            QString errorText = "Cannot write to port: ";
            errorText.append(serialPort->errorString());
            _savePortError();
            emit log(LogErr, errorText);
            errCode = errCritical;
            break;
        }
        else
        {
            // Some data is written. Log data
            emit logTx(data, bytesWritten);
            data += bytesWritten;
            len -= bytesWritten;
            // Wait until data is actually written through serial port
            timer.setSingleShot(true);
            timer.start(portWriteTimeout);
            // Keep processing incoming RX data
            while (timer.isActive() && serialPort->bytesToWrite())  // TODO: add terminated signal
                QCoreApplication::processEvents();
            // Check if timeout is reached
            if (!timer.isActive())
            {
                // Timeout error
                emit log(LogErr, "Port write timeout");
                errCode = errTimeout;
                break;
            }
        }
    }
    return errCode;
}


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





