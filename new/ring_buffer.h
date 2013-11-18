
#include "stdint.h"


typedef struct {
	char *data;
	uint16_t tail_index;
	uint16_t head_index;
	uint16_t write_count;
	uint16_t read_count;
	uint16_t size;
} ring_buffer_t;


#define ringBufferIsFull(rb)	((uint16_t)(rb.write_count - rb.read_count) == rb.size)
#define ringBufferIsEmpty(rb)	(rb.write_count == rb.read_count)
#define ringBufferCount(rb)		((uint16_t)(rb.write_count - rb.read_count))


void initRingBuffer(ring_buffer_t *rb, char *data, uint16_t size);
uint8_t putIntoRingBuffer(ring_buffer_t *rb, char item);
uint8_t getFromRingBuffer(ring_buffer_t *rb,  char *item);



