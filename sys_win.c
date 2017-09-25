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
#include <winsock2.h>
#include <winerror.h>
#include <direct.h>

#include "dk_essentials.h"
#include "shared.h"
#include "http_dl.h"

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

void Sys_Quit(void)
{
	CURL_HTTP_Shutdown();
	NET_Shutdown();
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

static qboolean Detect_WinNT6 (void)
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

	if (WinLowByte == 5)
	{
		Con_DPrintf("Windows 2k/XP/2003 Detected.\n");
		return false;
	}

	if (WinLowByte > 5)
	{
		Con_DPrintf("Windows Vista or Later Detected.\n");
		return true;
	}

	return false;
}

unsigned int Sys_ExecuteFile (const char *fileName, const char *parameters, unsigned int flags, qboolean bWaitToFinish)
{
	SHELLEXECUTEINFO shExInfo = {0};
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	if (Detect_WinNT6()) /* FS: This doesn't work with XP or earlier... */
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

	if (ShellExecuteEx(&shExInfo) && bWaitToFinish)
	{
		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
	}

	return 0;
}
