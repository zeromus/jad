#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "jad.h"
#include "jadstd.h"
#include "jadtool.h"
#include "../libjadvac/jadvac.h"

#include "ProgressManager.h"

#ifdef _MSC_VER
#define stricmp _stricmp
int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

/*
Issue a series of commands and arguments to be executed in order
v - enable verbose debugging
jad <infile> <outfile> - converts infile to outfile in jad format
jac <infile> <outfile> - converts infile to outfile in jac format
in/out/inout <jad|vac|mirage|mednafen> - demands specified API to be used for IO
test <infile> - tests the infile, which must be a jad or jac, for hash

note: the output API may seem pointless now (since this tool can only convert to jad/jac)
however, in the future, we may at least support writing cue+bin or ccd via libjadvac for comparison purposes
*/

//any of these may not exist, but they wont be referenced unless the correct compile options are issued
int jt_api_libmirage_start(Options *opt, jadCreationParams *jcp);
int jt_api_libmirage_end(Options *opt, jadCreationParams *jcp);
int jt_api_libjadvac_start(Options *opt, jadCreationParams *jcp);
int jt_api_libjadvac_end(Options *opt, jadCreationParams *jcp);

ProgressManager g_ProgressManager;
Options opt;

//setup an allocator for libjad (and libjadvac)
static void* myMalloc(jadAllocator* allocator, size_t amt) { return malloc(amt); }
static void* myRealloc(jadAllocator* allocator, void* ptr, size_t amt) { return realloc(ptr,amt); }
static void myFree(jadAllocator* allocator, void* ptr) { free(ptr); }
static jadAllocator allocator = {
	NULL,
	&myMalloc,
	&myRealloc,
	&myFree
};

void verb(const char* msg, ...)
{
	if(!opt.verbose) return;
  va_list ap;
  va_start(ap, msg);
	char* str;
	vasprintf(&str,msg,ap);
	g_ProgressManager.TryNewline();
	printf("JT:%s\n",str);
}

void bail(const char* msg)
{
	g_ProgressManager.TryNewline();
	printf("JT:BAILING!\nJT:%s\n",msg);
	exit(1);
}

class ArgQueue
{
public:
	ArgQueue(int _argc, char** _argv)
		: argc(_argc)
		, argv(_argv)
		, idx(1) //skip program name
	{
	}

	int argc;
	char ** argv;
	int idx;

	bool is(const char* cmp) const
	{
		if(!curr()) return false;
		return !stricmp(cmp,curr());
	}
	const char* curr() const
	{
		if(idx>argc)
			return NULL;
		return argv[idx];
	}
	const char* next()
	{
		idx++;
		return curr();
	}
};

void command_jadjac(bool isjac)
{
	jadCreationParams jcp;
	jcp.allocator = &allocator;

	if(opt.in == JAD_API_MIRAGE)
	{
		#if JADTOOL_BUILD_API_LIBMIRAGE
			jt_api_libmirage_start(&opt,&jcp);
		#endif
	}
	if (opt.in == JAD_API_VAC)
	{
		#if JADTOOL_BUILD_API_LIBJADVAC
			jt_api_libjadvac_start(&opt,&jcp);
		#endif
	}


	//create the jad context
	jadContext jad;
	if(jadCreate(&jad, &jcp, &allocator) != JAD_OK)
		bail("failed jadCreate");

	//open up the output stream
	jadStream outstream;
	if(jadstd_OpenStdio(&outstream,opt.outfile,"wb") != JAD_OK)
		bail("failed opening output file");

	//start progress and dump the jad
	g_ProgressManager.Start(jcp.numSectors);
	int result = jadDump(&jad,&outstream,isjac?1:0);

	//close resources
	g_ProgressManager.Stop();
	jadstd_CloseStdio(&outstream);
	jadClose(&jad);

	if(result != JAD_OK)
	{
		verb("Cleaning up output file");
		remove(opt.outfile);
	}

	if (opt.in == JAD_API_MIRAGE)
	{
		#if JADTOOL_BUILD_API_LIBMIRAGE
			jt_api_libmirage_end(&opt,&jcp);
		#endif
	}
	if (opt.in == JAD_API_VAC)
	{
		#if JADTOOL_BUILD_API_LIBJADVAC
			jt_api_libjadvac_end(&opt,&jcp);
		#endif
	}
}

static void validate_api()
{
	#if !JADTOOL_BUILD_API_LIBMIRAGE
		if(opt.in == JAD_API_MIRAGE || opt.out == JAD_API_MIRAGE) bail("libmirage disc IO support not compiled in");
	#endif
	#if !JADTOOL_BUILD_API_MEDNAFEN
		if (opt.in == JAD_API_MEDNAFEN || opt.out == JAD_API_MEDNAFEN) bail("mednafen disc IO support not compiled in");
	#endif
	#if !JADTOOL_BUILD_API_LIBJADVAC
		if (opt.in == JAD_API_VAC || opt.out == JAD_API_VAC) bail("libjadvac disc IO support not compiled in");
	#endif
}

int main(int argc, char** argv)
{
	ArgQueue aq(argc,argv);

#define APICHECK() \
	(aq.is("mirage")) ? JAD_API_MIRAGE : ( \
		(aq.is("mednafen")) ? JAD_API_MEDNAFEN : ( \
			(aq.is("vac")) ? JAD_API_VAC : ( \
				(aq.is("jad")) ? JAD_API_JAD : ( \
					JAD_API_NONE) ) ) )

	while(aq.curr())
	{
		if(aq.is("v"))
			opt.verbose = true;
		else if (aq.is("in"))
		{
			aq.next();
			if ((opt.in = APICHECK()) == JAD_API_NONE) bail("unknown argument to `in` command");
		}
		else if (aq.is("out"))
		{
			aq.next();
			if ((opt.out = APICHECK()) == JAD_API_NONE) bail("unknown argument to `in` command");
		}
		else if (aq.is("inout"))
		{
			aq.next();
			if ((opt.in = opt.out = APICHECK()) == JAD_API_NONE) bail("unknown argument to `in` command");
		}
		else if(aq.is("jad") || aq.is("jac"))
		{
			bool isjac = aq.is("jac");
			opt.infile = aq.next();
			opt.outfile = aq.next();

			//auto-select API when non specified
			if(opt.in == JAD_API_NONE) opt.in = JAD_API_VAC;
			if (opt.out == JAD_API_NONE) opt.out = JAD_API_JAD;

			validate_api();

			if(!opt.infile || !opt.outfile)
				bail("jad/jac command missing infile and outfile");
			if(opt.out != JAD_API_JAD)
				bail("jad/jac command's output API must be jad");
			command_jadjac(isjac);
		}
		else
			bail("unknown command");

		aq.next();
	}

	return 0;
}
