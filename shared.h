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
#ifndef __SHARED_H
#define __SHARED_H

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
	#define PLATFORM "Windows"
	#define __PLATFORM_EXT__ "win32"
#elif defined(__linux__)
	#define PLATFORM "Linux"
	#define __PLATFORM_EXT__ "linux"
#elif defined(__FreeBSD__)
	#define PLATFORM "FreeBSD"
	#define __PLATFORM_EXT__ "bsd"
#elif defined(__APPLE__)
	#define PLATFORM "Mac OS X"
	#define __PLATFORM_EXT__ "mac"
#else
	#define PLATFORM "Unknown"
	#define __PLATFORM_EXT__ "zzz"
#endif

#ifdef _WIN32 /* FS: Shut up VS2005 */
#define getch _getch
#define stricmp _stricmp
#define strdup _strdup
#pragma warning( disable : 4996 ) /* FS: Shut up about VS2005 shit */
#endif


#ifdef _WIN32
	#if defined(_MSC_VER) && _MSC_VER >= 1400
		#define __func__ __FUNCTION__
	#else
		#define __func__ ""
	#endif
#endif // _WIN32

#ifdef _WIN32
// DG: actually, the real limit windows-size is 260 (MAX_PATH)
//     see http://msdn.microsoft.com/en-us/library/aa365247%28VS.85%29.aspx#maxpath
// and Win NT can do up to 32k or so in unicode-mode or something
#define	MAX_OSPATH			260		// max length of a filesystem pathname
#else
#define MAX_OSPATH			4096 // as in /usr/include/linux/limits.h
#endif

#define byte unsigned char
#define MAXPRINTMSG 16384
#define MAX_URLLENGTH	4000 /* FS: See http://boutell.com/newfaq/misc/urllength.html.  Apache is 4000 max.  This is pretty damn long for a URL. */

#define HTTP_SIG_SIZE 1024

#define	MAX_QPATH			64		// max length of a quake game pathname

typedef enum {false, true}	qboolean;

extern qboolean Debug;
extern qboolean silent;
extern qboolean skipPrompts;

void Sys_Mkdir (char *path);
void NET_Init (void);
void NET_Shutdown (void);
int Sys_DeleteFile(char *file);
void Sys_ClearConScreen (void);
void Sys_SleepMilliseconds(int ms);
void Sys_Error (void);
int Sys_Milliseconds (void);
unsigned int Sys_ExecuteFile (const char *fileName, const char *parameters, unsigned int flags);
void Error_Shutdown(void);

#endif // __SHARED_H
