#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

extern "C" double baseline_strtod (const char* str, char** endptr);

//hopefully this is an adequate implementation
extern "C" _Check_return_ _CRTIMP double __cdecl strtod(_In_z_ const char * _Str, _Out_opt_ _Deref_post_z_ char ** _EndPtr)
{
	//add hex parsing capability
	if(_Str[0] == '0' && _Str[1] && (_Str[1] == 'X' || _Str[1] == 'x'))
	{
		unsigned int u;
		_Str += 2;
		sscanf(_Str,"%x",&u);
		if(!_EndPtr)
			return u;

		for(;;)
		{
			char c = *_Str++;
			if(!c) break;
			if( (c >= '0' && c <= '9') || (c>='a' && c<='f') || (c>='A' && c<='F'))
				c++;
			break;
		}
		*_EndPtr = (char*)_Str;
		return u;
	}

	//use someone else's strtod (without hex capability)
	return baseline_strtod(_Str, _EndPtr);
}

#include <mirage.h>
#include <monolithic.h>

#define stricmp _stricmp
extern "C" void g_clock_win32_init();
extern "C" void g_thread_win32_init();
extern "C" void glib_init();

void msc_libmirage_init()
{
	//initialization that glib dlls would normally do, approximately
	g_clock_win32_init();
	g_thread_win32_init();
	glib_init();
	g_type_init();

	//special for us because we use monolithic libmirage
	mirage_preinitialize_monolithic();
}

//=============================================================
//=============================================================
//=============================================================
//=============================================================

/*-
 * Copyright (c) 2006 Verdens Gang AS
 * Copyright (c) 2006-2007 Linpro AS
 * All rights reserved.
 *
 * Author: Dag-Erling Smørgrav <des@linpro.no>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define va_copy(a,b) memcpy((&a),(&b),sizeof(b))

int vasprintf(char **strp, const char *fmt, va_list ap)
{
        va_list aq = NULL;
        int ret;

        va_copy(aq, ap);
        ret = vsnprintf(NULL, 0, fmt, aq);
        va_end(aq);
        if ((*strp = (char*)malloc(ret + 1)) == NULL)
                return (-1);
        ret = vsnprintf(*strp, ret + 1, fmt, ap);
        return (ret);
}
