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
        int newValue;
    } setVsetArgs_t;

signals:
    //-------- Public signals -------//
    void connectedChanged(bool);
    void _log(QString text, int type);
    void bytesTransmitted(int);
    void bytesReceived(int);
    void updVmea(int);
    void updCmea(int);
    void updPmea(int);
    void updVset(int);
    void updIset(int, int, int);
    //------- Private signals -------//
    void signal_Terminate();
    void signal_ProcessTaskQueue();
public slots:
    void init(void);
    void connectToDevice(void);
    void disconnectFromDevice(void);
    void sendString(const QString &text);
    void keyEvent(int key, int event);
    //int setState(int value);
    //int setCurrentRange(int channel, int value);
    void setVoltage(int channel, int value);
    //int setCurrent(int channel, int range, int value);
private slots:

    void onWorkerLog(int, QString);
    void onPortTxLog(const char *, int);
    void onPortRxLog(const char *, int);
    void processTaskQueue();
private:
    void _setVoltage(void *arguments);


    bool checkConnected(void);
    bool connected;
    bool processingTask;
    static QString getKeyName(int keyId);
    static QString getKeyEventType(int keyEventType);
    SerialWorker *worker;

    QQueue<TaskQueueRecord_t> taskQueue;
    QMutex taskQueueMutex;


};

#endif // SERIALTOP_H
