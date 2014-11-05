#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QtSerialPort/QSerialPort>
#include <QMutex>
#include <QSemaphore>
#include <QQueue>
#include <QThread>
#include <QTimer>
#include "serialparser.h"


class SerialWorker : public QObject
{
    Q_OBJECT

// Data types
public:
    enum ErrorCodes {noError, errCritical = -1, errTimeout = -2,
                     errPortConfigure = 10,
                     errPortAlreadyOpen,
                     errPortCannotOpen,
                     errIncompletePortWrite,
                     errTerminated = 64,
                     errNoAck = 128,
                     errParser };
    enum LogMessageTypes {LogErr, LogWarn, LogInfo, LogThread};

    typedef struct {
        int channel;
        int result;
        int errCode;
    } getVoltageSettingArgs_t;

    typedef struct {
        int channel;
        int newValue;
        int resultValue;
        int errCode;
    } setVoltageSettingArgs_t;

    //typedef struct {
    //    int errCode;
    //} operationResult_t;

private:
    enum RxProcessorStates {RSTATE_READING_NEW_DATA = 0, RSTATE_PROCESSING_DATA};

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

    typedef void (SerialWorker::*TaskPointer)(QSemaphore *doneSem, void *arguments);
    typedef struct {
        TaskPointer fptr;
        QSemaphore *doneSem;
        void *arg;
    } TaskQueueRecord_t;

    typedef struct {
        char *data;
        int len;
    } sendStringArgs_t;



    typedef struct {
        int errCode;
    } openPortArgs_t;


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
    void sendString(const QString &text, bool blocking = false);
    void getVoltageSetting(getVoltageSettingArgs_t *a, bool blocking = false);
    void setVoltageSetting(setVoltageSettingArgs_t *a, bool blocking = false);

public slots:

    void init(void);

signals:
    //-------- Public signals -------//
    void operationDone(void);
    void updVmea(int value);
    void updCmea(int value);
    void updPmea(int value);
    void log(int type, QString message);
    void logTx(const char *message, int len);
    void logRx(const char *message, int len);
    void bytesReceived(int);
    void bytesTransmitted(int);

    //------- Private signals -------//
    // Intended for internal use only
    void signal_openPort(QSemaphore *doneSem, openPortArgs_t *a);
    void signal_closePort(QSemaphore *doneSem);
    void signal_Terminate(void);
    void signal_ProcessTaskQueue(void);
    void signal_ForceReadSerialPort(void);
    void signal_ackReceived(void);
    void signal_infoReceived(void);
private slots:
    void _openPort(QSemaphore *doneSem, openPortArgs_t *a);
    void _closePort(QSemaphore *doneSem);
    void _processTaskQueue(void);
    void _sendString(QSemaphore *doneSem, void *args);
    void _getVoltageSetting(QSemaphore *doneSem, void *arguments);
    void _setVoltageSetting(QSemaphore *doneSem, void *arguments);

    void _infoPacketHandler(void);

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


    QQueue<TaskQueueRecord_t> taskQueue;
    QMutex taskQueueMutex;
    bool processingTask;

    bool ackRequired;
    QByteArray ackData;

    int verboseLevel;

    bool connected;

    QByteArray receiveBuffer;

    void _invokeTaskQueued(TaskPointer fptr, void *arguments, bool blocking);

    void _savePortError(void);
    int _writeSerialPort(const char* data, int len);
    int _sendDataWithAcknowledge(const QByteArray &ba);

    void _processReceivedData();
    void _continueProcessingReceivedData();
    void _stopReceive();

    struct {
        char *data;
        int len;
        int index;
        int state;
        bool receiveSlotConnected;

    } rx;


};




#endif // SERIALWORKER_H
