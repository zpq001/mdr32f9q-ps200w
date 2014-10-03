#ifndef SERIALTOP_H
#define SERIALTOP_H

#include <QObject>
#include "serialworker.h"


class SerialTop : public QObject
{
    Q_OBJECT
public:


    explicit SerialTop(QObject *parent = 0);

signals:
    void connectedChanged(bool);
    void _log(QString text, int type);
public slots:
    void init(void);
    void connectToDevice(void);
    void disconnectFromDevice(void);
    void sendString(const QString &text);
    void keyEvent(int key, int event);
private slots:
    void onWorkerLog(int, QString);
    void onPortTxLog(const char *, int);
    void onPortRxLog(const char *, int);
private:
    bool checkConnected(void);
    QString getKeyName(int keyId);
    QString getKeyEventType(int keyEventType);
    SerialWorker *worker;

};

#endif // SERIALTOP_H
