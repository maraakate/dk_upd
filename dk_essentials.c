/*
Copyright (C) 2016-2017 Frank Sapone

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
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

void Con_Printf (const char *fmt, ...) /* FS: Adapted From KMQ2 */
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if (silent)
		return;

	va_start (argptr,fmt);
//	vsprintf (msg,fmt,argptr);
	KMQ2_vsnprintf(msg, sizeof(msg), fmt, argptr);	// Knightmare 10/28/12- buffer-safe version
	va_end (argptr);

	printf("%s", msg);
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

	Con_Printf("%s", msg);
}

void COM_StripExtension (char *in, char *out) /* FS: From Q2 */
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}
