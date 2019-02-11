#include <stdlib.h>

#include <mirage.h>
#include <monolithic.h>

#include "ProgressManager.h"
#include "jadtool.h"
#include "../libjad/jad.h"

static void _LibmirageVerboseLog(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	g_ProgressManager.TryNewline();
	printf("LM:%s",message);
}

bool gerrprint(GError* error)
{
	if(!error) return false;
	g_ProgressManager.TryNewline();
	printf("LM:Error %d %d - %s\n",error->code,error->domain,error->message);
	return true;
}

void gbailif(GError* error)
{
	if(gerrprint(error))
	exit(error->code);
}

static void _initLibmirage()
{
	static bool initialized = false;
	if(initialized) return;

	initialized = true;

	//MSVC builds need special initialization of libmirage since it isn't a unixy environment and it's linked into this process
	#ifdef _MSC_VER
		void msc_libmirage_init();
		msc_libmirage_init();
	#endif

	GError* error = NULL;
	mirage_initialize(&error);
	gbailif(error);
}

struct CreateContext
{
	CreateContext()
		: disc(NULL)
		, sector(NULL)
	{
	}

	~CreateContext()
	{
		//make sure the last mirage sector is shut down
		if(sector != NULL)
			g_object_unref(sector);

		g_object_unref(disc);
	}

	MirageDisc* disc;
	MirageSector* sector;
};

//delivers sectors to the library when theyre requested for encoding.
//management of the sector buffers is tricky.
static int _jadCreateCallback(void* opaque, int sectorNumber, void** sectorBuffer, void **subcodeBuffer)
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

int jt_api_libmirage_start(Options *opt, jadCreationParams *jcp)
{
	_initLibmirage();

	g_log_set_handler (NULL, (GLogLevelFlags)(G_LOG_LEVEL_DEBUG), opt->verbose?_LibmirageVerboseLog:NULL, NULL);

	GError *error = NULL;

	MirageContext* context = (MirageContext*)g_object_new(MIRAGE_TYPE_CONTEXT,NULL);
	if(opt->verbose)
		mirage_context_set_debug_mask(context,MIRAGE_DEBUG_PARSER);

	const char* fnames[] = { opt->infile, NULL };
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
	CreateContext *cc = new CreateContext();
	cc->disc = disc;

	//instructions to libjad for how to create the jad
	jcp->opaque = cc;
	//jcp->numTocEntries = 0;
	//jcp->tocEntries = NULL;
	jcp->numSectors = length;
	jcp->callback = _jadCreateCallback;

	return JAD_OK;
}

int jt_api_libmirage_end(Options *opt, jadCreationParams *jcp)
{
	CreateContext *cc = (CreateContext *)jcp->opaque;
	delete cc;
	return JAD_OK;
}
