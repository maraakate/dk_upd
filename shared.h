#ifndef __SHARED_H
#define __SHARED_H

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
	#define PLATFORM "Windows"
	#ifdef _WIN64
		#define __PLATFORM_EXT__ "win64"
	#else
		#define __PLATFORM_EXT__ "win32"
	#endif
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

#define HTTP_SIG_SIZE 512

#define	MAX_QPATH			64		// max length of a quake game pathname

#ifdef __cplusplus
typedef int	qboolean;
#else
typedef enum {false, true}	qboolean;
#endif

typedef struct
{
	char *fileName;
	char *md5FileName;
	char originalDownloadFile[HTTP_SIG_SIZE];
	char downloadfile[HTTP_SIG_SIZE];
	char *filepath;
	unsigned char pakFileSignature[16];
	char pakHttp_md5[HTTP_SIG_SIZE];
	const char *description;
}
pakfiles_t;

extern qboolean Debug;
extern qboolean silent;
extern qboolean skipPrompts;
extern char dk_updName[];

void Sys_Mkdir (char *path);
void NET_Init (void);
void NET_Shutdown (void);
int Sys_DeleteFile(char *file);
void Sys_ClearConScreen (void);
void Sys_SleepMilliseconds(int ms);
void Sys_Error (void);
void Sys_Quit (void);
int Sys_Milliseconds (void);
unsigned int Sys_ExecuteFile (const char *fileName, const char *parameters, unsigned int flags, qboolean bWaitToFinish);
void Error_Shutdown(void);
void Sys_CheckBinaryType (pakfiles_t *pakfile);

#endif // __SHARED_H
