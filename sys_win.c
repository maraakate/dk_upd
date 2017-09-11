#include <winsock2.h>
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

static qboolean Detect_WinNT (void)
{
	DWORD WinVersion;
	DWORD WinLowByte, WinHiByte;

	WinVersion = GetVersion();
	WinLowByte = (DWORD)(LOBYTE(LOWORD(WinVersion)));
	WinHiByte = (DWORD)(HIBYTE(HIWORD(WinVersion)));

	if (WinLowByte <= 4)
	{
		Con_DPrintf("Windows 9x Detected.\n");
		return false;
	}

	if (WinLowByte > 4)
		return true;

	return false;
}

unsigned int Sys_ExecuteFile (const char *fileName, const char *parameters, unsigned int flags)
{
	SHELLEXECUTEINFO shExInfo = {0};
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	if (Detect_WinNT())
	{
		shExInfo.lpVerb = "runas";
	}
	else
	{
		shExInfo.lpVerb = "open";
	}
	shExInfo.lpFile = fileName;
	shExInfo.lpParameters = parameters;
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;

	if (ShellExecuteEx(&shExInfo))
	{
		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
	}

	return 0;
}
