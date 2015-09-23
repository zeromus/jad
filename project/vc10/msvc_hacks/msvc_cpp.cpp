#include <stdio.h>
#include <string.h>
#include <ctype.h>

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