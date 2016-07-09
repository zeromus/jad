#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <string>

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

struct FilesystemContext
{
	std::string basedir;
};

int myOpen(jadvacFilesystem* fs, jadStream* stream, const char* path)
{
	int ret;
	//TODO: more sophistication, for sure

	//first, try opening it directly as an absolute path
	ret = jadstd_OpenStdio(stream,path,"rb");
	if(ret == JAD_OK) return JAD_OK;

	//try doing it relative to the given filesystem
	FilesystemContext* ctx = (FilesystemContext*)fs->opaque;
	std::string trial = ctx->basedir + path;

	ret = jadstd_OpenStdio(stream,trial.c_str(),"rb");
	if(ret == JAD_OK) return JAD_OK;

	return JAD_ERROR;
}
int myClose(jadvacFilesystem* fs, jadStream* stream)
{
	return jadstd_CloseStdio(stream);
}

int jt_api_libjadvac_start(Options *opt, jadCreationParams *jcp)
{
	FilesystemContext fscontext;
	fscontext.basedir = opt->infile;

	//this is not 100% reliable.. not an easy problem.. that's why it isn't built into libjadvac
	//but uhhh maybe we want it to be an optional service, because.. it's not an easy problem.
	const char* dirspot = strrchr(opt->infile,'/');
	if(!dirspot) dirspot = strrchr(opt->infile,'\\');
	if(dirspot)
	{
		fscontext.basedir.resize(dirspot-opt->infile+1); //leaves the path seperator
	}
	else
	{
		fscontext.basedir = "";
	}

	jadvacFilesystem fs = {
		&fscontext,
		&myOpen,
		&myClose
	};

	//TODO- put utility in libjadvac to determine type from extension? or a utility to automatically select the right loader?
	const char* dot = strrchr(opt->infile,'.');
	if(!dot) bail("Can't determine filetype from extension");
	if(tolower(dot[1]) == 'c' && tolower(dot[2]) == 'u' && tolower(dot[3]) == 'e' && !dot[4])
	{
		//handle as cue
		jadContext jad;
		jadStream instream;
		jadvacContext ctx;
		ctx.allocator = jcp->allocator;
		ctx.stream = &instream;
		ctx.jad = &jad;
		ctx.fs = &fs;
		if (jadstd_OpenStdio(&instream, opt->infile, "rb") != JAD_OK)
			bail("failed opening input cuefile");
		jadvacOpenFile_cue(&ctx);
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
