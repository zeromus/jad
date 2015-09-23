#include <stdlib.h>

#include <mirage.h>
#include <monolithic.h>

#include "ProgressManager.h"

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

void Libmirage_SyncVerbose(bool verbose)
{
	//if(!opt.mirage_in && !opt.mirage_out) return; //needed?
	g_log_set_handler (NULL, (GLogLevelFlags)(G_LOG_LEVEL_DEBUG), verbose?_LibmirageVerboseLog:NULL, NULL);
}

void Libmirage_Init()
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
