#include <mirage.h>
#include <monolithic.h>
#include <stdlib.h>

#include "jad.h"
#include "jadstd.h"

/*
Issue a series of commands and arguments
v - enable verbose debugging
jad <infile> <outfile> - converts infile to outfile in jad format
jac <infile> <outfile> - converts infile to outfile in jac format
mirage <in|out|inout> - disregards jadtool choice and requires use of libmirage
test <infile> - tests the infile, which must be a jad or jac, for hash
*/

#ifdef _MSC_VER
#include <wchar.h>
#define stricmp _stricmp
extern "C" void g_clock_win32_init();
extern "C" void g_thread_win32_init();
extern "C" void glib_init();
static void msc_libmirage_init()
{
	//initialization that glib dlls would normally do, approximately
	g_clock_win32_init();
	g_thread_win32_init();
	glib_init();
	g_type_init();

	//special for us because we use monolithic libmirage
	mirage_preinitialize_monolithic();
}
static void msc_init()
{
	//work around some kind of linking error from libintl
	volatile void* blech = &wmemcpy;
}
#endif


struct Options {
	bool verbose;
	bool mirage_in, mirage_out;
	const char *infile, *outfile;
} opt;

static void gbailif(GError* error)
{
	if(!error) return;
	printf("LM:Error %d %d - %s\n",error->code,error->domain,error->message);
	exit(error->code);
}

static void verb(const char* msg, ...)
{
	if(!opt.verbose) return;
  va_list ap;
  va_start(ap, msg); 
	gchar* str;
	g_vasprintf(&str,msg,ap);
	printf("JT:%s\n",str);
}

static void bail(const char* msg)
{
	printf("JT:BAILING!\nJT:%s\n",msg);
	exit(1);
}

static void LibmirageVerboseLog(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	printf("LM:%s",message);
}

static void LibmirageSyncVerbose()
{
	if(!opt.mirage_in && !opt.mirage_out) return;
	g_log_set_handler (NULL, (GLogLevelFlags)(G_LOG_LEVEL_DEBUG), opt.verbose?LibmirageVerboseLog:NULL, NULL);
}

static void LibmirageInit()
{
	static bool initialized = false;
	if(initialized) return;

	initialized = true;

	#ifdef _MSC_VER
		msc_libmirage_init();
	#endif

	GError* error = NULL;
	mirage_initialize(&error);
	gbailif(error);

	LibmirageSyncVerbose();
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

static int myJadCreateCallback(void* opaque, int sectorNumber, void** sectorBuffer, void **subcodeBuffer)
{
	//TODO - this might leave a half-finished file if we bail from here. let's return an error to let the jad processes complete gracefully

	CreateContext* ctx = (CreateContext*)opaque;

	//close the old sector, since the buffer is done being used
	if(ctx->sector != NULL)
		g_object_unref(ctx->sector);

	//open sector
	GError *error = NULL;
	ctx->sector = mirage_disc_get_sector(ctx->disc,sectorNumber,&error);
	gbailif(error);

	//extract data
	const guint8 *secbuf, *subbuf;
	mirage_sector_extract_data(ctx->sector,&secbuf,2352,MIRAGE_SUBCHANNEL_PW,&subbuf,96,&error);
	gbailif(error);

	static uint8_t s_subbuf[96]; //this is needed because libmirage sends us interleaved subchannel

	//but for now, dont bother
	memcpy(s_subbuf,subbuf,96);

	*sectorBuffer = (void*)secbuf;
	*subcodeBuffer = (void*)secbuf;

	return JAD_OK;
}

void command_jadjac(bool isjac)
{
	//for now, thats just how it is
	opt.mirage_in = true;
	LibmirageInit();

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

	//well... i dont know. i guess we'll skip the lead-in, or else we read too many sectors next
	length -= 150;

	//libjad will fire callbacks, so this will track our process stte
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

	//write it out and close the jad
	jadStream outstream;
	if(jadstd_OpenStdio(&outstream,opt.outfile,"wb") != JAD_OK)
		bail("failed opening output file");
	jadDump(&jad,&outstream,isjac?1:0);
	jadstd_CloseStdio(&outstream);
	jadClose(&jad);

	//make sure the last sctor is shut down
	if(cc.sector != NULL)
		g_object_unref(cc.sector);

	g_object_unref(disc);
}

int main(int argc, char** argv)
{
	ArgQueue aq(argc,argv);


	#ifdef _MSC_VER
		msc_init();
	#endif

	while(aq.curr())
	{
		if(aq.is("v"))
		{
			opt.verbose = true;
			LibmirageSyncVerbose();
		}
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
				LibmirageInit();
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