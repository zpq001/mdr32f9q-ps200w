#ifndef __UART_PARSER_H_
#define __UART_PARSER_H_

#include <stdint.h>



//=================================================================//
//                          UART parser test                       //
//=================================================================//
typedef struct {
    char *keyword;
    void *nextTable;
    void (*funcPtr)(char *nextArg, int32_t n);
    int32_t funcArg;
} parser_table_record_t;
//=================================================================//
//                         \ UART parser test                      //
//=================================================================//



void uart_parse(char **argv, uint8_t argc);



#endif
