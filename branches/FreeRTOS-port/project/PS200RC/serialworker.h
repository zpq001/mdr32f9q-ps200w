#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QtSerialPort/QSerialPort>
#include <QMutex>
#include <QSemaphore>
#include <QQueue>
#include <QThread>
#include <QTimer>



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
        int newValue;
        int result;
        int errCode;
    } argsState_t;

    typedef struct {
        int newValue;
        int result;
        int errCode;
    } argsChannel_t;

    typedef struct {
        int channel;
        int newValue;
        int result;
        int errCode;
    } argsCurrentRange_t;

    typedef struct {
        int channel;
        int newValue;
        int result;
        int errCode;
    } argsVset_t;

    typedef struct {
        int channel;
        int currentRange;
        int newValue;
        int result;
        int errCode;
    } argsCset_t;


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

    typedef void (SerialWorker::*TaskPointer)(QSemaphore *doneSem, void *arguments);
    typedef struct {
        TaskPointer fptr;
        QSemaphore *doneSem;
        void *arg;
    } TaskQueueRecord_t;

    typedef struct {
        int errCode;
    } openPortArgs_t;

    typedef struct {
        char *data;
        int len;
    } sendDataArgs_t;

    static const int portWriteTimeout = 100;   //ms
    static const int ackWaitTimeout = 500;     //ms
    static const int receiveBufferLength = 100; // chars

// Methods
public:
    SerialWorker();

    // Serial port functions
    int openPort(void);
    void closePort(void);
    void writePortSettings(const QString &name, int baudRate, int dataBits, int parity, int stopBits, int flowControl);
    int getPortErrorCode(void);
    QString getPortErrorString(void);
    void setVerboseLevel(int);

    //
    void sendString(const QString &text, bool blocking = false);

    void getState(argsState_t *a, bool blocking = false);
    void getChannel(argsChannel_t *a, bool blocking = false);
    void getCurrentRange(argsCurrentRange_t *a, bool blocking = false);
    void getVoltageSetting(argsVset_t *a, bool blocking = false);
    void getCurrentSetting(argsCset_t *a, bool blocking = false);

    void setState(argsState_t *a, bool blocking = false);
    void setCurrentRange(argsCurrentRange_t *a, bool blocking = false);
    void setVoltageSetting(argsVset_t *a, bool blocking = false);
    void setCurrentSetting(argsCset_t *a, bool blocking = false);

public slots:

    void init(void);

signals:
    //-------- Public signals -------//
    void operationDone(void);
    void log(int type, QString message);
    void logTx(const char *message, int len);
    void logRx(const char *message, int len);
    void bytesReceived(int);
    void bytesTransmitted(int);

    void updVmea(int value);
    void updCmea(int value);
    void updPmea(int value);

    void updState(int value);
    void updChannel(int value);
    void updCurrentRange(int channel, int value);
    void updVset(int channel, int value);
    void updCset(int channel, int crange, int value);


    //------- Private signals -------//
    // Intended for internal use only
    void signal_openPort(QSemaphore *doneSem, openPortArgs_t *a);
    void signal_closePort(QSemaphore *doneSem);
    void signal_setVerboseLevel(int);
    void signal_Terminate(void);
    void signal_ProcessTaskQueue(void);
    void signal_ForceReadSerialPort(void);
    void signal_ackReceived(void);
    void signal_infoReceived(void);
private slots:
    void _openPort(QSemaphore *doneSem, openPortArgs_t *a);
    void _closePort(QSemaphore *doneSem);
    void _setVerboseLevel(int);
    void _readSerialPort(void);
    void _transmitDone(qint64 bytesWritten);
    void _portWriteTimeout(void);
    void _processTaskQueue(void);
    void _infoPacketHandler(void);

private:
    void _sendData(QSemaphore *doneSem, void *arguments);

    void _getState(QSemaphore *doneSem, void *arguments);
    void _getChannel(QSemaphore *doneSem, void *arguments);
    void _getCurrentRange(QSemaphore *doneSem, void *arguments);
    void _getVset(QSemaphore *doneSem, void *arguments);
    void _getCset(QSemaphore *doneSem, void *arguments);
    void _getVsetLim(QSemaphore *doneSem, void *arguments);
    void _getCsetLim(QSemaphore *doneSem, void *arguments);
    void _getProtectionState(QSemaphore *doneSem, void *arguments);

    void _setState(QSemaphore *doneSem, void *arguments);
    void _setCurrentRange(QSemaphore *doneSem, void *arguments);
    void _setVset(QSemaphore *doneSem, void *arguments);
    void _setCset(QSemaphore *doneSem, void *arguments);
    void _setVsetLim(QSemaphore *doneSem, void *arguments);
    void _setCsetLim(QSemaphore *doneSem, void *arguments);
    void _setProtectionState(QSemaphore *doneSem, void *arguments);



    //SerialParser parser;

    QMutex portSettingsMutex;
    QSerialPort *serialPort;
    portSettings_t portSettings;
    QTimer *portWriteTimer;

    QMutex portErrorDataMutex;
    QString portErrorString;
    int portErrorCode;


    QQueue<TaskQueueRecord_t> taskQueue;
    QMutex taskQueueMutex;
    bool processingTask;


    int verboseLevel;

    bool connected;

    QByteArray receiveBuffer;
    bool ackRequired;

    void _invokeTaskQueued(TaskPointer fptr, void *arguments, bool blocking);

    void _savePortError(void);
    int _writeSerialPort(const char* data, int len);
    int _sendCmdWithAcknowledge(const QByteArray &ba);

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
