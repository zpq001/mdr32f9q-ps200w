
#include <stdint.h>
#include "utils.h"

//-------------------------------------------------------//
//	Converts 32-bit integer to a right - aligned string
//	input:
//		val - 32-bit value to convert, signed
//		*buffer - pointer to store result data
//		len[6:0] - output string length, including \0 symbol
//				If result length < len[6:0], result will be cut to fit buffer
//				If result length > len[6:0], buffer will be filled with spaces
//		len[7] - if set, result string will be null-terminated
//  output:
//      returns number of usefull chars (those not spaces or '-' sign)
//-------------------------------------------------------//
uint8_t i32toa_align_right(int32_t val, char *buffer, uint8_t len, uint8_t min_digits_required)
{
    uint8_t is_negative = 0;
    uint8_t chars_count = 0;
    if (!len)	return 0;

    if (val < 0)
    {
        val = -val;
        is_negative = 1;
    }

    if (len & NO_TERMINATING_ZERO)
    {
        len = len & ~NO_TERMINATING_ZERO;
        buffer += len;
    }
    else
    {
        buffer += len;
        *buffer = 0;
        chars_count++;
        min_digits_required++;
    }

    do
    {
        *--buffer = val % 10 + '0';
        val /= 10;
        len--;
        chars_count++;
    }
    while ((val != 0) && len);

    // Pad with 0
    while ((chars_count < min_digits_required) && (len))
    {
        chars_count++;
        *--buffer = '0';
        len--;
    }

    // Minus sign
    if ( (len) && (is_negative) )
    {
        *--buffer = '-';
        len--;
    }

    // Padding with spaces
    while(len--)
        *--buffer = ' ';

    return chars_count;
}



