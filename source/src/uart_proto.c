
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "uart_proto.h"

// Damn c++ forbids aggregate initialization
// This file is also used for embedded code in C environment, so
// solution is exact in-order initializers with comments :(

const protocol_definition_struct proto = {
        '\r',               // termSymbol
        ' ',                // spaceSymbol
        //------- message types -------//
        {
            "ack",          // .ack
            "info",         // .info
            "error"         // .error
        },
        //----------- groups ----------//
        {
            "converter",    // .converter
            "button",       // .buttons
			"profiling",    // .profiling
			"test"          // .test
        },
        //---------- actions ----------//
        {
            "set",          // .set
            "get",          // .get
        },
        //-------- parameters --------//
        {
            "state",        // .state
            "channel",      // .channel
            "crange",       // .crange
            "vset",         // .vset
            "cset",         // .cset
            "vset_lim",     // .vset_lim
            "cset_lim",     // .cset_lim
            "protection",   // .protection
            "measured"      // .measured
        },
        //----------- keys -----------//
        {
            "-state",       // .state
            "-vset",        // .vset
            "-cset",        // .cset
            "-low_val",     // .low_limit
            "-high_val",    // .high_limit
            "-low_state",   // .low_state
            "-high_state",  // .high_state
            "-timeout",     // .timeout
            "-vm",          // .vmea
            "-cm",          // .cmea
            "-pm",          // .pmea
            "-event",       // .btn_event
            "-code",        // .btn_code
            "-enc_delta"    // .enc_delta
        },
        //----------- flags ----------//
        {
            "-ch5v",        // .ch5v
            "-ch12v",       // .ch12v
            "-crange20",    // .crangeLow
            "-crange40",    // .crangeHigh
        },
        //---------- values ----------//
        {
            "on",           // .on
            "off",          // .off
            "en",           // .enable
            "dis",          // .disable
        },
        //------- button events ------//
        {
            "down",         // .down
            "up",           // .up
            "up_short",     // .up_short
            "up_long",      // .up_long
            "hold",         // .hold
            "repeat"        // .repeat
        },
        //------- button codes -------//
        {
            "esc",          // .esc
            "ok",           // .ok
            "left",         // .left
            "right",        // .right
            "on",           // .on
            "off",          // .off
            "encoder",      // .encoder
        },
        //------ error messages ------//
        {
            "unknown command group",          // .no_group
            "missing key",                    // .missing_key
            "missing value for key",          // .missing_value
            "illegal value",                  // .illegal_value
            "unknown error",                  // .unknown_error
            "parameter read-only"             // .param_readonly
        }
};



//-------------------------------------------------------//
// String into words splitter
//
// Replaces spacing and terminating symbols with '\0'
//-------------------------------------------------------//
uint8_t splitString(char *sourceString, uint16_t sourceLength, char **words, uint8_t maxWordCount)
{
    uint16_t wordCount = 0;
    uint16_t i = 0;
    uint8_t search_for_word = 1;
    uint8_t foundEOM = 0;
    char temp_char;

    while ((i < sourceLength) && (wordCount < maxWordCount) && (foundEOM == 0))
    {
            temp_char = sourceString[i];
            if (temp_char == proto.spaceSymbol)
            {
                sourceString[i] = '\0';
                search_for_word = 1;
            }
            else if (temp_char == proto.termSymbol)
            {
                sourceString[i] = '\0';
                foundEOM = 1;
            }
            else
            {
                // Normal char
                if (search_for_word == 1)
                {
                    words[wordCount++] = &sourceString[i];		// Found start position of a word
                    search_for_word = 0;
                }
            }
            i++;
    }
    return wordCount;
}

int16_t getIndexOfKey(char **strArray, uint8_t arraySize, const char *key)
{
    uint8_t i;
    int16_t keyIndex = -1;
    for (i=0; (i<arraySize) && (keyIndex == -1); i++)
    {
        if (strcmp(strArray[i], key) == 0)
            keyIndex = i;
    }
    return keyIndex;
}

int8_t getStringForKey(char **strArray, uint8_t arraySize, const char *key, char**result)
{
    int16_t index = getIndexOfKey(strArray, arraySize, key);
    uint8_t resultCode;
    if (index >= 0)
    {
        if (index < arraySize - 1)
        {
            resultCode = 0;
            *result = strArray[index + 1];
        }
        else
        {
            resultCode = ERROR_MISSING_VALUE;
        }
    }
    else
    {
        resultCode = ERROR_MISSING_KEY;
    }
    return resultCode;
}

int8_t getValueUI32ForKey(char **strArray, uint8_t arraySize, const char *key, uint32_t *result)
{
    int16_t index = getIndexOfKey(strArray, arraySize, key);
    uint8_t resultCode;
    if (index >= 0)
    {
        if (index < arraySize - 1)
        {
            resultCode = 0;
            *result = (uint32_t)strtoul(strArray[index + 1], 0, 0);
        }
        else
        {
            resultCode = ERROR_MISSING_VALUE;
        }
    }
    else
    {
        resultCode = ERROR_MISSING_KEY;
    }
    return resultCode;
}

int8_t getValueI32ForKey(char **strArray, uint8_t arraySize, const char *key, int32_t *result)
{
    int16_t index = getIndexOfKey(strArray, arraySize, key);
    uint8_t resultCode;
    if (index >= 0)
    {
        if (index < arraySize - 1)
        {
            resultCode = 0;
            *result = (uint32_t)strtol(strArray[index + 1], 0, 0);
        }
        else
        {
            resultCode = ERROR_MISSING_VALUE;
        }
    }
    else
    {
        resultCode = ERROR_MISSING_KEY;
    }
    return resultCode;
}


void startString(char *dest, const char *headerStr)
{
    dest[0] = '\0';
    strcat(dest, headerStr);
}

void appendToString(char *dest, const char *source)
{
    strcat(dest, &proto.spaceSymbol);
    strcat(dest, source);
}

void appendToStringLast(char *dest, const char *source)
{
    strcat(dest, &proto.spaceSymbol);
    strcat(dest, source);
    terminateString(dest);
}

void appendToStringU32(char *dest, const char *key, uint32_t value)
{
    char tempStr[12];
    snprintf(tempStr, 12, "%u", value);
    strcat(dest, &proto.spaceSymbol);
    strcat(dest, key);
    strcat(dest, &proto.spaceSymbol);
    strcat(dest, tempStr);
}

void appendToStringI32(char *dest, const char *key, int32_t value)
{
    char tempStr[12];
    snprintf(tempStr, 12, "%d", value);
    strcat(dest, &proto.spaceSymbol);
    strcat(dest, key);
    strcat(dest, &proto.spaceSymbol);
    strcat(dest, tempStr);
}

void terminateString(char *dest)
{
    strcat(dest, &proto.termSymbol);
}





