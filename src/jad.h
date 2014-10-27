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

typedef enum
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
} jadEnumControlQ;

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

//the seek callback for a jadStream. like fseek, but returns ftell. (0,SEEK_CUR) should work as ftell and work even on fundamentally non-seekable streams.
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
typedef struct jadAllocator
{
	void* opaque;
	jadAllocatorAlloc alloc;
	jadAllocatorFree free;
} jadAllocator;

//a timestamp like the format stored on a disc
typedef struct jadTimestamp
{
	jadBCD2 MIN, SEC, FRAC;
	uint8_t _padding;
} jadTimestamp;

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

//performs necessary static initialization
int jadStaticInit();

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

void jadTimestamp_increment(jadTimestamp* ts);

#ifdef  __cplusplus
}
#endif

#endif //_JAD_H
