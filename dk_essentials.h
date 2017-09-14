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

#ifndef _DK_ESSENTIALS_H
#define _DK_ESSENTIALS_H

#include "shared.h"

void Com_sprintf( char *dest, int size, const char *fmt, ... );
void Con_Printf (const char *fmt, ...);
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
