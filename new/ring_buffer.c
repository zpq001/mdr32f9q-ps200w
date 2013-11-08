
#include "stdint.h"
#include "ring_buffer.h"



void initRingBuffer(ring_buffer_t *rb, char *data, uint16_t size)
{
	rb->read_count = 0xFFFA;
	rb->write_count = 0xFFFA;
	rb->head_index = 0;
	rb->tail_index = 0;
	rb->size = size;
	rb->data = data;
}


uint8_t putIntoRingBuffer(ring_buffer_t *rb, char item)
{
	if (rb->write_count - rb->read_count < rb->size)
	{
		rb->data[rb->tail_index] = item;
		rb->tail_index = (rb->tail_index == rb->size) ? 0 : rb->tail_index + 1;
		rb->write_count++;
		return 1;
	}
	return 0;
}


uint8_t getFromRingBuffer(ring_buffer_t *rb,  char *item)
{
	if (rb->write_count - rb->read_count > 0)
	{
		*item = rb->data[rb->head_index];
		rb->head_index = (rb->head_index == rb->size) ? 0 : rb->head_index + 1;
		rb->read_count++;
		return 1;
	}
	return 0;
}













