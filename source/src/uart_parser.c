
#include "stdint.h"
#include <string.h>
#include "stdlib.h"     /* strtol */

#include "uart_parser.h"
#include "key_def.h"
#include "global_def.h"


//-----------------------------------//
//-----------------------------------//



arg_table_record_t arg_table_converter[] = {
//    argument name   | argument number |       argument type      |  flag
    { "off",                    0,        ATYPE_FLAG,                 0   },
    { "on",                     0,        ATYPE_FLAG,                 1   },
    { "set_voltage",            0,        ATYPE_FLAG,                 2   },
    { "set_current",            0,        ATYPE_FLAG,                 3   },
    { "-ch5v",                  1,        ATYPE_FLAG,                 CHANNEL_5V   			},
    { "-ch12v",                 1,        ATYPE_FLAG,                 CHANNEL_12V  			},
    { "-range20",               2,        ATYPE_FLAG,                 CURRENT_RANGE_LOW   	},
    { "-range40",               2,        ATYPE_FLAG,                 CURRENT_RANGE_HIGH   	},
    { "-v",                     3,        ATYPE_UINT32 | READ_NEXT,   0   },
    { 0,                        0,        ATYPE_NONE,                 0   }
};



arg_table_record_t arg_table3[] = {
//    argument name   | argument number |       argument type      |  flag
    { "down",                   0,        ATYPE_FLAG,                 BTN_EVENT_DOWN   		},   
	{ "up",                   	0,        ATYPE_FLAG,                 BTN_EVENT_UP   		},   
    { "up_short",               0,        ATYPE_FLAG,                 BTN_EVENT_UP_SHORT   	},
	{ "up_long",                0,        ATYPE_FLAG,                 BTN_EVENT_UP_LONG   	},
    { "hold",                   0,        ATYPE_FLAG,                 BTN_EVENT_HOLD   		},
	{ "repeat",                 0,        ATYPE_FLAG,                 BTN_EVENT_REPEAT		},
    { "esc",                    1,        ATYPE_FLAG,                 BTN_ESC   			},   
    { "ok",                     1,        ATYPE_FLAG,                 BTN_OK   				},
    { "left",                   1,        ATYPE_FLAG,                 BTN_LEFT   			},
    { "right",                  1,        ATYPE_FLAG,                 BTN_RIGHT   			},
    { "on",                     1,        ATYPE_FLAG,                 BTN_ON   				},
    { "off",                    1,        ATYPE_FLAG,                 BTN_OFF   			},
    { "encoder",                1,        ATYPE_FLAG,                 BTN_ENCODER   		},
    { 0,                        0,        ATYPE_NONE,                 0   					}
};

arg_table_record_t arg_table4[] = {
//    argument name   | argument number |       argument type      |  flag
    { 0,                        0,        ATYPE_INT32,                 0   					}
};


func_table_record_t func_table[] = {
//    command name      |       function            |       argument table
    { "converter",          UART_CMD_CONVERTER,           arg_table_converter      	},
    { "key",                UART_CMD_KEY,                 arg_table3          		},
	{ "encoder",            UART_CMD_ENCODER,             arg_table4          		},
    { "profiling",          UART_CMD_PROFILING,       			0                   },
	{ "test1",          	UART_CMD_TEST1,       				0                   },
    //{ "get_voltage",        &converter_get_voltage,         arg_table2          },
    //{ "get_current",        &converter_get_current,         arg_table2          },
    //{ "get_state",          &converter_get_state,           0                   },
    {0,                     UART_CMD_UNKNOWN,                   0                   }
};



static void getFunction(func_table_record_t *searchTable, char* keyword, uint8_t *function, arg_table_record_t **argTable)
{
    func_table_record_t *rec;
    do
        rec = searchTable++;
    while ((rec->cmdName) && (strcmp(keyword, rec->cmdName)));
    // Function name match or end of table
    *function = rec->cmdCode;
    *argTable = rec->argTable;
}


static arg_table_record_t *getArgument(arg_table_record_t *searchTable, char* keyword)
{
    arg_table_record_t *rec;
    do
        rec = searchTable++;
    while((rec->argName) && (strcmp(keyword, rec->argName)));
    return rec;
}





uint8_t parse_argv(char **argv, uint8_t argc, arg_t *parsedArguments)
{
    uint8_t cmdCode;
    arg_table_record_t *argTable;
    arg_table_record_t *argRecord;
    uint8_t argIndex = 0;
    // Check argument count
    if (argc < 1) return 0;
    // Get function pointer for command (command is argv[0])
    getFunction(func_table, argv[argIndex++], &cmdCode, &argTable);
    argc--;
    if (argTable)
    {
        // Read argument vector and fill parsedArguments array
        while (argc!=0)
        {
            argRecord = getArgument(argTable, argv[argIndex]);
            if (argRecord->argType & READ_NEXT)
            {
                argIndex++;
                if (--argc == 0)
                    break;
            }
            switch (argRecord->argType & ATYPE_MASK)
            {
                case ATYPE_STRING:
                    parsedArguments[argRecord->argNum].string = argv[argIndex];   // from input argv[]
                    parsedArguments[argRecord->argNum].type = ATYPE_STRING;
                    break;
                case ATYPE_FLAG:
                    parsedArguments[argRecord->argNum].flag = argRecord->flag;    // from argument table
                    parsedArguments[argRecord->argNum].type = ATYPE_FLAG;
                    break;
                case ATYPE_UINT32:
                    parsedArguments[argRecord->argNum].data32u = (uint32_t)strtoul(argv[argIndex], 0, 0);
                    parsedArguments[argRecord->argNum].type = ATYPE_UINT32;
                    break;
                case ATYPE_INT32:
                    parsedArguments[argRecord->argNum].data32 = (int32_t)strtol(argv[argIndex], 0, 0);
                    parsedArguments[argRecord->argNum].type = ATYPE_INT32;
                    break;
                case ATYPE_FLOAT:
                    parsedArguments[argRecord->argNum].dataf = strtof(argv[argIndex], 0);
                    parsedArguments[argRecord->argNum].type = ATYPE_FLOAT;
                    break;
                default:    // ATYPE_NONE
                    break;
            }
            argIndex++;
            argc--;
        }
    }
    return cmdCode;
}










