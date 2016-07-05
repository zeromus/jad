#include <stdio.h>

#include "../libjad/jad.h"

typedef struct CueLexer
{
	int lastc, unget;
	int line, col;
	jadStream* stream;
} CueLexer;

static int read(CueLexer* L)
{
	for (;;)
	{
		int c;
		if(L->lastc == '\n') { L->line++; L->col = 1; }
		if(L->unget) { c = L->unget; L->unget = 0; }
		else { c = L->stream->get(L->stream); L->col++; }

		if (c == '\r')
		{
			c = L->stream->get(L->stream);
			if (c == '\n')
			{
				// \r\n was received; eat and return \n
			}
			else
			{
				// \r ? was received. return \n and unget ?
				// \r\r was received; return \n and unget \r
				L->unget = c;
				c = '\n';
			}
		}
		else
		{
			// \n was received; eat and return \n
			// ? was received; eat and return ?
		}

		//for convenience in the lexer, we transform EOF to \0
		if (c == JAD_EOF) c = 0;

		return L->lastc = c;
	}

}

#define READ() { c = read(L); }

//opens a jadContext, which gets its data from the provided stream (containing a cue file) and allocator
//TODO: pass in other things, like a subfile resolver
int jadvacOpenFile_cue(jadContext* jad, jadStream* stream, jadAllocator* allocator)
{
	int c;
	CueLexer _L = {0}, *L = &_L;
	L->lastc = '\n';
	L->stream = stream;

	READ();

	while (c  && (c == ' ' || c == '\t')) READ();

	return JAD_OK;
}
