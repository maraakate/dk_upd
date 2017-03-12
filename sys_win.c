#include <winsock.h>
#include <winerror.h>
#include <direct.h>

#include "dk_essentials.h"
#include "shared.h"

WSADATA ws;

void NET_Init (void)
{
	int err;

	err = WSAStartup ((WORD)MAKEWORD (1,1), &ws);
	if (err)
	{
		Con_Printf("Error loading Windows Sockets! Error: %i\n",err);
		Error_Shutdown();
	}
	else
	{
		Con_DPrintf("[I] Winsock Initialized\n");
	}
}

void NET_Shutdown (void)
{
	WSACleanup();
}

void Sys_Mkdir (char *path)
{
	_mkdir (path);
}

int Sys_DeleteFile (char *file)
{
	return DeleteFile(file);
}

void Sys_ClearConScreen (void)
{
	system("cls");
}

void Sys_Error (void)
{
	char error_buff[512]; /* FS: Get the error message. */

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), error_buff, sizeof(error_buff), NULL);
	Con_Printf ("Error: %s'\n", error_buff) ;
}

void Sys_SleepMilliseconds (int ms)
{
	if (ms < 0)	return;
	Sleep (ms);
}

int	curtime;
int Sys_Milliseconds (void)
{
	static int		base;
	static qboolean	initialized = false;

	if (!initialized)
	{	// let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xffff0000;
		initialized = true;
	}
	curtime = timeGetTime() - base;

	return curtime;
}

unsigned int Sys_ExecuteFile (const char *fileName, unsigned int flags)
{
	return WinExec(fileName, flags);
}
