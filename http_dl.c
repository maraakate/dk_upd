/*
Copyright (c) 2016-2018 Frank Sapone <fhsapone@gmail.com>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#define CURL_STATICLIB
#define CURL_DISABLE_LDAP
#define CURL_DISABLE_LDAPS
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#ifdef _WIN32
#include <io.h>
#endif // WIN32

#include <curl.h>

#define CURL_ERROR(x)	curl_easy_strerror(x)

#include "http_dl.h"
#include "dk_essentials.h"
#include "filesystem.h"

static int curl_borked;
static int retryDeleteCount = 0;
static CURL *easy_handle;
static CURLM *multi_handle;
curl_dltype_t dltype;

FILE *download;
char name[MAX_PATH];
char completedURL[MAX_URLLENGTH];
qboolean cleanUp;

/* FS: For KBps calculator */
int		downloadpercent;
int		prevSize;
int		previousTime;
int		bytesRead;
int		byteCount;
float	timeCount;
float	previousTimeCount;
float	startTime;
float	downloadrate;

void CURL_Download_Calculate_KBps (int byteDistance, int totalSize)
{
	float timeDistance = (float)(Sys_Milliseconds()-previousTime);
	float totalTime = (timeCount-startTime) / 1000.0f;

	timeCount += timeDistance;
	byteCount += byteDistance;
	bytesRead += byteDistance;

	if (totalTime >= 1.0f)
	{
		downloadrate = (float)byteCount / 1024.0f;
		Con_Printf( "Rate: %7.2fKB/s, Downloaded %4.2fMB of %4.2fMB [%i%%]%\r", downloadrate, (float)(bytesRead/1024.0f)/1024.0f, (float)(totalSize/1024.0f)/1024.0f, downloadpercent);
		// FS: Start over
		byteCount = 0;
		startTime = (float)Sys_Milliseconds();
	}
	previousTime = Sys_Milliseconds();
}


void CURL_HTTP_Reset_KBps_Counter (void)
{
	previousTime = bytesRead = byteCount = 0;
	timeCount = previousTimeCount = 0.0f;
	startTime = (float)Sys_Milliseconds();
	prevSize = 0;
	downloadpercent = 0;
}

void CURL_HTTP_Calculate_KBps (int curSize, int totalSize)
{
	int byteDistance = curSize-prevSize;

	CURL_Download_Calculate_KBps (byteDistance, totalSize);
	prevSize = curSize;
}

static int http_progress (void *clientp, double dltotal, double dlnow,
			   double ultotal, double uplow)
{
	if (dltotal)
	{
		downloadpercent = (int)((dlnow / dltotal) * 100.0f);
		CURL_HTTP_Calculate_KBps((int)dlnow, (int)dltotal);
	}
	else
	{
		downloadpercent = 0;
	}

	return 0;	//non-zero = abort
}

static size_t http_write (void *ptr, size_t size, size_t nmemb, void *stream)
{
	return fwrite (ptr, 1, size *nmemb, download);
}

static size_t http_write_md5 (void *ptr, size_t size, size_t nmemb, void *stream)
{
	if(nmemb >= HTTP_SIG_SIZE)
	{
		Con_Printf("Error: temporary file greater than buffer %d / %d!  Please report this as a bug: %s!\n", nmemb, HTTP_SIG_SIZE, completedURL);
		Error_Shutdown();
	}

	memcpy(stream, ptr, nmemb);
	return 0;
}

static size_t http_write_fake (void *ptr, size_t size, size_t nmemb, void *stream)
{
	/* FS: Fake to check 404 */
	return nmemb;
}

void CURL_HTTP_Init (void)
{
	if ((curl_borked = curl_global_init (CURL_GLOBAL_NOTHING)))
	{
		return;
	}

	multi_handle = curl_multi_init ();
}

void CURL_HTTP_Shutdown (void)
{
	if (curl_borked)
	{
		return;
	}

	curl_multi_cleanup (multi_handle);
	curl_global_cleanup ();
}

int CURL_HTTP_StartDownload (const char *url, char *filename)
{
	CURL_HTTP_Reset_KBps_Counter();

	if (!filename || filename[0] == '\0')
	{
		Con_Printf("Error: %s: Filename is blank!\n", __func__);
		Error_Shutdown();
		return 0;
	}

	if (!url || url[0] == '\0')
	{
		Con_Printf("Error: %s: URL is blank!\n", __func__);
		Error_Shutdown();
		return 0;
	}

	if (!strcmp(filename, dk_updName)) /* FS: FIXME: Is there a way to check 404 without downloading all this into memory? */
	{
		char cmdline[512];
		CURL *test_handle = NULL;

		test_handle = curl_easy_init();
		curl_easy_setopt (test_handle, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt (test_handle, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt (test_handle, CURLOPT_WRITEDATA, NULL);
		curl_easy_setopt (test_handle, CURLOPT_WRITEFUNCTION, http_write_fake);
		curl_easy_setopt (test_handle, CURLOPT_URL, url);
		if (test_handle)
		{
			long response_code = 0;
			CURLcode res;
			qboolean bFailed = false;

			res = curl_easy_perform(test_handle);
			curl_easy_getinfo(test_handle, CURLINFO_RESPONSE_CODE, &response_code);
			if (res == CURLE_OK)
			{
				Con_DPrintf("Response_code: %li\n", response_code);
				if (response_code != HTTP_OK)
					bFailed = true;
			}
			else
			{
				Con_DPrintf("Error: %s: CURL failed %d\n", __func__, res);
				bFailed = true;
			}

			curl_easy_cleanup (test_handle);
			test_handle = 0;
			if (bFailed)
			{
				char reason[64];

				if (response_code)
					Com_sprintf(reason, sizeof(reason), "%d", response_code);
				else
					strncpy(reason, "Unknown", sizeof(reason)-1);

				Con_Printf("Error: %s: Download failed for %s.  Reason: %s.\n", __func__, filename, reason);
				return 0;
			}
		}
		Com_sprintf(cmdline, sizeof(cmdline), "url.dll,FileProtocolHandler %s", url);

		Sys_ExecuteFile("rundll32.exe", cmdline, 0, false);
		Sys_Quit();
		exit(0);
		return 1;
	}

	Com_sprintf(name, sizeof(name), "%s", filename);
	if (!download)
	{
		download = fopen (name, "wb");

		if (!download)
		{
			Con_Printf ("Error: %s: Failed to open %s\n", __func__, name);
			Error_Shutdown();
			return 0;
		}
	}

	if(strstr(filename, ".zip"))
	{
		dltype = zipfile;
	}
	else
	{
		dltype = file;
	}

	Com_sprintf(completedURL, sizeof(completedURL), "%s", url);
	Con_DPrintf("[I] HTTP Download URL: %s\n", completedURL);
	easy_handle = curl_easy_init ();

	curl_easy_setopt (easy_handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt (easy_handle, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt (easy_handle, CURLOPT_WRITEFUNCTION, http_write);
	curl_easy_setopt (easy_handle, CURLOPT_URL, completedURL);
	curl_easy_setopt (easy_handle, CURLOPT_PROGRESSFUNCTION, http_progress);

	curl_multi_add_handle (multi_handle, easy_handle);

	return 1;
}

void CURL_HTTP_StartMD5Checksum_Download (const char *url, void *stream)
{
	CURL_HTTP_Reset_KBps_Counter();

	if (!stream)
	{
		Con_Printf("Error: %s: stream is NULL!\n", __func__);
		return;
	}

	if (!url || url[0] == '\0')
	{
		Con_Printf("Error: %s: URL is NULL!\n", __func__);
		return;
	}

	dltype = md5;

	Com_sprintf(completedURL, sizeof(completedURL), "%s", url);
	Con_DPrintf("[I] HTTP Download URL: %s\n", completedURL);
	easy_handle = curl_easy_init ();

	curl_easy_setopt (easy_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt (easy_handle, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt (easy_handle, CURLOPT_WRITEDATA, stream);
	curl_easy_setopt (easy_handle, CURLOPT_WRITEFUNCTION, http_write_md5);
	curl_easy_setopt (easy_handle, CURLOPT_URL, completedURL);

	curl_multi_add_handle (multi_handle, easy_handle);
}

int CURL_HTTP_Update (void)
{
	int         running_handles;
	int         messages_in_queue;
	CURLMsg    *msg;

	curl_multi_perform (multi_handle, &running_handles);
	while ((msg = curl_multi_info_read (multi_handle, &messages_in_queue)))
	{
		if (msg->msg == CURLMSG_DONE)
		{
			long        response_code;

			curl_easy_getinfo (msg->easy_handle, CURLINFO_RESPONSE_CODE,
							   &response_code);
			Con_DPrintf("HTTP URL response code: %li\n", response_code);

			if ( (response_code == HTTP_OK || response_code == HTTP_REST))
			{
				if(downloadpercent)
				{
					Con_Printf( "Rate: %7.2fKB/s, Downloaded %4.2fMB of %4.2fMB [%i%%]%\r", downloadrate, (float)(bytesRead/1024.0f)/1024.0f, (float)(bytesRead/1024.0f)/1024.0f, downloadpercent);
					Con_Printf("\n");
				}

				if(dltype >= file)
				{
					Con_Printf("[I] HTTP Download of %s completed\n", name); // FS: Tell me when it's done
				}

				if(download)
				{
					fclose(download);
				}

				if (dltype == zipfile)
				{
					char dlFileInZip[MAX_OSPATH];
					char dlPath[MAX_OSPATH];
					int i;

					Com_sprintf(dlFileInZip, sizeof(dlFileInZip), "%s", name);
					COM_StripExtension(dlFileInZip, dlFileInZip);
					strcat(dlFileInZip, ".pak");

					for (i = 5; i < sizeof(dlFileInZip); i++) /* FS: Skip past "data/" */
					{
						if(dlFileInZip[i] == '\0')
						{
							dlPath[i-5] = '\0';
							break;
						}
						dlPath[i-5] = dlFileInZip[i];
					}

					FS_DecompressFile(dlFileInZip, name, dlPath);

					if(!Sys_DeleteFile(name))
					{
						Con_Printf("Error deleting temporary file: %s\n", name);
						Sys_Error();
					}
				}

				if (dltype >= file)
				{
					char upgradePath[MAX_OSPATH];
					char parameters[256];

					if(strstr(name, ".exe") || strstr(name, ".EXE"))
					{
						strcpy(upgradePath, name);
						if(skipPrompts)
						{
							strcpy(parameters, "/S");
						}
						Con_Printf("Starting Daikatana v1.3 Upgrade...\n");
						Sys_ExecuteFile(upgradePath, parameters, 0, true);
						cleanUp = true;
						return 0;
					}
				}
				download = NULL;
			}
			else
			{
				if(downloadpercent)
				{
					Con_Printf("\n");
				}

				if(download)
				{
					fclose(download);
					Sys_DeleteFile(name);
				}
				download = NULL;

				Con_Printf("Error: HTTP Download Failed: %ld.\n", response_code);
				CURL_HTTP_Reset();
				return HTTP_MD5_DL_FAILED;

			}

			CURL_HTTP_Reset();
		}
		return 1;
	}

	if (cleanUp)
	{
		if(!Sys_DeleteFile(name) && retryDeleteCount < 120) /* FS: Try to get rid of the temp file for 2 minutes.  FIXME: Probably can just find a way to poll if the process is still open.  Must be Win9x compatible. */
		{
//			Con_Printf("Error deleting temporary file: %s\n", name);
			Sys_SleepMilliseconds(1000);
			retryDeleteCount++;
			return 0;
		}
		else
		{
			download = NULL;
			CURL_HTTP_Reset();
			return 1;
		}
	}

	return 0;
}

void CURL_HTTP_Reset (void)
{
	curl_multi_remove_handle (multi_handle, easy_handle);
	curl_easy_cleanup (easy_handle);
	easy_handle = 0;
	dltype = none;
	cleanUp = false;
	retryDeleteCount = 0;
}
