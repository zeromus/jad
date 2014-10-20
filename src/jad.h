#ifndef _JAD_H
#define _JAD_H

//TODO - decide what's for internal use only and take out of the public header
//TODO - decide what we even need in the first place
//TODO - maybe split all the jadUtil into another file, or maybe theyre all for jadTool, or maybe they should be named jadTool etc.
//TODO - jadContext API for reading out the TOC? for handling redundancies and boiling down to 100? we dont wnat to forcibly store the whole thing in memory
//TODO - divide codecs from streams? i thought maybe by combining them we could support both streamed and buffered scenarios... but i dont know.

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

//error types
typedef int jadError;

#define JAD_OK 0
#define JAD_EOF -1
#define JAD_ERROR -2

//type forward declarations
typedef struct jadSubchannelQ jadSubchannelQ;
typedef struct jadStream jadStream;
typedef struct jadContext jadContext;
typedef struct jadAllocator jadAllocator;
typedef uint8_t jadBCD2;

enum jadEnumControlQ
{
	jadEnumControlQ_None = 0,

	jadEnumControlQ_StereoNoPreEmph = 0,
	jadEnumControlQ_StereoPreEmph = 1,
	jadEnumControlQ_MonoNoPreemph = 8,
	jadEnumControlQ_MonoPreEmph = 9,
	jadEnumControlQ_DataUninterrupted = 4,
	jadEnumControlQ_DataIncremental = 5,

	jadEnumControlQ_CopyProhibitedMask = 0,
	jadEnumControlQ_CopyPermittedMask = 2,
};

enum jadEncodeSettings
{
	jadEncodeSettings_None = 0,

	jadEncodeSettings_DirectCopy = 1,
};

enum jadFlags
{
	jadFlags_None = 0,

	jadFlags_JAC = 1,
};

//the read callback for a jadStream
typedef int (*jadStreamRead)(void* buffer, size_t bytes, jadStream* stream);

//the write callback for a jadStream
typedef int (*jadStreamWrite)(const void* buffer, size_t bytes, jadStream* stream);

//the seek callback for a jadStream. like fseek, but returns ftell. (0,SEEK_CUR) should work as ftell
typedef long (*jadStreamSeek)(jadStream* stream, long offset, int origin);

//the codec get callback for a jadStream, like getc, but no provision for ferror is provided
typedef int (*jadStreamGet)(jadStream* stream);

//the codec put callback for a jadStream, like putc, but no provision for ferror is provided
typedef int (*jadStreamPut)(jadStream* stream, uint8_t val);

//just used for codec flushing
typedef int (*jadStreamFlush)(jadStream* stream);

//in case we need it
typedef void* (*jadAllocatorAlloc)(int amt, int alignment);
typedef void* (*jadAllocatorFree)(void* ptr); 

//a stdio-like stream to be used for random access. fill it with function pointers and pass to jadOpen
struct jadStream
{
	void* opaque;
	jadStreamRead read;
	jadStreamWrite write;
	jadStreamSeek seek;
	jadStreamGet get;
	jadStreamPut put;
	jadStreamFlush flush;
};

//a simple memory allocator interface, in case we need it
struct jadAllocator
{
	void* opaque;
	jadAllocatorAlloc alloc;
	jadAllocatorFree free;
} ;

//a timestamp like the format stored on a disc
struct jadTimestamp
{
	jadBCD2 MIN, SEC, FRAC, ABA;
};

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
	jadBCD2 min, sec, frame;

	//This is supposed to be zero.. maybe it's useful for copy protection or something
	uint8_t zero;

	//These are the second set of timestamps.  Meaning varies:
	//check yellowbook 22.3.3 and 22.3.4
	//normal track: absolute timestamp
	//leadin track: timestamp of toc entry
	jadBCD2 ap_min, ap_sec, ap_frame;

	//The CRC. This is the actual CRC value as would be calculated from our library (it is inverted and written big endian to the disc)
	//Don't assume this CRC is correct-- If this SubchannelQ was read from a dumped disc, the CRC might be wrong.
	//CCD doesnt specify this for TOC entries, so it will be wrong. It may or may not be right for data track sectors from a CCD file.
	//Or we may have computed this SubchannelQ data and generated the correct CRC at that time.
	uint16_t q_crc;
};

//Retrieves the initial set of timestamps (min,sec,frac) as a convenient jadTimestamp from a jadSubchannelQ
struct jadTimestamp jadSubchannelQ_GetTimestamp(jadSubchannelQ* q);

//Retrieves the second set of timestamps (ap_min, ap_sec, ap_frac) as a convenient jadTimestamp from a jadSubchannelQ
struct jadTimestamp jadSubchannelQ_GetAPTimestamp(jadSubchannelQ* q);

//computes a subchannel Q status byte from the provided adr and control values
uint8_t jadSubchannelQ_ComputeStatus(uint32_t adr, enum jadEnumControlQ control); // { return (uint8_t)(adr | (((int)control) << 4)); }
	
//Retrives the ADR field of the q_status member (low 4 bits) of a jadSubchannelQ
int jadSubchannelQ_GetADR(jadSubchannelQ* q); // { get { return q_status & 0xF; } }

//Retrieves the CONTROL field of the q_status member (high 4 bits) of a jadSubchannelQ
enum jadEnumControlQ jadSubchannelQ_CONTROL(jadSubchannelQ* q);

//sets a jadSubchannelQ status from the provided adr and control values
void jadSubchannelQ_SetStatus(uint8_t adr, enum jadEnumControlQ control); // { q_status = ComputeStatus(adr, control); }

//one sector of data in the 2448 byte raw JAD format
struct jadSector
{
	union {
		struct {
			uint8_t data[2352];
			uint8_t subcode[96];
		};
		uint8_t entire[2448];
	};
};

//a single TOC entry (essentially the contents of the Q subchannel)
typedef uint8_t jadTOCEntry[12];

//clears a jadTOCEntry to zeroes
void jadTOCEntry_Clear(jadTOCEntry* tocentry);

//the context for JAD-reading operations. 
//consider it as opaque (full declaration provided for easier static allocation)
//create with jadOpen, etc.
struct jadContext
{
	struct jadStream* stream;
	struct jadAllocator* allocator;
	uint32_t flags, numSectors, numTocEntries;
};

//opens a jadContext, which gets its data from the provided stream and allocator
int jadOpen(jadContext* jad, jadStream* stream, jadAllocator* allocator);

//writes the given jad to the output stream as a JAC file
int jadWriteJAC(jadContext* jad, jadStream* stream);

//writes the given jad to the output stream as a JAD file
int jadWriteJAD(jadContext* jad, jadStream* stream);

//closes a jadContext opened with jadOpen. do not call on unopened jadContexts.
int jadClose(jadContext* jad);

//converts a 2352-byte BIN file to JAD
void jadUtil_Convert2352ToJad(const char* inpath, const char* outpath);

//compares two files for identity
int jadUtil_CompareFiles(const char* inpath, const char* inpath2);

#ifdef  __cplusplus
}
#endif

#endif //_JAD_H
