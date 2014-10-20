//TODO - should this be called jadtool?

//NOTE: this is c++ because I can't stand C

#include <stdio.h>

#include <vector>

#include "jad.h"
#include "jadstd.h"
#include "jadcodec_heatshrink.h"

int main(int argc, char **argv)
{
	jadUtil_Convert2352ToJad("memtest.2352","memtest.JAD");
	
	jadContext jad;
	jadStream instream;
	jadStream outstream;


	//open the starting point jad and write it out as a jac
	printf("a\n");
	jadstd_OpenStdio(&instream,"memtest.JAD","rb");
	jadstd_OpenStdio(&outstream,"test.JAC","wb");
	jadOpen(&jad,&instream,NULL);
	jadWriteJAC(&jad,&outstream);
	jadClose(&jad);
	jadstd_CloseStdio(&instream);
	jadstd_CloseStdio(&outstream);

	//open the jac we just wrote and write it out as a jad
	printf("b\n");
	jadstd_OpenStdio(&instream,"test.JAC","rb");
	jadstd_OpenStdio(&outstream,"test.JAD","wb");
	jadOpen(&jad,&instream,NULL);
	jadWriteJAD(&jad,&outstream);
	jadClose(&jad);
	jadstd_CloseStdio(&instream);
	jadstd_CloseStdio(&outstream);

	printf("cmp: %d\n",jadUtil_CompareFiles("test.JAD","memtest.jad"));

	std::vector<uint8_t> encodeTest;
	for(int i=0;i<5;i++) 
		encodeTest.push_back(i);
	for(int i=0;i<5;i++) 
		encodeTest.push_back(i);

	jadHeatshrinkEncoder encoder;
	jadcodec_OpenHeatshrinkEncoder(&encoder);
	jadHeatshrinkDecoder decoder;
	jadcodec_OpenHeatshrinkDecoder(&decoder);

	printf("encoded: ");
	std::vector<uint8_t> decodeTest;
	for(int i=0;i<(int)encodeTest.size();i++)
	{
		int c = encodeTest[i];
		encoder.stream.put(&encoder.stream,(uint8_t)c);
		if(i==encodeTest.size()-1)
			encoder.stream.flush(&encoder.stream);

		for(;;)
		{
			c = encoder.stream.get(&encoder.stream);
			bool done = (c==JAD_EOF) ;
			if(done)
			{
				decoder.stream.flush(&decoder.stream);
			}
			else 
			{
				int did = decoder.stream.put(&decoder.stream,(uint8_t)c);
				printf("%02X ",c);
			}
			for(;;)
			{
				c = decoder.stream.get(&decoder.stream);
				if(c == JAD_EOF) break;
				decodeTest.push_back((uint8_t)c);
			}
			if(done) break;
		}
	}

	printf("\n");
	printf("decoded: ");
	for(int i=0;i<(int)decodeTest.size();i++)
	{
		printf("%02X ",decodeTest[i]);
	}
	printf("\n");

	int zzz=9;

	return 0;
}