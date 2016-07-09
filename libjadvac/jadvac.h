#ifndef JADVAC_H_
#define JADVAC_H_

#include "../libjad/jad.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct jadContext;
struct jadStream;
struct jadAllocator;

//callback for a jadvacFilesystem's open operation.
//any logic to attempt other extensions (e.g. myfile.bin.ecm instead of myfile.bin) is the responsibility of libjadbac
//this callback's job is simply to open the file. however, it should process paths relative to the basefile (.cue, etc.)
//open files in "rb" mode.
typedef int (*jadvacOpenFileCallback)(struct jadvacFilesystem* fs, struct jadStream* stream, const char* path);

//callback for a libjadvacFilesystem's close operation
typedef int (*jadvacCloseFileCallback)(struct jadvacFilesystem* fs, struct jadStream* stream);

typedef struct jadvacFilesystem
{
	void* opaque;
	jadvacOpenFileCallback open;
	jadvacCloseFileCallback close;
} jadvacFilesystem;

typedef struct jadvacContext
{
	jadContext* jad;
	jadStream* stream;
	jadAllocator* allocator;
	jadvacFilesystem* fs;
} jadvacContext;

int jadvacOpenFile_cue(jadvacContext* ctx);

#ifdef  __cplusplus
}
#endif

#endif //JADVAC_H_
