/*
 * data_stream.h
 *
 *  Created on: Dec 16, 2015
 *      Author: jcoe
 */

#ifndef DATA_STREAM_H_
#define DATA_STREAM_H_

#include <stdint.h>
#include <stdbool.h>

// TODO: FIX THIS
typedef struct
{
    uint32_t secs;
    uint32_t nsecs;
} Seconds_Time;

#ifdef __cplusplus
extern "C" {
#endif

#define F_START			0xFE
#define F_ESC			0xFD
#define F_END			0xF0

typedef struct
{
    int16_t			currentNow;
    int16_t			current0_5msAgo;
    int16_t			voltageNow;
} DataType;


/* Data Packet Structure */
typedef struct //__attribute__((__packed__))
{
    Seconds_Time	time;
    DataType		sample;
    unsigned char	id;
    uint8_t			sc;
    uint8_t			flags;
    uint8_t			spare;
    uint16_t		crc;
} DataPacket;

/* Data Stream Structure */
typedef struct {
    DataPacket		data;
    uint16_t		index;
    bool			isSOF;
    bool			escFound;
    bool			isCRCgood;
    //void (*callback)(uint8_t);
    uint8_t			msg[200];
    uint8_t			length;
} DataStream;

void data_stream_set_send_byte_CB(void (*callback)(uint8_t));
void data_stream_send(DataStream* dataHandle);

void data_stream_param_init(DataStream* stream);
bool data_stream_is_goodCRC(DataStream* stream);
bool data_stream_process_byte(DataStream* stream, uint8_t byte);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DATA_STREAM_H_ */