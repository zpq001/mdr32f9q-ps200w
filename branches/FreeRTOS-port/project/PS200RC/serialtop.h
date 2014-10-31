#ifndef SERIALTOP_H
#define SERIALTOP_H

#include <QObject>
#include <QEventLoop>
#include "serialworker.h"



class SerialTop : public QObject
{
    Q_OBJECT
public:
    typedef void (SerialTop::*Some_fnc_ptr)(int, int);

    explicit SerialTop(QObject *parent = 0);

    ////////////////
    void requestForParam1(int arg1, int arg2);
    void getParam1(int arg1, int arg2);
signals:
    void sig_execute(/*Some_fnc_ptr ptr,*/ int arg1, int arg2);
public slots:
    void execute(/*Some_fnc_ptr ptr,*/ int arg1, int arg2);
    ////////////////

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
private:
    bool checkConnected(void);
    bool connected;
    bool busy;
    QString getKeyName(int keyId);
    QString getKeyEventType(int keyEventType);
    SerialWorker *worker;


};

#endif // SERIALTOP_H
