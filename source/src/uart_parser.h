
#include <stdint.h>


#define PARSER_MAX_FUNCTION_ARGUMENTS   5

#define READ_NEXT   0x80
#define ATYPE_MASK  0x7F

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

typedef void (*funcPtr_t)(uint8_t uart_num, arg_t *args);

typedef struct {
    char *argName;
    uint8_t argNum;
    uint8_t argType;
    uint16_t flag;
} arg_table_record_t;


typedef struct {
    char *cmdName;
    funcPtr_t funcPtr;
    arg_table_record_t *argTable;
} func_table_record_t;






funcPtr_t parse_argv(char **argv, uint8_t argc, arg_t *parsedArguments);





