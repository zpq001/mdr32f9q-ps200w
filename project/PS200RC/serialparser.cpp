#include "serialparser.h"
#include <QStringList>

SerialParser::SerialParser()
{
}



QByteArray SerialParser::cmd_writeVset(int channel, int newValue)
{
    QByteArray ba = "converter set_voltage ";
    ba.append((channel == 0) ? "-ch5v " : "-ch12v ");
    ba.append(QByteArray::number(newValue));
    ba.append('\r');
    //char *bytes = (char *)malloc(ba.size());    // not using terminatig \0
    //memcpy(bytes, ba.data(), ba.size());
    //return bytes;
    return ba;
}

QByteArray SerialParser::cmd_readVset(int channel)
{
    QByteArray ba = "converter get_voltage ";
    ba.append((channel == 0) ? "-ch5v" : "-ch12v");
    ba.append('\r');
    return ba;
}

QByteArray SerialParser::cmd_readVmea()
{
    QByteArray ba = "converter get_vmea";
    ba.append('\r');
    return ba;
}

int SerialParser::getMessageType(const QByteArray &ba)
{
    int msgType;
    if (ba.indexOf("[i]") != -1)
        msgType = MSG_INFO;
    else if (ba.indexOf("[a]") != -1)
        msgType = MSG_ACK;
    else
        msgType = MSG_UNKNOWN;
    return msgType;
}


int SerialParser::getAckData_Vset(const QByteArray &ba, int *value)
{
    QString valueStr;
    int errCode;
    if (_getValueForKey(ba, "-vset", valueStr) != -1)
    {
        *value = valueStr.toInt();
        errCode = 0;
    }
    else
    {
        errCode = -1;
    }
    return errCode;
}



int SerialParser::_getValueForKey(const QByteArray &ba, const QString &key, QString &value)
{
    QStringList list = (QString(ba)).split(' ', QString::SkipEmptyParts);
    int keyIndex = list.indexOf(key);
    if ((keyIndex > 0) && (keyIndex < list.size()-1))
    {
        value = list.at(keyIndex + 1);
        return 0;
    }
    else
    {
        return -1;
    }
}



