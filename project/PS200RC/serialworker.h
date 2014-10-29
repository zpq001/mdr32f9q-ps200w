#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QtSerialPort/QSerialPort>
#include <QMutex>
#include <QSemaphore>
#include <QQueue>
#include <QThread>
#include <QTimer>
#include "serialparser.h"


class SerialWorker : public QThread
{
    Q_OBJECT

// Data types
public:
    enum ErrorCodes {noError, errCritical = -1, errTimeout = -2,
                     errPortConfigure = 10,
                     errPortAlreadyOpen,
                     errPortCannotOpen,
                     errIncompletePortWrite,
                     errNoAck = 128,
                     errParser };
    enum LogMessageTypes {LogErr, LogWarn, LogInfo};

    typedef struct {
        int channel;
        int result;
        int errCode;
    } getVoltageSettingArgs_t;

    //typedef struct {
    //    int errCode;
    //} operationResult_t;

private:
    enum RxProcessorStates {RSTATE_READING_NEW_DATA = 0, RSTATE_PROCESSING_DATA, RSTATE_PROCESSING_ACK};

    typedef struct {
        QString name;
        int baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBits;
        QSerialPort::FlowControl flowControl;
    }  portSettings_t;

//    typedef struct {
//        QSemaphore sem;
//        int errCode;
//        bool isBlocking;
//    } reqArg_t;

    typedef struct {
        int errCode;
    } openPortArgs_t;

    typedef struct {
        QString str;
        int errCode;
    } sendStringArgs_t;

    static const int portWriteTimeout = 1000;   //ms
    static const int ackWaitTimeout = 1000;     //ms
    static const int receiveBufferLength = 100; // chars

// Methods
public:
    SerialWorker();
    QMutex workerMutex;

    // Serial port functions
    int openPort(void);
    void closePort(void);
    void writePortSettings(const QString &name, int baudRate, int dataBits, int parity, int stopBits, int flowControl);
    int getPortErrorCode(void);
    QString getPortErrorString(void);


    //
    int sendString(const QString &text);
    void getVoltageSetting(getVoltageSettingArgs_t *a, bool wait);

public slots:


signals:
    //-------- Public signals -------//
    void operationDone(void);


    void log(int type, QString message);
    void logTx(const char *message, int len);
    void logRx(const char *message, int len);
    //------- Private signals -------//
    // Intended for internal use only
    void signal_openPort(QSemaphore *doneSem, openPortArgs_t *a);
    void signal_closePort(QSemaphore *doneSem);
    void signal_sendString(QSemaphore *doneSem, sendStringArgs_t *a);
    void signal_getVoltageSetting(QSemaphore *doneSem, getVoltageSettingArgs_t *a);
    void signal_ackReceived(void);
    //void signal_ackTimeout(void);
    void signal_ForceReadSerialPort(void);
private slots:
    void _openPort(QSemaphore *doneSem, openPortArgs_t *a);
    void _closePort(QSemaphore *doneSem);
    void _sendString(QSemaphore *doneSem, sendStringArgs_t *a);
    void _getVoltageSetting(QSemaphore *doneSem, getVoltageSettingArgs_t *a);

    void _readSerialPort(void);
    void _transmitDone(qint64 bytesWritten);
private:
    SerialParser parser;

    QMutex portSettingsMutex;
    QSerialPort *serialPort;
    portSettings_t portSettings;

    QMutex portErrorDataMutex;
    QString portErrorString;
    int portErrorCode;

    QTimer ackTimer;

    bool ackRequired;
    QByteArray ackData;

    int verboseLevel;

    QByteArray receiveBuffer;

    void _savePortError(void);
    int _writeSerialPort(const char* data, int len);
    int _sendDataWithAcknowledge(const QByteArray &ba);

    // Override thread RUN function
    //void run();

};





/*
class SerialWorker : public QObject
{
    Q_OBJECT

public:
    enum ErrorCodes {noError, errCritical = -1, errTimeout = -2,
                     errPortConfigure = 10,
                     errPortAlreadyOpen,
                     errPortCannotOpen,
                     errNoAck = 128 };
    enum LogMessageTypes {LogErr, LogWarn, LogInfo};
    //enum KeyCodes {key_OK, key_ESC, key_RIGHT, key_LEFT, key_ENCODER};
    //enum KeyActions {act_DOWN, act_UP, act_UP_SHORT, act_UP_LONG, act_HOLD};
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

    int sendString(const QString &text);

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
    void _log(int type, QString message);
    void _logTx(const char *message, int len);
    void _logRx(const char *message, int len);
private slots:
    void on_SerialDataReceive(void);
private:
    int writeSerialPort(const char* data, int len);
    int readSerialPort(char *data);

    QSerialPort *serialPort;
    portSettings_t portSettings;
    struct {
        bool connected;
    } state;


};
*/
#endif // SERIALWORKER_H
