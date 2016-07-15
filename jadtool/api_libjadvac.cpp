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
	Options *opt;
	jadCreationParams *jcp;
	size_t strlen_infile;
};

//determines the base directory of the given path
static void myBasedir(char *path)
{
	//last = find_last_slash(path);
	char *slash = strrchr(path, '/');

	//unix can have backslashes so only check on windows
	#ifdef _WIN32
		if (!slash) slash = strrchr(path, '\\');
	#endif

	//libretro-common had more complex logic here for windows dealing with backslashes, I don't see why

	if (slash)
	{
		//N.B. in case the slash was at the end of the path (and at the end of the buffer) there would certainly be room for a null terminator after it
		slash[1] = 0;
	}
	else {
		//libretro-common did this, maybe it's helpful for some reason
		//maybe for maintaining the ends-with-/ invariant?
		//the alternative is returning an empty path
		strcpy(path, "./");
	}
}

static int myOpen(jadvacFilesystem* fs, jadStream* stream, const char* path)
{
	static const size_t kPathBufLen = 1024;
	char tmp[kPathBufLen];
	int ret;

	FilesystemContext* ctx = (FilesystemContext*)fs->opaque;

	//first, try opening it directly as an absolute path (so we dont have to worry about the path resolving handling absolute paths)
	ret = jadstd_OpenStdio(stream,path,"rb");
	if(ret == JAD_OK) return JAD_OK;

	//before doing any work with buffers, make sure we have room
	if(ctx->strlen_infile >= kPathBufLen-1)
		return JAD_ERROR;

	//copy the infile, reduce it to a base directory
	memcpy(tmp,ctx->opt->infile,ctx->strlen_infile);
	tmp[ctx->strlen_infile] = 0;
	myBasedir(tmp);

	//concatenate the given file, if we can
	jadutil_strlcat(tmp,path,kPathBufLen);

	//now try opening it
	ret = jadstd_OpenStdio(stream,tmp,"rb");

	if(ret) return ret;

	return JAD_OK;
}

static int myClose(jadvacFilesystem* fs, jadStream* stream)
{
	return jadstd_CloseStdio(stream);
}

int jt_api_libjadvac_start(Options *opt, jadCreationParams *jcp)
{
	FilesystemContext fscontext;
	fscontext.jcp = jcp;
	fscontext.opt = opt;
	fscontext.strlen_infile = strlen(opt->infile);

	jadvacFilesystem fs = {
		&fscontext,
		&myOpen,
		&myClose
	};

	//TODO- put utility in libjadvac to determine type from extension? or a utility to automatically select the right loader?
	const char* dot = jadutil_strrchr(opt->infile,fscontext.strlen_infile,'.');
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
