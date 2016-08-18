#ifndef _DK_ESSENTIALS_H
#define _DK_ESSENTIALS_H

#include "shared.h"

void Com_sprintf( char *dest, int size, const char *fmt, ... );
void Con_DPrintf (const char *fmt, ...);
void COM_StripExtension (char *in, char *out);

// Knightmare 05/27/12- buffer-safe variant of vsprintf
// This may be different on different platforms, so it's abstracted
#ifdef _MSC_VER	// _WIN32
//#define KMQ2_vsnprintf _vsnprintf	
__inline int KMQ2_vsnprintf (char *Dest, size_t Count, const char *Format, va_list Args) {
	int ret = _vsnprintf(Dest, Count, Format, Args);
	Dest[Count-1] = 0;	// null terminate
	return ret;
}
#else
#define KMQ2_vsnprintf vsnprintf	
#endif // _MSC_VER

#endif // _DK_ESSENTIALS_H
