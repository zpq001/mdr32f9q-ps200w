#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QtSerialPort/QSerialPort>

class SerialWorker : public QObject
{
    Q_OBJECT

public:
    enum ErrorCodes {noError, errCritical = -1, errTimeout = -2,
                     errPortConfigure = 10,
                     errPortAlreadyOpen,
                     errPortCannotOpen,
                     errNoAck = 128 };
    enum LogMessageTypes {LogMsg, LogErr, LogWarn, LogInfo};
    typedef struct {
        QString name;
        int baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBits;
        QSerialPort::FlowControl flowControl;
    }  portSettings_t;

    bool connected;

    SerialWorker();
    int openPort(void);
    void closePort(void);
    void writePortSettings(const QString &name, int baudRate, int dataBits, int parity, int stopBits, int flowControl);
    int getPortErrorCode(void);
    QString getPortErrorString(void);

    int getChannel(int *result);
    int getCurrentRange(int channel, int *result);
    int getVoltageSetting(int channel, int *result);
    int getCurrentSetting(int channel, int range, int *result);
    int getState(int *result);

    int setState(int value);
    int setCurrentRange(int channel, int value);
    int setVoltage(int channel, int value);
    int setCurrent(int channel, int range, int value);

public slots:
signals:

    void _error(QString);
    void _log(QString message, int type);
    void _logTx(const char *message, int len);
    void _logRx(const char *message, int len);
private:
    int writeSerialPort(const char* data, int len);
    int readSerialPort(char *data);

    QSerialPort *serialPort;
    portSettings_t portSettings;
    struct {
        bool connected;
    } state;


};

#endif // SERIALWORKER_H
