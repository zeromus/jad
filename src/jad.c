//IDEA: we might should split the sector into a several streams.. for now lets call it 'header' and 'codec'
//the point being, several opcodes from the header portion will read data from the codec stream, which wont need to be interleaved with the opcode bytes.
//this would allow us to keep the codec running without having to flush and re-initialize it.. 
//BUT.. is there any reason to expect similarity between different sections of data? this would add some complexity to the logic.
//but it may be a good idea for the types of encoders we use

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "jad.h"

#include "jadcodec_heatshrink.h"

#define SYNC_SIZE 12

static const uint8_t kSync[] = {0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00};
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

	//TODO - better header reading and validation
	//TODO - make an alternate codepath for safely blittable little endian systems to read in one piece
	
	//read what of the header we need
	if(!_jadSeek(stream,OFS_FLAGS)) return JAD_ERROR;
	if(!_jadRead32(stream,&jad->flags)) return JAD_ERROR;
	if(!_jadRead32(stream,&jad->numSectors)) return JAD_ERROR;
	if(!_jadRead32(stream,&jad->numTocEntries)) return JAD_ERROR;

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

int _jadCheckSync(struct jadSector* sector)
{
	return !memcmp(sector->data,kSync,SYNC_SIZE);
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
			memcpy(sector->data,kSync,SYNC_SIZE);
			break;

		default:
			//????
			return JAD_ERROR;
		}

	} //opcode processing loop
}

int _jadEncodeSector(uint32_t jadEncodeSettings, struct jadSector* sector, struct jadStream* stream)
{
	//early exit for testing: if we dont want to do anything, for reference, just copy it directly
	if(jadEncodeSettings & jadEncodeSettings_DirectCopy)
	{
		_jadWrite8(stream,JACOP_COPY_FULL_2352);
		_jadWriteBytes(stream,sector->entire,2352);
		_jadWrite8(stream,JACOP_COPY_SUBCODE_MASK);
		_jadWrite8(stream,0xFF);
		_jadWriteBytes(stream,sector->subcode,96);
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
		_jadWriteBytes(stream,sector->entire,12);
	}

	//test 1. copy the main data sector (TODO - detect different ECM modes, pre-code, compress, etc.)
	//_jadWrite8(stream,JACOP_COPY_DATA_2340);
	//_jadWriteBytes(stream,sector->entire+12,2340);

	//test 2. encode it
	{
		jadHeatshrinkEncoder encoder;
		jadcodec_OpenHeatshrinkEncoder(&encoder);
		_jadWrite8(stream,JACOP_COMPRESS_DATA_2340);
		_jadStreamEncode(sector->entire+12,2430,&encoder.stream,stream);
	}

	//handle the subcode
	_jadWrite8(stream,JACOP_COPY_SUBCODE_MASK);
	_jadWrite8(stream,0xFF);
	_jadWriteBytes(stream,sector->subcode,96);


	_jadWrite8(stream,JACOP_END);

	return JAD_OK;
}

int _jadWrite(struct jadContext* jad, struct jadStream* stream, int JACIT)
{
	const uint32_t encodeSettings = jadEncodeSettings_None;
	const uint32_t flags = JACIT?jadFlags_JAC:jadFlags_None;
	const int isJaced = jad->flags & jadFlags_JAC;
	uint32_t i,s;
	uint32_t ofsIndex = OFS_TOC + jad->numTocEntries*12;
	uint32_t ofsSectorsPastIndex = ofsIndex + jad->numSectors*4; //(each sector index is 4 bytes right now.. not sufficient for dvds but working ok for cds for now)

	struct jadStream* ins = jad->stream;
	struct jadStream* outs = stream;

	_jadWriteMagic(outs);
	_jadWrite32(outs,1); //version
	_jadWrite32(outs,flags); //flags
	_jadWrite32(outs,jad->numSectors);
	_jadWrite32(outs,jad->numTocEntries);
	_jadWrite32(outs,0); //reserved for CRC
	_jadWrite32(outs,0); //reserved for ??
	
	//copy the TOC to output
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
		struct jadSector sector;
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
			if(ins->read(&sector,sizeof(sector),ins) != sizeof(sector))
				return JAD_ERROR;
		}
		
		//compress or dump the sector, as appropriate
		if(JACIT)
		{
			ret = _jadEncodeSector(encodeSettings, &sector, outs);
		}
		else
		{
			if(outs->write(&sector,sizeof(sector),outs) != sizeof(sector))
				return JAD_ERROR;
		}


	}

	return JAD_OK;
}

int jadWriteJAC(struct jadContext* jad, struct jadStream* stream)
{
	return _jadWrite(jad,stream,1);
}

int jadWriteJAD(struct jadContext* jad, struct jadStream* stream)
{
	return _jadWrite(jad,stream,0);
}

//this does a poor job right now. a better job would generate correct Q-subchannel data, and setup the TOC, among possibly other things...
void jadUtil_Convert2352ToJad(const char* inpath, const char* outpath)
{
	const uint32_t kVersion = 1;
	const uint32_t kFlags = 0;
	const uint32_t kReserved = 0;
	const uint32_t kNumtocs = 0;
	const uint8_t kSubcode[96] = {0};

	uint32_t nSectors = 0;
	
	FILE* inf = fopen(inpath,"rb");
	FILE* outf = fopen(outpath,"wb");

	//write the format
	fwrite(kMagic,1,8,outf); //magic
	fwrite(&kVersion,1,4,outf); //version
	fwrite(&kFlags,1,4,outf); //flags
	fwrite(&kReserved,1,4,outf); //numsectors later
	fwrite(&kNumtocs,1,4,outf); //numtocs
	fwrite(&kReserved,1,4,outf); //reserved for CRC
	fwrite(&kReserved,1,4,outf); //reserved

	//write the TOC
	//(nothing to do)

	//begin copying sectors
	while(!feof(inf))
	{
		uint8_t buf[2352];
		fread(buf,1,2352,inf);
		fwrite(buf,1,2352,outf);
		fwrite(kSubcode,1,96,outf);
		nSectors++;
	}

	//rewrite the numsectors
	fseek(outf,16,SEEK_SET);
	fwrite(&nSectors,1,4,outf);

	fclose(inf);
	fclose(outf);
}

int jadUtil_CompareFiles(const char* inpath, const char* inpath2)
{
	FILE* infa = fopen(inpath,"rb");
	FILE* infb = fopen(inpath2,"rb");

	char bufa[4096], bufb[4096];
	for(;;)
	{
		int a = fread(bufa,1,4096,infa);
		int b = fread(bufb,1,4096,infb);
		if(a != b) return -1;
		if(a==-1) return 0;
		if(memcmp(bufa,bufb,a))
			return -1;
		if(a != 4096) return 0;
	}
	return 0;
}