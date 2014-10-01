#ifndef SETTINGSHELPER_H
#define SETTINGSHELPER_H

#include <QVariant>
#include <QSettings>
#include <QtSerialPort/QSerialPort>
#include <QMutex>

extern QSettings appSettings;
extern QMutex settingsMutex;

/*
typedef struct {
    const char *key;
} settingItem_t;

typedef struct {
    const char *key;
    const char *possibleValues[];
    const QVariant converterValues[];
} settingItemVar_t;
*/

typedef struct {
    const char *sv;
    QVariant qv;
} sqv_record_t;
/*
typedef struct {
    QString name;
    int baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;
} SerialSettings_t;
*/
const sqv_record_t serial_databits[] = {
    {"5", QSerialPort::Data5},
    {"6", QSerialPort::Data6},
    {"7", QSerialPort::Data7},
    {"8", QSerialPort::Data8},
    {0,0}
};

const sqv_record_t serial_parity[] = {
    {"None", QSerialPort::NoParity},
    {"Odd", QSerialPort::OddParity},
    {"Even", QSerialPort::EvenParity},
    {"Mark", QSerialPort::MarkParity},
    {"Mark", QSerialPort::SpaceParity},
    {0,0}
};

const sqv_record_t serial_stopbits[] = {
    {"1", QSerialPort::OneStop},
#ifdef Q_OS_WIN
    {"1.5", QSerialPort::OneAndHalfStop},
#endif
    {"2", QSerialPort::TwoStop},
    {0,0}
};

const sqv_record_t serial_flowctrl[] = {
    {"None", QSerialPort::NoFlowControl},
    {"RTS/CTS", QSerialPort::HardwareControl},
    {"XON/XOFF", QSerialPort::SoftwareControl},
    {0,0}
};

class SettingsHelper
{
public:
private:
    static int checkSetting(const QString key, QVariant defaultValue);
    static int checkSettingValue(const QString key, QVariant defaultValue, const sqv_record_t *table);
public:
    static bool validateSettings(void);
    static int findValue(const sqv_record_t *table, const QString key);
    static QVariant getValue(const sqv_record_t *table, const QString key);

};

#endif // SETTINGSHELPER_H
