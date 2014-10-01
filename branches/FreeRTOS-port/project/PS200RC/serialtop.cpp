#include "serialtop.h"
#include "settingshelper.h"

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

