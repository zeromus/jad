#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "jad.h"
#include "jadstd.h"

#include "ProgressManager.h"

//I tried to separate the libmirage stuff but I don't have time now
//the plan is to make it optional eventually (it is a MIGHTY dependency)
#include <mirage.h>
#include "api_libmirage.h"

#ifdef _MSC_VER
#define stricmp _stricmp
int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

/*
Issue a series of commands and arguments
v - enable verbose debugging
jad <infile> <outfile> - converts infile to outfile in jad format
jac <infile> <outfile> - converts infile to outfile in jac format
mirage <in|out|inout> - disregards jadtool choice and requires use of libmirage
test <infile> - tests the infile, which must be a jad or jac, for hash
*/

struct Options {
	bool verbose;
	bool mirage_in, mirage_out;
	const char *infile, *outfile;
} opt;



static void verb(const char* msg, ...)
{
	if(!opt.verbose) return;
  va_list ap;
  va_start(ap, msg); 
	char* str;
	vasprintf(&str,msg,ap);
	g_ProgressManager.TryNewline();
	printf("JT:%s\n",str);
}

static void bail(const char* msg)
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

struct CreateContext
{
	CreateContext() 
		: disc(NULL)
		, sector(NULL)
	{
	}
	MirageDisc* disc;
	MirageSector* sector;
};

//delivers sectors to the library when theyre requested for encoding.
//management of the sector buffers is tricky.
static int myJadCreateCallback(void* opaque, int sectorNumber, void** sectorBuffer, void **subcodeBuffer)
{
	CreateContext* ctx = (CreateContext*)opaque;

	g_ProgressManager.Tick();

	//close the old sector, since the buffer is done being used
	if(ctx->sector != NULL)
		g_object_unref(ctx->sector);

	//open sector
	GError *error = NULL;
	ctx->sector = mirage_disc_get_sector(ctx->disc,sectorNumber,&error);
	if(gerrprint(error)) return JAD_ERROR;

	//extract data
	const guint8 *secbuf, *subbuf;
	mirage_sector_extract_data(ctx->sector,&secbuf,2352,MIRAGE_SUBCHANNEL_PW,&subbuf,96,&error);
	if(gerrprint(error)) return JAD_ERROR;

	static uint8_t s_subbuf[96]; //this is needed because libmirage sends us interleaved subchannel

	//but for now, dont bother
	memcpy(s_subbuf,subbuf,96);

	*sectorBuffer = (void*)secbuf;
	*subcodeBuffer = (void*)s_subbuf;

	return JAD_OK;
}

void command_jadjac(bool isjac)
{
	//for now, thats just how it is
	opt.mirage_in = true;
	Libmirage_Init();
	Libmirage_SyncVerbose(opt.verbose);

	GError *error = NULL;

	MirageContext* context = (MirageContext*)g_object_new(MIRAGE_TYPE_CONTEXT,NULL);
	if(opt.verbose)
		mirage_context_set_debug_mask(context,MIRAGE_DEBUG_PARSER);

	const char* fnames[] = { opt.infile, NULL };
	MirageDisc* disc = mirage_context_load_image(context,(gchar**)fnames,&error);
	gbailif(error);
	error = NULL;

	gint sessions = mirage_disc_get_number_of_sessions(disc);
	if(sessions != 1)
		bail("Only discs with 1 session are supported");

	gint length = mirage_disc_layout_get_length(disc);
	verb("Disc length: %d sectors",length);

	//well... i dont know. i guess we'll skip the track 1 pregap, or else we read too many sectors next
	length -= 150;

	//libjad will fire callbacks, so this will track our process state
	CreateContext cc;
	cc.disc = disc;

	//instructions to libjad for how to create the jad
	jadCreationParams jcp;
	jcp.opaque = &cc;
	jcp.numTocEntries = 0;
	jcp.tocEntries = NULL;
	jcp.numSectors = length;
	jcp.callback = myJadCreateCallback;

	//create the jad context
	jadContext jad;
	if(jadCreate(&jad, &jcp, NULL) != JAD_OK)
		bail("failed jadCreate");

	//open up the output stream
	jadStream outstream;
	if(jadstd_OpenStdio(&outstream,opt.outfile,"wb") != JAD_OK)
		bail("failed opening output file");

	//start progress and dump the jad
	g_ProgressManager.Start(length);
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

	//make sure the last mirage sector is shut down
	if(cc.sector != NULL)
		g_object_unref(cc.sector);

	g_object_unref(disc);
}

int main(int argc, char** argv)
{
	ArgQueue aq(argc,argv);

	while(aq.curr())
	{
		if(aq.is("v"))
			opt.verbose = true;
		else if(aq.is("mirage"))
		{
			aq.next();
			if(aq.is("in"))
				opt.mirage_in = true;
			else if(aq.is("out"))
				opt.mirage_out = true;
			else if(aq.is("inout"))
				opt.mirage_in = opt.mirage_out = true;
			else bail("unknown argument to `mirage` command");
			if(opt.mirage_in || opt.mirage_out)
				Libmirage_Init();
		}
		else if(aq.is("jad") || aq.is("jac"))
		{
			bool isjac = aq.is("jac");
			opt.infile = aq.next();
			opt.outfile = aq.next();
			if(!opt.infile || !opt.outfile)
				bail("jad/jac command missing infile and outfile");
			command_jadjac(isjac);
		}
		else 
			bail("unknown command");

		aq.next();
	}

	return 0;
}