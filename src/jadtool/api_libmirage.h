#ifndef _JAD_API_LIBMIRAGE_H_

typedef struct _GError GError;

void Libmirage_SyncVerbose(bool verbose);
void Libmirage_Init();

//lame
void gbailif(GError* error);
bool gerrprint(GError* error);

#endif