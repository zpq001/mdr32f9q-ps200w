#include "settingshelper.h"
#include <QSettings>


QSettings appSettings("settings.ini", QSettings::IniFormat);
QMutex settingsMutex;

// Check that there is a setting with specified key.
// If setting key is preset, returns 0
// If setting key is absent, returns 1 and sets defaultValue for that key.
int SettingsHelper::checkSetting(const QString key, QVariant defaultValue)
{
    int result = 0;
    if (appSettings.contains(key) == false)
    {
        appSettings.setValue(key, defaultValue);
        result = 1;
    }
    return result;
}

// Check that there is a setting with specified key and setting value is one of the listed in table
// If setting key is preset and value is found in the table, returns 0
// If setting key is absent, returns 1 and sets defaultValue for that key.
// If setting key is preset, but value is unknown, sets defaultValue for that key and returns 2.
int SettingsHelper::checkSettingValue(const QString key, QVariant defaultValue, const sqv_record_t *table)
{
    int result = 0;
    if (appSettings.contains(key) == false)
    {
        result = 1;
    }
    else if (findValue(table, appSettings.value(key).toString()) == -1)
    {
        result = 2;
    }
    if (result)
    {
         appSettings.setValue(key, defaultValue);
    }
    return result;
}


bool SettingsHelper::validateSettings(void)
{
    int result = 0;

    result |= checkSetting("serial/port", "");
    result |= checkSetting("serial/baudrate", QSerialPort::Baud115200);

    result |= checkSettingValue("serial/databits", QSerialPort::Data8, serial_databits);
    result |= checkSettingValue("serial/databits", QSerialPort::Data8, serial_databits);
    result |= checkSettingValue("serial/parity", "None", serial_parity);
    result |= checkSettingValue("serial/stopbits", "1", serial_stopbits);
    result |= checkSettingValue("serial/flowctrl", "None", serial_flowctrl);

    // More...
    return (result == 0);
}



int SettingsHelper::findValue(const sqv_record_t *table, const QString key)
{
    int index = -1;
    for(int i=0; table[i].sv!=0; i++)
    {
        if (key.compare(table[i].sv) == 0)
        {
            index = i;
            break;
        }
    }
    return index;
}



QVariant SettingsHelper::getValue(const sqv_record_t *table, const QString key)
{
    QVariant v;
    int index = findValue(table, appSettings.value(key).toString());
    if (index > 0)
    {
        v = table[index].qv;
    }
    else
    {
        v = 0;
    }
    return v;
}

/*
void SettingsHelper::getGetSerialSettings(SerialSettings_t *d)
{
    QVariant q;
    if ()
}
*/



