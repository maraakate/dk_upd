#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define DG_MISC_IMPLEMENTATION // FS: Caedes special string safe stuff
#include "dk_essentials.h"

void Com_sprintf( char *dest, int size, const char *fmt, ... ) /* FS: From KMQ2 */
{
	// DG: implement this with vsnprintf() instead of a big buffer etc
	va_list	argptr;

	va_start(argptr,fmt);
	KMQ2_vsnprintf(dest, size, fmt, argptr);
	// TODO: print warning if cut off!
	va_end(argptr);
}

void Con_DPrintf (const char *fmt, ...) /* FS: Adapted From KMQ2 */ 
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if (!Debug)	// don't confuse non-developers with techie stuff...
		return;

	va_start (argptr,fmt);
//	vsprintf (msg,fmt,argptr);
	KMQ2_vsnprintf(msg, sizeof(msg), fmt, argptr);	// Knightmare 10/28/12- buffer-safe version
	va_end (argptr);

	printf("%s", msg);
}

void COM_StripExtension (char *in, char *out) /* FS: From Q2 */
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}
