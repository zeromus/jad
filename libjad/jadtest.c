#include "jadtest.h"
#include <string.h>

int jadTest_CompareFiles(const char* inpath, const char* inpath2)
{
	FILE* infa = fopen(inpath,"rb");
	FILE* infb = fopen(inpath2,"rb");

	int ret = 0;
	char bufa[4096], bufb[4096];
	for(;;)
	{
		int a = fread(bufa,1,4096,infa);
		int b = fread(bufb,1,4096,infb);
		if(a != b) { ret = -1; goto RETURN; }
		if(a==-1) goto RETURN;
		if(memcmp(bufa,bufb,a))
		{ ret = -1; goto RETURN; }
		if(a != 4096) goto RETURN;
	}
RETURN:
	fclose(infa);
	fclose(infb);
	return ret;
}