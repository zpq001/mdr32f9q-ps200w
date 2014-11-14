#include "serialparser.h"
#include <QStringList>
#include "globaldef.h"

//const char SerialParser::termSymbol = '\r';
//const char SerialParser::spaceSymbol = ' ';
const SerialParser::protocol_definition_struct SerialParser::proto;

/*

  Commands:

converter
    get_state
    get_channel
    get_crange
    get_vset
    get_cset
    get_vset_limits
    get_cset_limits
    get_protection_state

    set_state
    set_crange
    set_vset
    set_cset
    set_vset_limits
    set_cset_limits
    set_protection_state

key
    down
    up
    up_short
    up_long
    hold
    repeat

    esc
    ok
    left
    right
    left
    on
    off
    encoder


  Events:

    upd_state
    upd_channel
    upd_crange
    upd_vset
    upd_cset
    upd_vset_limits
    upd_cset_limits
    upd_protection_state
    upd_mea_params
    upd_health_state
    upd_profile

*/


//-----------------------------------------------------------------//
// Reading data

QByteArray SerialParser::cmd_readState()
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.get);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.state);
    ba.append(proto.termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readChannel()
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.get);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.channel);
    ba.append(proto.termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readCurrentRange(int channel)
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.get);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.crange);
    ba.append(proto.spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? proto.flags.ch5v : ((channel == CHANNEL_12V) ? proto.flags.ch12v : ""));
    ba.append(proto.termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readVset(int channel)
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.get);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.vset);
    ba.append(proto.spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? proto.flags.ch5v : ((channel == CHANNEL_12V) ? proto.flags.ch12v : ""));
    ba.append(proto.termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readCset(int channel, int currentRange)
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.get);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.cset);
    ba.append(proto.spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? proto.flags.ch5v : ((channel == CHANNEL_12V) ? proto.flags.ch12v : ""));
    ba.append(proto.spaceSymbol);
    ba.append((currentRange == CURRENT_RANGE_LOW) ? proto.flags.crangeLow : ((currentRange == CURRENT_RANGE_HIGH) ? proto.flags.crangeHigh : ""));
    ba.append(proto.termSymbol);
    return ba;
}






//-----------------------------------------------------------------//
// Writing data

QByteArray SerialParser::cmd_writeState(int state)
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.set);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.state);
    ba.append(proto.spaceSymbol);
    ba.append((state == CONVERTER_ON) ? proto.values.on : proto.values.off);
    ba.append(proto.termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_writeCurrentRange(int channel, int newCurrentRange)
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.set);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.crange);
    ba.append(proto.spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? proto.flags.ch5v : ((channel == CHANNEL_12V) ? proto.flags.ch12v : ""));
    ba.append(proto.spaceSymbol);
    ba.append((newCurrentRange == CURRENT_RANGE_LOW) ? proto.flags.crangeLow : ((newCurrentRange == CURRENT_RANGE_HIGH) ? proto.flags.crangeHigh : ""));
    ba.append(proto.termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_writeVset(int channel, int newValue)
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.set);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.vset);
    ba.append(proto.spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? proto.flags.ch5v : ((channel == CHANNEL_12V) ? proto.flags.ch12v : ""));
    ba.append(proto.spaceSymbol);
    ba.append(proto.keys.vset);
    ba.append(proto.spaceSymbol);
    ba.append(QByteArray::number(newValue));
    ba.append(proto.termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_writeCset(int channel, int currentRange, int newValue)
{
    QByteArray ba = proto.groups.converter;
    ba.append(proto.spaceSymbol);
    ba.append(proto.actions.set);
    ba.append(proto.spaceSymbol);
    ba.append(proto.parameters.cset);
    ba.append(proto.spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? proto.flags.ch5v : ((channel == CHANNEL_12V) ? proto.flags.ch12v : ""));
    ba.append(proto.spaceSymbol);
    ba.append((currentRange == CURRENT_RANGE_LOW) ? proto.flags.crangeLow : ((currentRange == CURRENT_RANGE_HIGH) ? proto.flags.crangeHigh : ""));
    ba.append(proto.spaceSymbol);
    ba.append(proto.keys.cset);
    ba.append(proto.spaceSymbol);
    ba.append(QByteArray::number(newValue));
    ba.append(proto.termSymbol);
    return ba;
}


//-----------------------------------------------------------------//
// Parsing acknowledge packets





/*
int SerialParser::getAckData_Vset(const QByteArray &ba, int *value)
{
    QString valueStr;
    int errCode;
    if (getValueForKey(ba, "-vset", valueStr) != -1)
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
*/


//-----------------------------------------------------------------//
// Helper

int SerialParser::getMessageType(const QByteArray &ba)
{
    int msgType;
    if (ba.indexOf(proto.message_types.ack) != -1)
        msgType = MSG_ACK;
    else
        msgType = MSG_INFO;
    return msgType;
}


int SerialParser::findKey(const QByteArray &ba, const QString &key)
{
    int keyIndex = ba.indexOf(key);
    if ((keyIndex > 0) && (keyIndex < ba.size()-1))
        return 0;
    else
        return 1;

}


int SerialParser::getValueForKey(const QByteArray &ba, const QString &key, QString &value)
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

int SerialParser::getValueForKey(const QByteArray &ba, const QString &key, int *value)
{
    QStringList list = (QString(ba)).split(' ', QString::SkipEmptyParts);
    int keyIndex = list.indexOf(key);
    if ((keyIndex > 0) && (keyIndex < list.size()-1))
    {
        QString valueStr = list.at(keyIndex + 1);
        bool conversionOk;
        *value = valueStr.toInt(&conversionOk);
        if (conversionOk)
            return 0;
        else
            return -2;
    }
    else
    {
        return -1;
    }
}



