

#include <string.h>
#include "uartParser.h"


static void parse_error(char *nextArg, int32_t n);
static void uart_converter_control(char *nextArg, int32_t n);
static void uart_converter_set_voltage(char *nextArg, int32_t n);
static void uart_converter_set_current(char *nextArg, int32_t n);
static void uart_get_profiling(char *nextArg, int32_t n);
static void uart_key_type(char *nextArg, int32_t n);
static void uart_key_code(char *nextArg, int32_t n);



//------ Converter control keyword table ------//
parser_table_record_t parser_converter_table[] = {
    {"on",         0,                          uart_converter_control,     1 },
    {"off",        0,                          uart_converter_control,     0 },
    {"set_voltage",0,                          uart_converter_set_voltage, 0 },
    {"set_current",0,                          uart_converter_set_current, 0 },
};

//--------- Keys control keyword table --------//
parser_table_record_t parser_key_code_table[] = {
    {"esc",        0,                          uart_key_code,              0x30     },
    {"ok",         0,                          uart_key_code,              0x31     }
};

parser_table_record_t parser_key_type_table[] = {
    {"down",        &parser_key_code_table,     uart_key_type,              0x0100    },
    {"up",          &parser_key_code_table,     uart_key_type,              0x0200    },
    {0,             0,                          parse_error,                26        }
};

parser_table_record_t parser_first_table[] = {
    {"converter",   &parser_converter_table,    0,                          0   },
    {"key",         &parser_key_type_table,     0,                          0   },
    {"profiling",   0,                          uart_get_profiling,         0   },
    {0,             0,                          parse_error,                25  }
};



void uart_parse(char **argv, uint8_t argc)
{
    uint8_t i = 0;
    uint8_t tableIndex;
    char *str;
    parser_table_record_t* table = parser_first_table;
    while(argc)
    {
        str = argv[i];
        tableIndex = 0;
        while (1)
        {
            if (table[tableIndex].keyword != 0)
            {
                if (strcmp(table[tableIndex].keyword, str) == 0)
                {
                    // Equal keywords - call handler (if any)
                    if (table[tableIndex].funcPtr)
                    {
                        //table[tableIndex].funcPtr( table[tableIndex].funcArg );
                    }
                    // If there is child table, switch to it
                    if (table[tableIndex].nextTable)
                    {
                        table = table[tableIndex].nextTable;
                        tableIndex = 0;
                    }

                }
                tableIndex++;
            }
            else
            {
                // End of table
            }
        }
    }


}



static void parse_error(char *nextArg, int32_t n)
{
    // Print message from table with number n
}

static void uart_converter_control(char *nextArg, int32_t n)
{
    // if n, turn converter on, else turn off
}

static void uart_converter_set_voltage(char *nextArg, int32_t n)
{
    // convert next argument (atoi() or similar) and set conveter voltage
}

static void uart_converter_set_current(char *nextArg, int32_t n)
{
    // convert next argument (atoi() or similar) and set conveter current
}

static void uart_get_profiling(char *nextArg, int32_t n)
{
    // send back profiling information
}

static void uart_key_type(char *nextArg, int32_t n)
{
    // set global parser variable - key event type
}

static void uart_key_code(char *nextArg, int32_t n)
{
    // Combine global parser variable - key event type with argument n and send command to dispatcher
}



