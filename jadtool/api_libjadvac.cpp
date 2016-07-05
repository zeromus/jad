#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mirage.h>
#include <monolithic.h>

#include "jadtool.h"
#include "../libjad/jad.h"
#include "../libjad/jadstd.h"
#include "../libjadvac/jadvac.h"

//delivers sectors to the library when theyre requested for encoding.
//management of the sector buffers is tricky.
static int _jadCreateCallback(void* opaque, int sectorNumber, void** sectorBuffer, void **subcodeBuffer)
{
}

int jt_api_libjadvac_start(Options *opt, jadCreationParams *jcp)
{
	const char* dot = strrchr(opt->infile,'.');
	if(!dot) bail("Can't determine filetype from extension");
	if(tolower(dot[1]) == 'c' && tolower(dot[2]) == 'u' && tolower(dot[3]) == 'e' && !dot[4])
	{
		//handle as cue
		jadContext ctx;
		jadStream instream;
		if (jadstd_OpenStdio(&instream, opt->infile, "rb") != JAD_OK)
			bail("failed opening input cuefile");
		jadvacOpenFile_cue(&ctx,&instream, NULL);
	}
	////libjad will fire callbacks, so this will track our process state
	//CreateContext *cc = new CreateContext();
	//cc->disc = disc;

	////instructions to libjad for how to create the jad
	//jcp->opaque = cc;
	//jcp->numTocEntries = 0;
	//jcp->tocEntries = NULL;
	//jcp->numSectors = length;
	//jcp->callback = _jadCreateCallback;

	return JAD_OK;
}

int jt_api_libjadvac_end(Options *opt, jadCreationParams *jcp)
{
	//CreateContext *cc = (CreateContext *)jcp->opaque;
	//delete cc;
	return JAD_OK;
}
