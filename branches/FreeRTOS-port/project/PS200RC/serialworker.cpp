#include "serialworker.h"
#include <QEventLoop>
#include <QSignalMapper>

SerialWorker::SerialWorker()
{
    serialPort = new QSerialPort;   // no "this" parent - thread object itself lives in master thread

    connect(this, SIGNAL(signal_openPort(QSemaphore*,openPortArgs_t*)), this, SLOT(_openPort(QSemaphore*,openPortArgs_t*)));
    connect(this, SIGNAL(signal_closePort(QSemaphore*)), this, SLOT(_closePort(QSemaphore*)));
    connect(this, SIGNAL(signal_sendString(reqArg_t*,sendStringArgs_t*)),
            this, SLOT(_sendString(reqArg_t*,sendStringArgs_t*)));
    connect(this, SIGNAL(signal_getVoltageSetting(reqArg_t*,getVoltageSettingArgs_t*)),
            this, SLOT(_getVoltageSetting(reqArg_t*,getVoltageSettingArgs_t*)));

    connect(serialPort, SIGNAL(readyRead()), this, SLOT(_processRx()));
    connect(serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(_transmitDone(qint64)));
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
    reqArg_t req;
    sendStringArgs_t a;
    a.str = text;
    emit signal_sendString(&req, &a);
    req.sem.acquire();
    return req.errCode;
}


//-------------------------------------------------------//
/// @brief Reads voltage setting for a channel
/// @param[in]
/// @note
/// @return
//-------------------------------------------------------//
int SerialWorker::getVoltageSetting(getVoltageSettingArgs_t *a, bool wait = false)
{
    workerMutex.lock();     // Protect from re-entrance
    reqArg_t req;
    req.isBlocking = wait;
    emit signal_getVoltageSetting(&req, a);
    req.sem.acquire();
    if (req.errCode == noError)
    {
        if (wait)
            req.sem.acquire();
    }
    return req.errCode;
}







void SerialWorker::_savePortError(void)
{
    QMutexLocker locker(&portErrorDataMutex);
    portErrorCode = serialPort->error();
    portErrorString = serialPort->errorString();
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


void SerialWorker::_sendString(reqArg_t *req, sendStringArgs_t *a)
{
    req->errCode = _writeSerialPort(a->str.toLocal8Bit(), a->str.length());
    req->sem.release();
}


void SerialWorker::_getVoltageSetting(reqArg_t *req, getVoltageSettingArgs_t *a)
{
    // Create command string
    QString cmdStr = "converter get_voltage -ch12v";
    // Write data to port
    req->errCode = _writeSerialPort(cmdStr.toLocal8Bit(), cmdStr.length());
    // First phase confirmation
    req->sem.release();
    if (req->errCode == noError)
    {
        // Wait for acknowledge from device.
        QEventLoop loop;
        connect(this, SIGNAL(signal_ackReceived()), &loop, SLOT(quit()));
        connect(&ackTimer, SIGNAL(timeout()), &loop, SLOT(quit()));
        ackTimer.start(50);
        ackRequired = true;
        loop.exec();
        // Check if ACK is received - request flag is reset by receive routine
        if (ackRequired == false)
        {
            ackTimer.stop();
            // Parse acknowledge string
            // Fill return values
            a->result = 3560;   //mV
            a->errCode = noError;
            emit log(LogInfo, "Voltage read OK");
        }
        else
        {
            a->errCode = errNoAck;
            emit log(LogErr, "No acknowledge from device");
        }


        // Second phase confirmation
        if (req->isBlocking)
        {
            req->sem.release();
        }
        else
        {
            // emit _cmd_done()_
        }
    }
    workerMutex.unlock();
}










void SerialWorker::_processRx(void)
{
    int len = serialPort->bytesAvailable();
    if (len > 0)
    {
        char *data = new char[len];
        serialPort->read(data, len);
        emit logRx(data, len);
        delete data;
    }
}



void SerialWorker::_transmitDone(qint64 bytesWritten)
{
    emit log(LogInfo, "TX done");
}


int SerialWorker::_writeSerialPort(const char* data, int len)
{
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
    else
    {
        // Some data is written. Log data
        emit logTx(data, bytesWritten);
        // Check for errors
        if (bytesWritten != len)
        {
            emit log(LogErr, "Cannot write all data to port!");
            _savePortError();
            errCode = errIncompletePortWrite;
        }
    }
    return errCode;
}


/*
 * BUGGY due to waitForBytesWritten() issue
 *
 * int SerialWorker::_writeSerialPort(const char* data, int len)
{
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
    else
    {
        // Some data is written. Log data
        emit logTx(data, bytesWritten);
        // Check for errors
        if (bytesWritten != len)
        {
            emit log(LogErr, "Cannot write all data to port!");
            _savePortError();
            errCode = errIncompletePortWrite;
        }
        if (!serialPort->waitForBytesWritten(portWriteTimeout))
        {
            errCode = errTimeout;
            _savePortError();
        }
    }
    return errCode;
}


*/
/*
void SerialWorker::on_SerialDataReceive()
{
    int len = serialPort->bytesAvailable();
    char *data = new char[len];
    serialPort->read(data, len);
    emit logRx(data, len);
    delete data;
}
*/


/*
void SerialWorker::run()
{
    exec();
}
*/

/*
SerialWorker::SerialWorker()
{
    // Initialize thread
    serialPort = new QSerialPort;   // no "this" parent - thread object itself lives in main GUI thread
    connected = false;

    connect(serialPort, SIGNAL(readyRead()), this, SLOT(on_SerialDataReceive()));
}


void SerialWorker::writePortSettings(const QString &name, int baudRate, int dataBits, int parity, int stopBits, int flowControl)
{
    portSettings.name = name;
    portSettings.baudRate = (QSerialPort::BaudRate) baudRate;
    portSettings.dataBits = (QSerialPort::DataBits) dataBits;
    portSettings.parity = (QSerialPort::Parity) parity;
    portSettings.stopBits = (QSerialPort::StopBits) stopBits;
    portSettings.flowControl = (QSerialPort::FlowControl) flowControl;
}

int SerialWorker::openPort(void)
{
    int errCode = noError;
    QString errorText;
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
                connected = true;
                serialPort->clear();
                _log(LogInfo, "Port open OK");
            }
            else
            {
                // Port configure error
                errorText = "Port configure error: ";
                errorText.append(serialPort->errorString());
                errCode = errPortConfigure;
                serialPort->close();
            }
        }
        else
        {
            // Port open error
            errorText = "Port open error: ";
            errorText.append(serialPort->errorString());
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
        _log(LogErr, errorText);
    }
    return errCode;
}

void SerialWorker::closePort(void)
{
    serialPort->close();
    connected = false;
    _log(LogInfo, "Port closed");
}

int SerialWorker::getPortErrorCode(void)
{
    return serialPort->error();
}

QString SerialWorker::getPortErrorString(void)
{
    return serialPort->errorString();
}


//-------------------------------------------------------//
/// @brief Blocking read of device channel
/// @param[inout] *result - if function comletes succesfully,
///     stores actual device channel
/// @note
/// @return
//-------------------------------------------------------//
int SerialWorker::getChannel(int *result)
{
    int errCode = 0;
    QString cmdStr;
    if (state.connected == false)
    {
        //emit _error("Not connected to device");
        return errCritical;
    }
    cmdStr = "converter get_channel\r\n";
    errCode = writeSerialPort(cmdStr.toLocal8Bit(), cmdStr.length());
    if (errCode == noError)
    {
        // Wait for ACK response
        *result = 0;
    }
    else
    {
       // emit _error( (errCode == errTimeout) ? "Timeout error" : "Cannot write to port" );
    }
    return errCode;
}


int SerialWorker::writeSerialPort(const char* data, int len)
{
    int bytesWritten = serialPort->write(data, len);
    if (bytesWritten == -1)
    {
        QString errorText = "Cannot write to port: ";
        errorText.append(serialPort->errorString());
        _log(LogErr, errorText);
        return errCritical;
    }
    // Data is written. Log data
    emit _logTx(data, bytesWritten);
    return (serialPort->waitForBytesWritten(100) == true) ? noError : errTimeout;
}

int SerialWorker::sendString(const QString &text)
{
    return writeSerialPort(text.toLocal8Bit(), text.length());
}


void SerialWorker::on_SerialDataReceive()
{
    int len = serialPort->bytesAvailable();
    char *data = new char[len];
    serialPort->read(data, len);
    emit _logRx(data, len);
}
*/



