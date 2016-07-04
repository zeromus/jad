#include <stdio.h>

#include "../libjad/jad.h"

//opens a jadContext, which gets its data from the provided stream (containing a cue file) and allocator
//TODO: pass in other things, like a subfile resolver
int jadvacOpenFile_cue(jadContext* jad, jadStream* stream, jadAllocator* allocator)
{
	return JAD_OK;
}
