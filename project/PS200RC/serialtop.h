#ifndef SERIALTOP_H
#define SERIALTOP_H

#include <QObject>
#include <QEventLoop>
#include <QQueue>
#include "serialworker.h"



class SerialTop : public QObject
{
    Q_OBJECT
public:
    explicit SerialTop(QObject *parent = 0);

private:
    typedef void (SerialTop::*TaskPointer)(void *);
    typedef struct {
        TaskPointer fptr;
        void *arg;
    } TaskQueueRecord_t;

    typedef struct {
        int channel;
        int currentRange;
    } value_cache_t;

signals:
    //-------- Public signals -------//
    void connectedChanged(bool);
    void _log(QString text, int type);
    void bytesTransmitted(int);
    void bytesReceived(int);

    void updVmea(int);
    void updCmea(int);
    void updPmea(int);
/*
    void updState(int value);
    void updChannel(int value);
    void updCurrentRange(int channel, int value);
    void updVset(int channel, int value);
    void updCset(int channel, int crange, int value);
*/
    void updState(int value);
    void updChannel(int value);
    void updCurrentRange(int value);
    void updVset(int value);
    void updCset(int value);

    //------- Private signals -------//
    void signal_Terminate();
    void signal_ProcessTaskQueue();
public slots:
    // Must be connected only by signal-slot
    void init(void);
    void connectToDevice(void);
    void disconnectFromDevice(void);
    void sendString(const QString &text);
    void keyEvent(int key, int event);
    // Can be either signal-slot connected or called directly from other thread
    void setState(int state);
    void setCurrentRange(int channel, int value);
    void setVoltage(int channel, int value);
    void setCurrent(int channel, int currentRange, int value);
private slots:
    void _processTaskQueue(void);
    void onWorkerUpdVmea(int);
    void onWorkerUpdCmea(int);
    void onWorkerUpdPmea(int);
    void onWorkerUpdState(int);
    void onWorkerUpdChannel(int);
    void onWorkerUpdCurrentRange(int, int);
    void onWorkerUpdVset(int, int);
    void onWorkerUpdCset(int, int, int);
    void onWorkerLog(int, QString);
    void onPortTxLog(const char *, int);
    void onPortRxLog(const char *, int);
private:
    SerialWorker *worker;
    QQueue<TaskQueueRecord_t> taskQueue;
    QMutex taskQueueMutex;
    bool connected;
    bool processingTask;
    value_cache_t vcache;

    void _invokeTaskQueued(TaskPointer fptr, void *arguments);
    bool _checkConnected(void);
    void _initialRead(void *arguments);
    void _setState(void *arguments);
    void _setCurrentRange(void *arguments);
    void _setVoltage(void *arguments);
    void _setCurrent(void *arguments);
    static QString getKeyName(int keyId);
    static QString getKeyEventType(int keyEventType);

};

#endif // SERIALTOP_H
