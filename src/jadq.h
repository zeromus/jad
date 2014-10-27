#ifndef _JADQ_H
#define _JADQ_H

#include "jad.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct jadSubchannelQ
{
	//ADR and CONTROL
	uint8_t q_status;

	//normal track: BCD indications of the current track number
	//leadin track: should be 0 
	uint8_t q_tno;

	//normal track: BCD indications of the current index
	//leadin track: 'POINT' field used to ID the TOC entry #
	uint8_t q_index;

	//These are the initial set of timestamps. Meaning varies:
	//check yellowbook 22.3.3 and 22.3.4
	//normal track: relative timestamp
	//leadin track: unknown
	jadTimestamp q_timestamp;

	//This is supposed to be zero.. maybe it's useful for copy protection or something
	uint8_t zero;

	//These are the second set of timestamps.  Meaning varies:
	//check yellowbook 22.3.3 and 22.3.4
	//normal track: absolute timestamp
	//leadin track: timestamp of toc entry
	jadTimestamp q_apTimestamp;

	//The CRC. This is the actual CRC value as would be calculated from our library (it is inverted and written big endian to the disc)
	//Don't assume this CRC is correct-- If this SubchannelQ was read from a dumped disc, the CRC might be wrong.
	//CCD doesnt specify this for TOC entries, so it will be wrong. It may or may not be right for data track sectors from a CCD file.
	//Or we may have computed this SubchannelQ data and generated the correct CRC at that time.
	uint16_t q_crc;
};

//Reads the subchannel from the specified buffer
void jadSubchannelQ_DeserializeFromDeinterleaved(jadSubchannelQ* q, void* buf);

//Writes the subchannel to the specified buffer
void jadSubchannelQ_SerializeToDeinterleaved(jadSubchannelQ* q, void* buf);

//Retrieves the initial set of timestamps (min,sec,frac) as a convenient jadTimestamp from a jadSubchannelQ
jadTimestamp jadSubchannelQ_GetTimestamp(jadSubchannelQ* q);

//Retrieves the second set of timestamps (ap_min, ap_sec, ap_frac) as a convenient jadTimestamp from a jadSubchannelQ
jadTimestamp jadSubchannelQ_GetAPTimestamp(jadSubchannelQ* q);

//computes a subchannel Q status byte from the provided adr and control values
uint8_t jadSubchannelQ_ComputeStatus(uint32_t adr, jadEnumControlQ control); // { return (uint8_t)(adr | (((int)control) << 4)); }
	
//Retrives the ADR field of the q_status member (low 4 bits) of a jadSubchannelQ
int jadSubchannelQ_GetADR(jadSubchannelQ* q); // { get { return q_status & 0xF; } }

//Retrieves the CONTROL field of the q_status member (high 4 bits) of a jadSubchannelQ
enum jadEnumControlQ jadSubchannelQ_CONTROL(jadSubchannelQ* q);

//Sets a jadSubchannelQ status from the provided adr and control values
void jadSubchannelQ_SetStatus(uint8_t adr, jadEnumControlQ control); // { q_status = ComputeStatus(adr, control); }

//Synthesizes a Q subchannel for a leadin track using the provided reference as a template (the timestampsand current sense of a timestamp
//not sure this declaration makes sense? should it be populated based on TOCEntries?
void jadSubchannelQ_SynthesizeLeadin(jadSubchannelQ* qOut, jadSubchannelQ* qTemplate, jadTimestamp* MSF, jadTimestamp* AMSF);

//Synthesizes a Q subchannel for a user data area track using the provided reference as a template (the timestampsand current sense of a timestamp
//TODO - this method is inefficient if you're just gonna serialize the results. make a variant which writes to a buffer
void jadSubchannelQ_SynthesizeUser(jadSubchannelQ* qOut, jadSubchannelQ* qTemplate, jadTimestamp* MSF, jadTimestamp* AMSF);

#ifdef  __cplusplus
}
#endif

#endif //_JADQ_H
