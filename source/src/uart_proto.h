#ifndef UART_PROTO_H
#define UART_PROTO_H

#include "stdint.h"

typedef struct {
    const char termSymbol;
    const char spaceSymbol;
    struct {
        const char *ack;
        const char *info;
        const char *error;
    } message_types;
    struct {
        const char *converter;
        const char *buttons;
		const char *profiling;
		const char *test;
    } groups;
    struct {
        const char *set;
        const char *get;
    } actions;
    struct {
        const char *state;
        const char *channel;
        const char *crange;
        const char *vset;
        const char *cset;
        const char *vset_lim;
        const char *cset_lim;
        const char *protection;
        const char *measured;
    } parameters;
    struct {
        const char *state;
        const char *vset;
        const char *cset;
        const char *low_limit;
        const char *high_limit;
        const char *low_state;
        const char *high_state;
        const char *timeout;
        const char *vmea;
        const char *cmea;
        const char *pmea;
		const char *btn_event;
		const char *btn_code;
		const char *enc_delta;
    } keys;
    struct {
        const char *ch5v;
        const char *ch12v;
        const char *crangeLow;
        const char *crangeHigh;
    } flags;
    struct {
        const char *on;
        const char *off;
        const char *enable;
        const char *disable;
    } values;
    struct {
        const char *down;
        const char *up;
        const char *up_short;
        const char *up_long;
        const char *hold;
        const char *repeat;
    } button_events;
	struct {
        const char *esc;
        const char *ok;
        const char *left;
        const char *right;
        const char *on;
        const char *off;
        const char *encoder;
	} button_codes;
    struct {
        const char *no_group;
        const char *missing_key;
        const char *missing_value;
        const char *illegal_value;
        const char *unknown_error;
		const char *param_readonly;
    } errors;
} protocol_definition_struct;


enum {NO_ERROR = 0, ERROR_CANNOT_DETERMINE_GROUP, ERROR_MISSING_KEY, ERROR_MISSING_VALUE, ERROR_ILLEGAL_VALUE, ERROR_PARAM_READONLY};


extern const protocol_definition_struct proto;

#ifdef __cplusplus
extern "C" {
#endif
uint8_t splitString(char *sourceString, uint16_t sourceLength, char **words, uint8_t maxWordCount);
int16_t getIndexOfKey(char **strArray, uint8_t arraySize, const char *key);
int8_t getStringForKey(char **strArray, uint8_t arraySize, const char *key, char**result);
int8_t getValueUI32ForKey(char **strArray, uint8_t arraySize, const char *key, uint32_t *result);
int8_t getValueI32ForKey(char **strArray, uint8_t arraySize, const char *key, int32_t *result);
void startString(char *dest, const char *headerStr);
void appendToString(char *dest, const char *source);
void appendToStringLast(char *dest, const char *source);
void appendToStringU32(char *dest, const char *key, uint32_t value);
void appendToStringI32(char *dest, const char *key, int32_t value);
void terminateString(char *dest);
#ifdef __cplusplus
}
#endif


#endif // UART_PROTO_H
