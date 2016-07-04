#ifndef JADTOOL_H_
#define JADTOOL_H_

enum JAD_API {
	JAD_API_NONE,
	JAD_API_JAD,
	JAD_API_MIRAGE,
	JAD_API_MEDNAFEN,
	JAD_API_VAC
};

struct Options {
	Options()
		: verbose(false)
		, in(JAD_API_JAD)
		, out(JAD_API_JAD)
		, infile(nullptr)
		, outfile(nullptr)
	{
	}
	bool verbose;
	JAD_API in, out;
	const char *infile, *outfile;
};

void bail(const char* msg);
void verb(const char* msg, ...);

#endif
