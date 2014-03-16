
#include <stdint.h>


#define READ_NEXT   0x80
#define ATYPE_MASK  0x7F


enum uart_cmd {
	UART_CMD_CONVERTER = 1,
	UART_CMD_KEY,
	UART_CMD_ENCODER,
	UART_CMD_PROFILING,
	UART_CMD_TEST1,
	UART_CMD_UNKNOWN
};



#pragma anon_unions

// Argument types (0x00 to 0x0F)
enum {
    ATYPE_NONE = 0,
    ATYPE_STRING,
    ATYPE_FLAG,
    ATYPE_UINT32,
    ATYPE_INT32,
    ATYPE_FLOAT
};

// Argument record
typedef struct {
    uint8_t type;
    union {
        char *string;
        uint32_t data32u;
        int32_t data32;
        uint16_t flag;
        float dataf;
    };
} arg_t;

//typedef void (*funcPtr_t)(uint8_t uart_num, arg_t *args);

typedef struct {
    char *argName;
    uint8_t argNum;
    uint8_t argType;
    uint16_t flag;
} arg_table_record_t;


typedef struct {
    char *cmdName;
    uint8_t cmdCode;
    arg_table_record_t *argTable;
} func_table_record_t;








uint8_t parse_argv(char **argv, uint8_t argc, arg_t *parsedArguments);





