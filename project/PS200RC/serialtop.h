#ifndef SERIALTOP_H
#define SERIALTOP_H

#include <QObject>
#include "serialworker.h"


class SerialTop : public QObject
{
    Q_OBJECT
public:
    SerialWorker *worker;

    explicit SerialTop(QObject *parent = 0);

signals:
    void connectedChanged(bool);
    void _log(QString text, int type);
    void initDone(void);
public slots:
    void init(void);
    void connectToDevice(void);
    void disconnectFromDevice(void);

private:


};

#endif // SERIALTOP_H
