#ifndef SERIALPARSER_H
#define SERIALPARSER_H

#include <QString>


//const char key_state[] = "-state";
//const char key_channel[] = "-channel";
//const char key_crange[] = "-crange";
//const char key_overload[] = "-overloaded";

const char val_state_on[] = "on";
const char val_state_off[] = "off";
const char val_crange_low[] = "low";

// converter set vset -channel ch5v -value 1200
// converter get vset -channel ch12v
// converter set vset_lim -channel ch5v -limit low -value 0
// converter set vset_lim -channel ch5v -limit high -value 10000
// converter get cset_lim -channel ch5v -crange low -limit low
// converter set cset_lim -channel ch5v -crange low -limit high -value 15000
// converter set protection -state on -timeout 10

// converter set vset -ch5v -vset 1200
// converter set vset -ch5v -cset 1200
// converter set cset_lim -ch5v -crange20 -lim_high 15000 -lim_low 2000
// converter set crange -ch12v -crange20

// Groups
// Actions
// Parameters
// Keys
// Flags
// Values
/*

*/





//---------------//
/*
const char cs_acknowledge[] = "ack";

const char cs_on[] = "-on";
const char cs_off[] = "-off";
const char cs_overloaded[] = "-overloaded";
const char cs_ch5v[] = "-ch5v";
const char cs_ch12v[] = "-ch12v";
const char cs_rangeLow[] = "-range20";
const char cs_rangeHigh[] = "-range40";
const char cs_limitLow[] = "-lowlimit";
const char cs_limiHigh[] = "-highlimit";
const char cs_vset[] = "-vset";
const char cs_cset[] = "-cset";

const char cs_cmdtype_converter[] = "converter";

const char cs_cmd_getState[] = "get_state";
const char cs_cmd_getChannel[] = "get_channel";
const char cs_cmd_getCrange[] = "get_crange";
const char cs_cmd_getVset[] = "get_vset";
const char cs_cmd_getCset[] = "get_cset";
const char cs_cmd_getVsetLim[] = "get_vset_limits";
const char cs_cmd_getCsetLim[] = "get_cset_limits";
const char cs_cmd_getProtectionState[] = "get_protection_state";

const char cs_cmd_setState[] = "set_state";
const char cs_cmd_setCrange[] = "set_crange";
const char cs_cmd_setVset[] = "set_vset";
const char cs_cmd_setCset[] = "set_cset";
const char cs_cmd_setVsetLim[] = "set_vset_limits";
const char cs_cmd_setCsetLim[] = "set_cset_limits";
const char cs_cmd_setProtectionState[] = "set_protection_state";

const char cs_updState[] = "upd_state";
const char cs_updChannel[] = "upd_channel";
const char cs_updCrange[] = "upd_crange";
const char cs_updVset[] = "upd_vset";
const char cs_updCset[] = "upd_cset";
const char cs_updVsetLim[] = "upd_vset_limits";
const char cs_updCsetLim[] = "upd_cset_limits";
const char cs_updProtectionState[] = "upd_protection_state";
const char cs_updMeasuredParams[] = "upd_mea_params";
const char cs_updHelthState[] = "upd_health_state";
const char cs_updProfile[] = "upd_profile";
*/

class SerialParser
{
public:
    enum MsgTypes {MSG_UNKNOWN, MSG_ACK, MSG_INFO};
    typedef struct {
        const char termSymbol = '\r';
        const char spaceSymbol = ' ';
        struct {
            const char *ack = "ack";
        } message_types;
        struct {
            const char *converter = "converter";
        } groups;
        struct {
            const char *set = "set";
            const char *get = "get";
            const char *upd = "upd";
        } actions;
        struct {
            const char *state = "state";
            const char *channel = "channel";
            const char *crange = "crange";
            const char *vset = "vset";
            const char *cset = "cset";
            const char *vset_lim = "vset_lim";
            const char *cset_lim = "cset_lim";
            const char *protection = "protection";
        } parameters;
        struct {
            const char *state = "-state";
            const char *vset = "-vset";
            const char *cset = "-cset";
            const char *low_limit = "-low_val";
            const char *high_limit = "-high_val";
            const char *low_state = "-low_state";
            const char *high_state = "-high_state";
            const char *timeout = "-timeout";
            const char *vmea = "-vm";
            const char *cmea = "-cm";
            const char *pmea = "-pm";
        } keys;
        struct {
            const char *ch5v = "-ch5v";
            const char *ch12v = "-ch12v";
            const char *crangeLow = "-crange20";
            const char *crangeHigh = "-crange40";
        } flags;
        struct {
            const char *on = "on";
            const char *off = "off";
            const char *enable = "en";
            const char *disable = "dis";
        } values;
    } protocol_definition_struct;

    static const SerialParser::protocol_definition_struct proto;


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

    // Getting data from acknowledge messages
    //static int getAckData_readState(const QByteArray &ba, int *value);
    //static int getAckData_readChannel(const QByteArray &ba, int *value);
    //static int getAckData_readCurrentRange(const QByteArray &ba, int *value);
    //static int getAckData_readVset(const QByteArray &ba, int *value);
    //static int getAckData_readCset(const QByteArray &ba, int *value);



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