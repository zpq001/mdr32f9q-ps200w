#include "serialparser.h"
#include <QStringList>
#include "globaldef.h"

const char SerialParser::termSymbol = '\r';
const char SerialParser::spaceSymbol = ' ';
//const char SerialParser::cs_ch5v[] = "-ch5v";
//const char SerialParser::cs_ch12v[] = "-ch12v";
//const char SerialParser::cs_rangeLow[] = "-range20";
//const char SerialParser::cs_rangeHigh[] = "-range40";





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
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_getState);
    ba.append(termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readChannel()
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_getChannel);
    ba.append(termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readCurrentRange(int channel)
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_getCrange);
    ba.append(spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? cs_ch5v : ((channel == CHANNEL_12V) ? cs_ch12v : ""));
    ba.append(termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readVset(int channel)
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_getVset);
    ba.append(spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? cs_ch5v : ((channel == CHANNEL_12V) ? cs_ch12v : ""));
    ba.append(termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_readCset(int channel, int currentRange)
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_getCset);
    ba.append(spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? cs_ch5v : ((channel == CHANNEL_12V) ? cs_ch12v : ""));
    ba.append(spaceSymbol);
    ba.append((currentRange == CURRENT_RANGE_LOW) ? cs_rangeLow : ((currentRange == CURRENT_RANGE_HIGH) ? cs_rangeHigh : ""));
    ba.append(termSymbol);
    return ba;
}






//-----------------------------------------------------------------//
// Writing data

QByteArray SerialParser::cmd_writeState(int state)
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_setState);
    ba.append(spaceSymbol);
    ba.append((state == CONVERTER_ON) ? cs_on : cs_off);
    ba.append(termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_writeCurrentRange(int channel, int newCurrentRange)
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_setCrange);
    ba.append(spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? cs_ch5v : ((channel == CHANNEL_12V) ? cs_ch12v : ""));
    ba.append(spaceSymbol);
    ba.append((newCurrentRange == CURRENT_RANGE_LOW) ? cs_rangeLow : ((newCurrentRange == CURRENT_RANGE_HIGH) ? cs_rangeHigh : ""));
    ba.append(termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_writeVset(int channel, int newValue)
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_setVset);
    ba.append(spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? cs_ch5v : ((channel == CHANNEL_12V) ? cs_ch12v : ""));
    ba.append(spaceSymbol);
    ba.append(cs_vset);
    ba.append(spaceSymbol);
    ba.append(QByteArray::number(newValue));
    ba.append(termSymbol);
    return ba;
}

QByteArray SerialParser::cmd_writeCset(int channel, int currentRange, int newValue)
{
    QByteArray ba = cs_cmdtype_converter;
    ba.append(spaceSymbol);
    ba.append(cs_cmd_setCset);
    ba.append(spaceSymbol);
    ba.append((channel == CHANNEL_5V) ? cs_ch5v : ((channel == CHANNEL_12V) ? cs_ch12v : ""));
    ba.append(spaceSymbol);
    ba.append((currentRange == CURRENT_RANGE_LOW) ? cs_rangeLow : ((currentRange == CURRENT_RANGE_HIGH) ? cs_rangeHigh : ""));
    ba.append(spaceSymbol);
    ba.append(cs_cset);
    ba.append(spaceSymbol);
    ba.append(QByteArray::number(newValue));
    ba.append(termSymbol);
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
    if (ba.indexOf(cs_acknowledge) != -1)
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



