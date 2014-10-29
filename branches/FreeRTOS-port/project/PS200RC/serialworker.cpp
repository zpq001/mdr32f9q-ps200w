#include <QEventLoop>
#include <QSignalMapper>
#include <QCoreApplication>

#include "serialworker.h"
#include "serialparser.h"

SerialWorker::SerialWorker()
{
    serialPort = new QSerialPort;   // no "this" parent - thread object itself lives in master thread

    connect(this, SIGNAL(signal_openPort(QSemaphore*,openPortArgs_t*)), this, SLOT(_openPort(QSemaphore*,openPortArgs_t*)));
    connect(this, SIGNAL(signal_closePort(QSemaphore*)), this, SLOT(_closePort(QSemaphore*)));
    connect(this, SIGNAL(signal_sendString(QSemaphore *,sendStringArgs_t*)),
            this, SLOT(_sendString(QSemaphore *,sendStringArgs_t*)));
    connect(this, SIGNAL(signal_getVoltageSetting(QSemaphore *,getVoltageSettingArgs_t*)),
            this, SLOT(_getVoltageSetting(QSemaphore *,getVoltageSettingArgs_t*)));

    connect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()));
    connect(this, SIGNAL(signal_ForceReadSerialPort()), this, SLOT(_readSerialPort()));
    connect(serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(_transmitDone(qint64)));

    verboseLevel = 1;
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
/// @note Non-blocking. Returns when text is put to serial buffer
/// (not completely transmitted).
/// @return
//-------------------------------------------------------//
int SerialWorker::sendString(const QString &text)
{
    workerMutex.lock();     // Protect from re-entrance
    QSemaphore sem;
    sendStringArgs_t a;
    a.str = text;
    emit signal_sendString(&sem, &a);
    sem.acquire();
    return a.errCode;
}


//-------------------------------------------------------//
/// @brief Reads voltage setting for a channel
/// @param[in]
/// @note
/// @return
//-------------------------------------------------------//
void SerialWorker::getVoltageSetting(getVoltageSettingArgs_t *a, bool wait = false)
{
    workerMutex.lock();     // Protect from re-entrance
    QSemaphore *sem = (wait) ? new QSemaphore() : 0;
    emit signal_getVoltageSetting(sem, a);
    if (wait)
    {
        sem->acquire();
        delete sem;
    }
}




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
    emit log(LogInfo, "Port closed");
    doneSem->release();
}

void SerialWorker::_savePortError(void)
{
    QMutexLocker locker(&portErrorDataMutex);
    portErrorCode = serialPort->error();
    portErrorString = serialPort->errorString();
}





void SerialWorker::_sendString(QSemaphore *doneSem, sendStringArgs_t *a)
{
    a->errCode = _writeSerialPort(a->str.toLocal8Bit(), a->str.length());
    if (doneSem)
    {
        doneSem->release();
    }
    workerMutex.unlock();
}



void SerialWorker::_getVoltageSetting(QSemaphore *doneSem, getVoltageSettingArgs_t *a)
{
    // Create command string
    QByteArray ba = parser.cmd_readVset(0);
    // Send and wait for acknowledge
    a->errCode = _sendDataWithAcknowledge(ba);
    // Analyze received acknowledge string
    if (a->errCode == noError)
    {
        // Parse acknowledge string
        // Fill return values
        if (parser.getAckData_Vset(receiveBuffer, &a->result) == 0)
        {
            a->errCode = noError;
        }
        else
        {
            a->errCode = errParser;
        }
        //a->result = 3560;   //mV
        //a->errCode = noError;
        emit log(LogInfo, "Voltage read OK");
    }
    // Confirm if slot call is blocking or generate a signal
    if (doneSem)
    {
        doneSem->release();
        // done!
    }
    else
    {
        // emit cmdReady() signal
    }

    // Allow further incoming data processing
    emit signal_ForceReadSerialPort();
    workerMutex.unlock();
}





int SerialWorker::_sendDataWithAcknowledge(const QByteArray &ba)
{
    // Write data to port
    int errCode = _writeSerialPort(ba.data(), ba.size());
    if (errCode == noError)
    {
        // Wait for acknowledge from device.
        QEventLoop loop;
        connect(this, SIGNAL(signal_ackReceived()), &loop, SLOT(quit()));
        connect(&ackTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        ackTimer.setSingleShot(true);
        ackTimer.start(ackWaitTimeout);
        ackRequired = true;
        loop.exec();
        // Check whether ACK is received or timeout is reached
        if (ackTimer.isActive())
        {
            ackTimer.stop();
            if (verboseLevel > 0)
            {
                QString logText = "Acknowledge received";
                emit log(LogInfo, logText);
            }
        }
        else
        {
            errCode = errNoAck;
            emit log(LogErr, "No acknowledge from device");
        }
        // not expecting ACK any more
        ackRequired = false;
    }
    return errCode;
}





void SerialWorker::_readSerialPort(void)
{
    static int state = RSTATE_READING_NEW_DATA;
    static char *data;
    static int len;
    static int index;
    char c;
    bool exit = false;

    while(!exit)
    {
        switch(state)
        {
            case RSTATE_READING_NEW_DATA:
                len = serialPort->bytesAvailable();
                if (len > 0)
                {
                    data = new char[len];
                    serialPort->read(data, len);
                    emit logRx(data, len);
                    state = RSTATE_PROCESSING_DATA;
                }
                else
                {
                    exit = true;    // no data - nothing to do
                }
                break;
            case RSTATE_PROCESSING_DATA:
                if (index < len)
                {
                    c = data[index++];
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
                                // Call parser
                                receiveBuffer.clear();
                                break;
                            case SerialParser::MSG_ACK:
                                if (ackRequired)
                                {
                                    log(LogInfo, "Received ACK");
                                    // Emit signal that exits local loop and enables further processing in requesting function
                                    emit signal_ackReceived();
                                    // Disconnect from serial port - ensure correct message processing order
                                    disconnect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()));
                                    // A signal that is connected to this slot must be emitted after ACK processing
                                    state = RSTATE_PROCESSING_ACK;
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
                    state = RSTATE_READING_NEW_DATA;
                    index = 0;
                    delete data;
                }
                break;
            case RSTATE_PROCESSING_ACK:
                // Connect to serial port again
                connect(serialPort, SIGNAL(readyRead()), this, SLOT(_readSerialPort()));
                // Clear buffer and return to further processing
                receiveBuffer.clear();
                state = RSTATE_PROCESSING_DATA;
                break;
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
            while (timer.isActive() && serialPort->bytesToWrite())
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
    if (verboseLevel > 0)
    {
        // Debug only
        QString text = "Tx done, total ";
        text.append(QString::number(bytesWritten));
        text.append(" bytes");
        emit log(LogInfo, text);
    }
}





