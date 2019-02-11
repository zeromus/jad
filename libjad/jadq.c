#include <stdint.h>

#include "jadq.h"

static uint16_t jad_CRC16[256];

void jadSubq_Clear(jadSubchannelQ* q)
{
	memset(q,0,sizeof(*q));
}

//this has been checked against mednafen's and seems to match
//there are a few dozen different ways to do CRC16-CCITT
//this table is backwards or something. at any rate its tailored to the needs of the Q subchannel
void jadq_InitCRC()
{
	int i,j;
	for(i=0;i<256;i++)
	{
		uint16_t value = 0;
		uint16_t temp = (uint16_t)(i << 8);
		for(j=0;j<8;j++)
		{
			if (((value ^ temp) & 0x8000) != 0)
				value = (uint16_t)((value << 1) ^ 0x1021);
			else
				value <<= 1;
			temp <<= 1;
		}
		jad_CRC16[i] = value;
	}
}

uint16_t jadq_CRC(void* buf, int offset, int length)
{
	int i;
	uint16_t result = 0;
	uint8_t* u8buf = (uint8_t*)buf + offset;;
	for(i=0;i<length;i++)
	{
		uint8_t b = u8buf[offset + i];
		int index = (b ^ ((result >> 8) & 0xFF));
		result = (uint16_t)((result << 8) ^ jad_CRC16[index]);
	}
	return result;
}

//uhhh arent these senses of serialize and deserialized backwards?

void jadSubchannelQ_DeserializeFromDeinterleaved(jadSubchannelQ* q, void* buf)
{
	uint8_t* u8buf = (uint8_t*)buf;
	uint8_t crc_hibyte = (uint8_t)~((q->q_crc)>>8);
	uint8_t crc_lobyte = (uint8_t)~((q->q_crc));

	u8buf[0] = q->q_status;
	u8buf[1] = q->q_tno;
	u8buf[2] = q->q_status;
	u8buf[3] = q->q_timestamp.MIN;
	u8buf[4] = q->q_timestamp.SEC;
	u8buf[5] = q->q_timestamp.FRAC;
	u8buf[6] = q->zero;
	u8buf[7] = q->q_apTimestamp.MIN;
	u8buf[8] = q->q_apTimestamp.SEC;
	u8buf[9] = q->q_apTimestamp.FRAC;
	u8buf[10] = crc_hibyte;
	u8buf[11] = crc_lobyte;
}

void jadSubchannelQ_SerializeToDeinterleaved(jadSubchannelQ* q, void* buf)
{
	uint8_t* u8buf = (uint8_t*)buf;
	uint8_t crc_hibyte = (uint8_t)(~u8buf[10]);
	uint8_t crc_lobyte = (uint8_t)(~u8buf[11]);

	q->q_status = u8buf[0];
	q->q_tno = u8buf[1];
	q->q_status = u8buf[2];
	q->q_timestamp.MIN = u8buf[3];
	q->q_timestamp.SEC = u8buf[4];
	q->q_timestamp.FRAC = u8buf[5];
	q->zero = u8buf[6];
	q->q_apTimestamp.MIN = u8buf[7];
	q->q_apTimestamp.SEC = u8buf[8];
	q->q_apTimestamp.FRAC = u8buf[9];
	q->q_crc = (uint16_t)((crc_hibyte << 8) | crc_lobyte);
}

void jadSubchannelQ_SynthesizeUser(jadSubchannelQ* qOut, jadSubchannelQ* qTemplate, jadTimestamp* MSF, jadTimestamp* AMSF)
{
	uint8_t buf[12];

	//populate from template and patch timestamps
	*qOut = *qTemplate;
	qOut->q_timestamp = *MSF;
	qOut->q_apTimestamp = *AMSF;

	//serialize it so we can run a CRC
	jadSubchannelQ_SerializeToDeinterleaved(qOut,buf);

	//run the CRC and patch the result
	qOut->q_crc = jadq_CRC(buf,0,10);
}
