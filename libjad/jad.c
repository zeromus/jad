//IDEA: we might should split the sector into a several streams.. for now lets call it 'header' and 'codec'
//the point being, several opcodes from the header portion will read data from the codec stream, which wont need to be interleaved with the opcode bytes.
//this would allow us to keep the codec running without having to flush and re-initialize it..
//BUT.. is there any reason to expect similarity between different sections of data? this would add some complexity to the logic.
//but it may be a good idea for the types of encoders we use

#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "jad.h"
#include "jadq.h"

#include "jadcodec_heatshrink.h"

static const uint8_t kSync[12] = {0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00};
static const char* kMagic = "JADJAC!\0";

#define OFS_FLAGS 0x0C
#define OFS_NUMSECTORS 0x10
#define OFS_TOC 0x20

#define JACOP_END 0x00

#define JACOP_COPY_FULL_2352 0x11 //copies 2352 bytes verbatim
#define JACOP_COPY_SYNC_12 0x12 //copies 12 bytes of sync
#define JACOP_COPY_DATA_2340 0x13 //copies 2340 bytes of data after the sync
#define JACOP_COPY_SUBCODE_MASK 0x14 //copies subcode channels to the appropriate point (next op is a byte bitfield for subchannel enabling)

#define JACOP_SYNTHESIZE_SYNC 0x20 //synthesizes a normal sync string at sec[0] for 12 bytes
#define JACOP_SYNTHESIZE_ECM_MODE1 0x21 //synthesizes EDC at sec[0x810]=EDC(0,0x810) and ECC(false)
#define JACOP_SYNTHESIZE_ECM_MODE2_1 0x22 //synthesizes EDC at sec[0x818]=EDC(16,0x808) and ECC(true)
#define JACOP_SYNTHESIZE_ECM_MODE2_2 0x23 //synthesizes EDC at sec[0x92C]=EDC(16,0x91C)

#define JACOP_DECOMPRESS_FLAG 0x40 //indicates that copies should come from the codec bitstream instead (maybe, just an idea)

#define JACOP_COMPRESS_DATA_2340 (JACOP_COPY_DATA_2340 | JACOP_DECOMPRESS_FLAG)

int jadStaticInit()
{
	void jadq_InitCRC();
	jadq_InitCRC();

	return JAD_OK;
}

//test
void _jadWriteMagic(struct jadStream* stream)
{
	stream->write(kMagic,8,stream);
}

void _jadWrite8(struct jadStream* stream, uint8_t v)
{
	stream->write(&v,1,stream);
}

void _jadWriteBytes(struct jadStream* stream, void* buf, size_t nBytes)
{
	stream->write(buf,nBytes,stream);
}


void _jadWrite32(struct jadStream* stream, uint32_t v)
{
	stream->write(&v,4,stream);
}

int _jadRead8(struct jadStream* stream, uint8_t* v)
{
	return stream->read(v,1,stream)==1;
}

int _jadRead32(struct jadStream* stream, uint32_t* v)
{
	return stream->read(v,4,stream)==4;
}

int _jadSeek(struct jadStream* stream, long offset)
{
	return stream->seek(stream,offset,SEEK_SET) == offset;
}

long _jadTell(struct jadStream* stream)
{
	return stream->seek(stream,0,SEEK_CUR);
}

int jadOpen(struct jadContext* jad, struct jadStream* stream, struct jadAllocator* allocator)
{
	if(!jad || !stream) return JAD_ERROR;

	jad->allocator = allocator;
	jad->stream = stream;
	jad->createParams = NULL;

	//TODO - better header reading and validation
	//TODO - make an alternate codepath for safely blittable little endian systems to read in one piece

	//read what of the header we need
	if(!_jadSeek(stream,OFS_FLAGS)) return JAD_ERROR;
	if(!_jadRead32(stream,&jad->flags)) return JAD_ERROR;
	if(!_jadRead32(stream,&jad->numSectors)) return JAD_ERROR;
	if(!_jadRead32(stream,&jad->numTocEntries)) return JAD_ERROR;

	return JAD_OK;
}

int jadCreate(jadContext* jad, jadCreationParams* jcp, jadAllocator* allocator)
{
	if(!jad || !jcp) return JAD_ERROR;

	jad->allocator = allocator;
	jad->stream = NULL;
	jad->createParams = jcp;

	jad->flags = 0;
	jad->numSectors = jcp->numSectors;
	jad->numTocEntries = jcp->numTocEntries;

	return JAD_OK;
}

int jadClose(struct jadContext* jad)
{
	//as needed
	return JAD_OK;
}

void jadTOCEntry_Clear(jadTOCEntry* tocentry)
{
	memset(tocentry,0,sizeof(*tocentry));
}

//run data from @outstream through @codec, until @amount bytes have been placed in @buf
int _jadStreamDecode(jadStream* outstream, jadStream* codec, void* buf, size_t amount)
{
	//lets do this first a byte at a time. once thats proven, we'll try it block by block as the codec can take it

	uint8_t* ptr = (uint8_t*)buf;
	while(amount>0)
	{
		int ret = outstream->get(outstream);
		if(ret<0) return ret;
		ret = codec->put(codec,(uint8_t)ret);
		if(ret<0) return ret;

		for(;;)
		{
			int c = codec->get(codec);
			if(c==JAD_EOF) break;
			if(c<0) return JAD_ERROR;
			if(amount==0) return JAD_ERROR;
			amount--;
			*ptr++ = (uint8_t)c;
		}

		if(amount==0) break;
	}

	return JAD_OK;
}

//run @amount bytes from @buf through @codec and into @outstream
int _jadStreamEncode(void* buf, size_t amount, jadStream* codec, jadStream* outstream)
{
	//lets do this first a byte at a time. once thats proven, we'll try it block by block as the codec can take it

	uint8_t* ptr = (uint8_t*)buf;
	while(amount>0)
	{
		int ret = codec->put(codec, *ptr);
		if(ret != JAD_OK) return ret;

		ptr++;
		amount--;

		//if we just finished, flush the operation
		//TODO - we may not want to do this here, actually, in order to accumulate multiple pieces of data through one encoder. I'm not sure that's a good idea though.
		if(amount==0)
			codec->flush(codec);

		//read from the codec until it's empty
		for(;;)
		{
			int c = codec->get(codec);
			if(c==JAD_EOF) break;
			else if(c<0) return c;
			outstream->put(outstream,(uint8_t)c);
		}
	}

	return JAD_OK;
}


//2048 bytes packed into 2352:
//12 bytes sync(00 ff ff ff ff ff ff ff ff ff ff 00)
//3 bytes sector address (min+A0),sec,frac //does this correspond to ccd `point` field in the TOC entries?
//sector mode byte (0: silence; 1: 2048Byte mode (EDC,ECC,CIRC), 2: mode2 (could be 2336[vanilla mode2], 2048[xa mode2 form1], 2324[xa mode2 form2])
//cue sheets may use mode1_2048 (and the error coding needs to be regenerated to get accurate raw data) or mode1_2352 (the entire sector is present)
//audio is a different mode, seems to be just 2352 bytes with no sync, header or error correction. i guess the CIRC error correction is still there

int _jadCheckSync(uint8_t* sector)
{
	return !memcmp(sector,kSync,12);
}

int _jadDecodeSector(struct jadSector* sector, struct jadStream* stream)
{
	//OLD IDEA: sector is assumed to start at 0 (this rule may be waived if it speeds decompression. it simplifies analysis of encoding, however)
	//EDIT: we dont really like this idea
	//NEWER IDEA - assume sector starts at 0 with a valid sync field, at least

	for(;;)
	{
		uint8_t op, arg;
		int i;

		//read an opcode
		_jadRead8(stream,&op);

		//process opcode
		switch(op)
		{
		case JACOP_END:
			return JAD_OK;

		case JACOP_COPY_FULL_2352:
			stream->read(sector->entire+12,2352,stream);
			break;
		case JACOP_COPY_SYNC_12:
			stream->read(sector->entire,12,stream);
			break;
		case JACOP_COPY_DATA_2340:
			stream->read(sector->data+12,2340,stream);
			break;
		case JACOP_COPY_SUBCODE_MASK:
			_jadRead8(stream,&arg);
			for(i=0;i<8;i++)
			{
				if(arg&1)
					stream->read(sector->subcode+i*12,12,stream);
				arg>>=1;
			}
			break;

		case JACOP_COMPRESS_DATA_2340:
			{
				jadHeatshrinkDecoder decoder;
				jadcodec_OpenHeatshrinkDecoder(&decoder);
				_jadStreamDecode(stream,&decoder.stream,sector->entire+12,2430);
				break;
			}
		case JACOP_SYNTHESIZE_SYNC:
			memcpy(sector->data,kSync,12);
			break;

		default:
			//????
			return JAD_ERROR;
		}

	} //opcode processing loop
}

int _jadEncodeSector(uint32_t jadEncodeSettings, uint8_t* sector, uint8_t* subcode, struct jadStream* stream)
{
	//early exit for testing: if we dont want to do anything, for reference, just copy it directly
	if(jadEncodeSettings & jadEncodeSettings_DirectCopy)
	{
		_jadWrite8(stream,JACOP_COPY_FULL_2352);
		_jadWriteBytes(stream,sector,2352);
		_jadWrite8(stream,JACOP_COPY_SUBCODE_MASK);
		_jadWrite8(stream,0xFF);
		_jadWriteBytes(stream,subcode,96);
		_jadWrite8(stream,JACOP_END);
		return JAD_OK;
	}

	//check how to handle the sync sector: is it intact, or do we need to copy it
	if(_jadCheckSync(sector))
	{
		_jadWrite8(stream,JACOP_SYNTHESIZE_SYNC);
	}
	else
	{
		_jadWrite8(stream,JACOP_COPY_SYNC_12);
		_jadWriteBytes(stream,sector,12);
	}

	//test 1. copy the main data sector (TODO - detect different ECM modes, pre-code, compress, etc.)
	//_jadWrite8(stream,JACOP_COPY_DATA_2340);
	//_jadWriteBytes(stream,sector->entire+12,2340);

	//test 2. encode it
	{
		jadHeatshrinkEncoder encoder;
		jadcodec_OpenHeatshrinkEncoder(&encoder);
		_jadWrite8(stream,JACOP_COMPRESS_DATA_2340);
		_jadStreamEncode(sector+12,2430,&encoder.stream,stream);
	}

	//handle the subcode
	_jadWrite8(stream,JACOP_COPY_SUBCODE_MASK);
	_jadWrite8(stream,0xFF);
	_jadWriteBytes(stream,subcode,96);


	_jadWrite8(stream,JACOP_END);

	return JAD_OK;
}

//this is a private implementtion in case we need to wrap it somehow
static int _jadDump(struct jadContext* jad, struct jadStream* stream, int JACIT)
{
	//part of the magic of this function is that it can read from a jadOpen'd file or a jadCreate'd file
	//therefore it is useful both for conversion and creation

	const uint32_t encodeSettings = jadEncodeSettings_None;
	const uint32_t flags = JACIT?jadFlags_JAC:jadFlags_None;
	const int isJaced = jad->flags & jadFlags_JAC;
	uint32_t i,s;
	uint32_t ofsIndex = OFS_TOC + jad->numTocEntries*12;
	uint32_t ofsSectorsPastIndex = ofsIndex + jad->numSectors*4; //(each sector index is 4 bytes right now.. not sufficient for dvds but working ok for cds for now)

	struct jadStream* ins = jad->stream;
	struct jadCreationParams* incp = jad->createParams;
	struct jadStream* outs = stream;

	_jadWriteMagic(outs); //8 bytes
	_jadWrite32(outs,1); //version
	_jadWrite32(outs,flags); //flags
	_jadWrite32(outs,jad->numSectors);
	_jadWrite32(outs,jad->numTocEntries);
	_jadWrite32(outs,0); //reserved for CRC
	_jadWrite32(outs,0); //reserved for ??

	//copy the TOC to output
	if(incp)
	{
		for(i=0;i<jad->numTocEntries;i++)
		{
			outs->write(&incp->tocEntries[i],sizeof(jadTOCEntry),outs);
		}
	}
	else
	{
		jadTOCEntry tocEntry;
		_jadSeek(ins,OFS_TOC);
		for(i=0;i<jad->numTocEntries;i++)
		{
			ins->read(&tocEntry,sizeof(tocEntry),ins);
			outs->write(&tocEntry,sizeof(tocEntry),outs);
		}
	}

	//skip the output cursor past the index. we'll start writing encoded sectors there
	if(JACIT)
		_jadSeek(outs, ofsSectorsPastIndex);

	//for each sector, encode and write the index
	for(s=0;s<(int)jad->numSectors;s++)
	{
		//a temporary sector, in case we need it
		struct jadSector sector;

		//the pointers we'll use for each of these buffers, but we may alter them if the temp sector isnt used
		uint8_t *dataptr = sector.data;
		uint8_t *subcodeptr = sector.subcode;

		int ret;

		//maintain the index, if we're writing a JAC
		if(JACIT)
		{
			//save the current outs position for writing sectors at
			long pos = _jadTell(outs);

			//write the index entry for this sector
			_jadSeek(outs,ofsIndex+s*4);
			_jadWrite32(outs,(uint32_t)pos);

			//restore the outs position
			_jadSeek(outs,pos);
		}

		//decompress or read the sector directly, as appropriate
		if(isJaced)
		{
			uint32_t sector_ofs;

			//seek the sector index record
			if(!_jadSeek(ins,ofsIndex + s*4))
				return JAD_ERROR;
			if(!_jadRead32(ins,&sector_ofs))
				return JAD_ERROR;
			if(!_jadSeek(ins,sector_ofs))
				return JAD_ERROR;
			if(_jadDecodeSector(&sector, ins))
				return JAD_ERROR;
		}
		else
		{
			if(incp)
			{
				int ret = incp->callback(incp->opaque,s,(void**)&dataptr,(void**)subcodeptr);
				if(ret != JAD_OK)
					return JAD_ERROR;
			}
			else
			{
				if(ins->read(&sector,sizeof(sector),ins) != sizeof(sector))
					return JAD_ERROR;
			}
		}

		//compress or dump the sector, as appropriate
		if(JACIT)
		{
			ret = _jadEncodeSector(encodeSettings, dataptr, subcodeptr, outs);
		}
		else
		{
			if(outs->write(dataptr,2352,outs) != 2352) return JAD_ERROR;
			if(outs->write(subcodeptr,96,outs) != 96) return JAD_ERROR;
		}

	}

	return JAD_OK;
}

int jadDump(struct jadContext* jad, struct jadStream* stream, int JACIT)
{
	return _jadDump(jad,stream,JACIT);
}

void jadTimestamp_increment(jadTimestamp* ts)
{
	ts->FRAC++;
	if(ts->FRAC == 75)
	{
		ts->FRAC = 0;
		ts->SEC++;
	}
	if(ts->SEC==60)
	{
		ts->SEC = 0;
		ts->MIN++;
	}
}

#ifdef _MSC_VER
//http://opensource.apple.com/source/OpenSSH/OpenSSH-7.1/openssh/bsd-strlcat.c
//TODO: rewrite for new license
size_t jadutil_strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (*d != '\0' && n-- != 0)
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
#else
size_t jadutil_strlcpy(char *dst, const char *src, size_t size)
{
	return strlcpy(dst,src,size);
}
size_t jadutil_strlcat(char *dst, const char *src, size_t size)
{
	return strlcat(dst,src,size);
}
#endif

//this function works a little differently from the normal strrchr. in case the length is known, it can save work
const char *jadutil_strrchr(const char* src, size_t len, char c)
{
	const char* cp = src+len;
	while(cp>=src)
	{
		if(*cp == c)
			return cp;
		cp--;
	}
	return NULL;
}
