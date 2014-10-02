#include "serialworker.h"


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



