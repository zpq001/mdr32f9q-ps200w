#ifndef SERIALPARSER_H
#define SERIALPARSER_H

#include <QString>


class SerialParser
{
public:
    enum MsgTypes {MSG_UNKNOWN, MSG_ACK, MSG_INFO};
    enum Errors {NO_ERROR, PACKET_ERROR = -2, CONVERSION_ERROR = -1};



public:

    // Creating command strings
    static QByteArray cmd_readState();
    static QByteArray cmd_readChannel();
    static QByteArray cmd_readCurrentRange(int channel);
    static QByteArray cmd_readVset(int channel);
    static QByteArray cmd_readCset(int channel, int currentRange);

    static QByteArray cmd_writeState(int state);
    static QByteArray cmd_writeCurrentRange(int channel, int newCurrentRange);
    static QByteArray cmd_writeVset(int channel, int newValue);
    static QByteArray cmd_writeCset(int channel, int currentRange, int newValue);

    // Getting data from incoming messages
    static int getVmea(const QStringList &list, int *value);
    static int getCmea(const QStringList &list, int *value);
    static int getPmea(const QStringList &list, int *value);
    static int getState(const QStringList &list, int *value);
    static int getChannel(const QStringList &list, int *value);
    static int getCurrentRange(const QStringList &list, int *value);
    static int getVset(const QStringList &list, int *value);
    static int getCset(const QStringList &list, int *value);


    //static int getAckData_readState(const QByteArray &ba, int *value);
    //static int getAckData_readChannel(const QByteArray &ba, int *value);
    //static int getAckData_readCurrentRange(const QByteArray &ba, int *value);
    //static int getAckData_readVset(const QByteArray &ba, int *value);
    //static int getAckData_readCset(const QByteArray &ba, int *value);


    static QStringList splitPacket(const QByteArray &ba);
    static int findKey(const QStringList &list, const QString &key);
    static int getValueForKey(const QStringList &list, const QString &key, QString &value);
    static int getValueForKey(const QStringList &list, const QString &key, int *value);

    // Getting received message type
    static int getMessageType(const QByteArray &ba);
    // Getting data from event messages
    //static int getData_Vmea(QString &str, void *data);



    static int findKey(const QByteArray &ba, const QString &key);
    static int getValueForKey(const QByteArray &ba, const QString &key, QString &value);
    static int getValueForKey(const QByteArray &ba, const QString &key, int *value);


private:


};




#endif // SERIALPARSER_H
