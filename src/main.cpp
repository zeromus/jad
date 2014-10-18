//TODO - should this be called jadtool?

//NOTE: this is c++ because I can't stand C

#include <stdio.h>

#include "jad.h"
#include "jadstd.h"

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

	printf("%d\n",jadUtil_CompareFiles("test.JAD","memtest.jad"));

	return 0;
}