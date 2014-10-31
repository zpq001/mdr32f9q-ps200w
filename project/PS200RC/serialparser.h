#ifndef SERIALPARSER_H
#define SERIALPARSER_H

#include <QString>

class SerialParser
{
public:
    enum MsgTypes {MSG_UNKNOWN, MSG_ACK, MSG_INFO};
    static const char termSymbol = '\r';

public:
    SerialParser();

    // Creating command strings
    QByteArray cmd_writeVset(int channel, int newValue);
    QByteArray cmd_readVset(int channel);
    QByteArray cmd_readVmea();

    // Getting received message type
    int getMessageType(const QByteArray &ba);

    // Getting data from event messages
    int getData_Vmea(QString &str, void *data);

    // Getting data from acknowledge messages
    int getAckData_Vset(const QByteArray &ba, int *value);


    int _getValueForKey(const QByteArray &ba, const QString &key, QString &value);

private:


};

#endif // SERIALPARSER_H
