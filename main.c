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
/*
  Daikatana Auto-Updater.

  Based on MD5.EXE by John Walker: http://www.fourmilab.ch/
*/

#define VERSION     "0.4"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <stdlib.h>
#include <conio.h>
#endif

#include "md5.h"
#include "http_dl.h"
#include "shared.h"
#include "dk_essentials.h"
#include "filesystem.h"

#ifdef _WIN32
char dk_updName[] = "dk_upd.exe";
#else
char dk_updName[] = "dk_upd";
#endif // WIN32

#define DK13_MIRROR1_URL "http://maraakate.org/dk_update/"
#define DK13_MIRROR2_URL "http://maraakate.org/DK13/"
#define DK13_MIRROR3_URL "http://dk.toastednet.org/DK13/"

typedef struct
{
	const char baseurl[MAX_URLLENGTH];
	int apiVersion;
	qboolean bFailed;
} filemirrors_t;

filemirrors_t filemirrors[]=
{
	{DK13_MIRROR1_URL, UPDATER_API_LEGACY, false},
	{DK13_MIRROR2_URL, UPDATER_API_LEGACY, false},
	{DK13_MIRROR3_URL, UPDATER_API_LEGACY, false},
	0
};

#define TOTAL_MIRRORS ((sizeof(filemirrors) / sizeof(filemirrors[0])) - 1)

filemirrors_t *availableMirrors = NULL;
filemirrors_t *headAvailableMirror = NULL;
int availableMirrorsCnt = 0;

pakfiles_t pakfiles[]=
{
	{dk_updName, "Type=3", "dk_upd.md5", "", "", dk_updName, "", "", "Daikatana v1.3 Auto-Updater"},
	{"daikatana.exe", "Type=0&Arch=0", "dk_"__PLATFORM_EXT__".md5", "", "", "daikatana.exe", "", "", "Daikatana v1.3 Binary"}, /* FS: Keep this before the PAKs because the latest build should have the latest pak4.pak */
	{"pak4.pak", "Type=2&Pak=0", "pak4.md5", "pak4.pak", "pak4.pak", "data/pak4.pak", "", "", "Widescreen HUD, Script Fixes, etc."},
	{"pak5.pak", "Type=2&Pak=1", "pak5.md5", "pak5.zip", "pak5.zip", "data/pak5.pak", "", "", "32-bit Textures (Optional)"},
	{"pak6.pak", "Type=2&Pak=2", "pak6.md5", "pak6.zip", "pak6.zip", "data/pak6.pak", "", "", "Map Updates (Recommended)"},
	0
};

/* FS: Runtime Vars */
qboolean Debug = false;
qboolean showfile = false;
qboolean silent = false;
qboolean skipPrompts = false;
qboolean forceFailure = false;
int currentMirror = 0;
char *hexfmt = "%02x";
int beta = 0;

/* FS: Prototypes */
qboolean Calc_MD5_File(pakfiles_t *pakfile);
int Download_Loop (void);
void Get_HTTP_MD5 (pakfiles_t *pakfile);
void Get_HTTP_Binary_Link(pakfiles_t *pakfile);
qboolean Check_MD5_Signatures (pakfiles_t *pakfile);
void Shutdown_DK_Update(void);
void Error_Shutdown(void);

void Get_HTTP_MD5 (pakfiles_t *pakfile)
{
	char url[MAX_URLLENGTH];
	int err = 0;

	memset((char *)pakfile->pakHttp_md5, 0, HTTP_SIG_SIZE);

	if (availableMirrors->apiVersion == UPDATER_API_VERSION1)
	{
		Com_sprintf(url, sizeof(url), "%sDownload/GetMD5?%s&Beta=%d", availableMirrors->baseurl, pakfile->queryParams, beta);
	}
	else
	{
		Com_sprintf(url, sizeof(url), "%s%s", availableMirrors->baseurl, pakfile->md5FileName);
	}
	CURL_HTTP_StartMD5Checksum_Download (url, pakfile->pakHttp_md5);
	err = Download_Loop();

	if (err == HTTP_MD5_DL_FAILED)
	{
		memset((char *)pakfile->pakHttp_md5, 0, sizeof(pakfile->pakHttp_md5));

		if (currentMirror == availableMirrorsCnt - 1)
		{
			currentMirror = 0;
			Con_Printf("Failed to check for updates.\n");
		}
		else
		{
			currentMirror++;
			availableMirrors++;
			Con_Printf("Failed to check for updates.  Trying Mirror.\n");
			Get_HTTP_MD5(pakfile);
		}
	}
	else
	{
		currentMirror = 0;
		availableMirrors = headAvailableMirror;
		pakfile->pakHttp_md5[32] = '\0';
		Get_HTTP_Binary_Link(pakfile);
	}
}

void Get_HTTP_Binary_Link(pakfiles_t *pakfile)
{
	char url[MAX_URLLENGTH] = { 0 } ;
	char fixedDownloadName[MAX_OSPATH] = { 0 };
	int err = 0;

	if (!strstr(pakfile->fileName, ".exe") && !strstr(pakfile->fileName, ".EXE"))
		return;

	COM_StripExtension(pakfile->md5FileName, fixedDownloadName);

	if (availableMirrors->apiVersion == UPDATER_API_VERSION1)
	{
		Com_sprintf(url, sizeof(url), "%sDownload/GetFileName?%s&Beta=%d", availableMirrors->baseurl, pakfile->queryParams, beta);
	}
	else
	{
		Com_sprintf(url, sizeof(url), "%s%s.txt", availableMirrors->baseurl, fixedDownloadName);
	}

	CURL_HTTP_StartMD5Checksum_Download (url, pakfile->downloadfile);
	err = Download_Loop();

	if (err == HTTP_MD5_DL_FAILED)
	{
		memset((char *)pakfile->downloadfile, 0, sizeof(pakfile->downloadfile));

		if (currentMirror == availableMirrorsCnt - 1)
		{
			currentMirror = 0;
			Con_Printf("Failed to check for updates.\n");
		}
		else
		{
			strncpy(pakfile->downloadfile, pakfile->originalDownloadFile, sizeof(pakfile->downloadfile)-1);
			currentMirror++;
			availableMirrors++;
			Con_Printf("Failed to check for updates.  Trying Mirror.\n");
			Get_HTTP_Binary_Link(pakfile);
		}
	}
	else
	{
		int x = 0;

		for (x = 0; x < sizeof(pakfile->downloadfile); x++)
		{
			if ((pakfile->downloadfile[x] == ' ') || (pakfile->downloadfile[x] == '\n') || (pakfile->downloadfile[x] == '\r') || (pakfile->downloadfile[x] == '\r\n')) /* FS: Echo in Windows command prompt adds a space and newline and fudges this up */
			{
				pakfile->downloadfile[x] = '\0';
				break;
			}
		}
		availableMirrors = headAvailableMirror;
		currentMirror = 0;
	}
}

qboolean Check_MD5_Signatures (pakfiles_t *pakfile)
{
	int i;
	char convertedSignature[33] = { 0 };

	for (i = 0; i<16; i++)
	{
		sprintf(convertedSignature+i*2, "%02x", pakfile->pakFileSignature[i]);
	}

	if (pakfile->pakHttp_md5[0] == '\0')
	{
		Con_Printf("No HTTP Signature!\n");
		return true;
	}
	else
	{
		if (!forceFailure && !stricmp(convertedSignature, pakfile->pakHttp_md5))
		{
			Con_Printf("No updates available.\n");
			return true;
		}
		else
		{
			Con_Printf("File mismatch!  ");
			return false;
		}
	}
	
	return false;
}

int Download_Loop (void)
{
	int bIsDone = 0;

	while(!bIsDone)
	{
		bIsDone = CURL_HTTP_Update();
		Sys_SleepMilliseconds(1);
	}

	return bIsDone;
}

void Get_PAK (pakfiles_t *pakfile, qboolean binary)
{
	int c;

	if (pakfile->description && pakfile->description[0] != '\0')
	{
		Con_Printf("\nFile Description: %s\n", pakfile->description);
	}

	if (pakfile->downloadfile[0] == '\0')
	{
		Con_DPrintf("Download URL invalid!\n");
		return;
	}

	Con_Printf("Do you want download %s? y/n", pakfile->downloadfile);

	if (!skipPrompts)
	{
		c = getch(); /* FS: FIXME: May not be portable */
	}

	Con_Printf("\n");

	if (skipPrompts || c == 'y' || c == 'Y')
	{
		char url[MAX_URLLENGTH] = { 0 };
		char fileName[MAX_QPATH] = { 0 };
		int err = 0;

retryDownload:
		if (!binary)
		{
			Sys_Mkdir("data");
		}

		if (availableMirrors->apiVersion == UPDATER_API_VERSION1)
		{
			Com_sprintf(url, sizeof(url), "%sDownload/GetLatestBuild?%s&Beta=%d", availableMirrors->baseurl, pakfile->queryParams, beta);
		}
		else
		{
			Com_sprintf(url, sizeof(url), "%s%s", availableMirrors->baseurl, pakfile->downloadfile);
		}

		if (!binary)
		{
			Com_sprintf(fileName, sizeof(fileName), "data/%s", pakfile->downloadfile);
		}
		else
		{
			Com_sprintf(fileName, sizeof(fileName), "%s", pakfile->downloadfile);
		}

		if (CURL_HTTP_StartDownload(url, fileName) == 0)
		{
			currentMirror++;
			availableMirrors++;
			goto retryDownload;
		}
		err = Download_Loop();
		if (err == HTTP_MD5_DL_FAILED)
		{
			if (currentMirror == availableMirrorsCnt - 1)
			{
				Con_Printf("Download Failed!\n");
			}
			else
			{
				currentMirror++;
				availableMirrors++;
				Con_Printf("Failed to check for updates.  Trying Mirror.\n");
				goto retryDownload;
			}
		}
	}

	availableMirrors = headAvailableMirror;
	currentMirror = 0;
}

void Check_MD5_vs_Local (pakfiles_t *pakfile)
{
	Con_Printf("%s: ", pakfile->fileName);

	if (pakfile->pakFileSignature[0] == 0)
	{
		Con_Printf("File missing!  ");

		if(!strstr(pakfile->fileName, ".exe") && !strstr(pakfile->fileName, ".EXE")) /* FS: Compare filename instead of downloadfile because it won't be set yet if the file is missing */
		{
			Get_PAK(pakfile, false);
		}
		else
		{
			Con_Printf("\nPlease run %s from your root Daikatana directory!\n", dk_updName);
			Error_Shutdown();
		}
	}
	else
	{
		qboolean bSameFile = false;

		Get_HTTP_MD5(pakfile);
		if (pakfile->downloadfile[0] == '\0')
		{
			Con_DPrintf("Download URL invalid!\n");
			return;
		}
		bSameFile = Check_MD5_Signatures(pakfile);

		if(!bSameFile)
		{
			if(!strstr(pakfile->downloadfile, ".exe") && !strstr(pakfile->downloadfile, ".EXE"))
			{
				Get_PAK(pakfile, false);
			}
			else
			{
				Get_PAK(pakfile, true);
			}
		}
	}
}

void Get_API_Version (pakfiles_t *pakfile)
{
	char url[MAX_URLLENGTH] = {0};
	int err = 0;
	int i = 0;
	filemirrors_t *mirror = NULL;

	if (!pakfile)
	{
		return;
	}

	for (i = 0; i < TOTAL_MIRRORS; i++)
	{
		memset((char *)pakfile->pakHttp_md5, 0, sizeof(pakfile->pakHttp_md5));

		Com_sprintf(url, sizeof(url), "%sDownload/GetAPIVersion", filemirrors[i].baseurl);
		CURL_HTTP_StartMD5Checksum_Download (url, pakfile->pakHttp_md5);
		err = Download_Loop();

		if (err == HTTP_MD5_DL_FAILED || atoi(pakfile->pakHttp_md5) < UPDATER_API_LEGACY || atoi(pakfile->pakHttp_md5) > UPDATER_API_VERSION1)
		{
			filemirrors[i].bFailed = true;
		}
		else
		{
			filemirrors[i].apiVersion = atoi(pakfile->pakHttp_md5);
			pakfile->pakHttp_md5[32] = '\0';
			availableMirrorsCnt++;
		}
	}

	if (availableMirrorsCnt == 0)
	{
		Con_Printf("No valid update server found.  Aborting!\n");
		Error_Shutdown();
		return;
	}

	availableMirrors = (filemirrors_t *)calloc(availableMirrorsCnt, sizeof(filemirrors_t));
	for (i = 0; i < TOTAL_MIRRORS; i++)
	{
		if (filemirrors[i].bFailed == false)
		{
			memcpy(availableMirrors, &filemirrors[i], sizeof(filemirrors_t));
			availableMirrors++;
		}
	}

	availableMirrors -= availableMirrorsCnt;
	headAvailableMirror = availableMirrors;
}

void ParseCommandLine (int argc, char **argv)
{
	int i = 0;
	int j = 0;

	for (i = 1; i < argc; i++) 
	{
		if (!_strnicmp(argv[i], "-debug", 6))
		{
			Debug = true;
		}

		if (!_strnicmp(argv[i], "-showfile", 9))
		{
			showfile = true;
		}

		if (!_strnicmp(argv[i], "-auto", 5))
		{
			skipPrompts = true;
		}

		if (!_strnicmp(argv[i], "-silent", 7))
		{
			skipPrompts = true;
			silent = true;
		}

		if (!_strnicmp(argv[i], "-nopak5", 7))
		{
			pakfiles[3].fileName = NULL;
		}

		if (!_strnicmp(argv[i], "-force", 6))
		{
			forceFailure = true;
		}

		if (!_strnicmp(argv[i], "-beta", 5))
		{
			beta = 1;
		}

		if (!_strnicmp(argv[i], "-listmirrors", 12))
		{
			Con_Printf("Available mirrors: \n\n");
			for (j = 0; j < TOTAL_MIRRORS; j++)
			{
				Con_Printf("%s\n", filemirrors[j].baseurl);
			}
			Con_Printf("\n");
			Error_Shutdown();
		}

		if (!_strnicmp(argv[i], "-setmirror", 10))
		{
			Con_Printf("Not yet implemented.\n");
			Error_Shutdown();
		}

		if (!_strnicmp(argv[i], "-help", 5) || !_strnicmp(argv[i], "-?", 2))
		{
			Con_Printf("Automatic updater for Daikatana v1.3 on the %s platform.\nAvailable paramters:\n \
				\n-debug to show verbose debugging output. \
				\n-showfile to show verbose output about MD5 comparisions. \
				\n-auto to skip prompts and update with no user intervention. \
				\n-silent for silent updates.  Implies -auto. \
				\n-nopak5 to skip checking pak5.pak (32-bit textures). \
				\n-force to force updates.  Useful for corrupt downloads. \
				\n-beta for beta updates. \
				\n-listmirrors for list of mirrors. \n\n", PLATFORM);
			Error_Shutdown();
		}
	}
}

int main(int argc, char **argv)
{
	int x = 0;

	ParseCommandLine(argc, argv);

#ifdef CHECK_HARDWARE_PROPERTIES
    /*	Verify unit32 is, in fact, a 32 bit data type.  */
    if (sizeof(uint32) != 4)
	{
		fprintf(stderr, "** Configuration error.  Setting for uint32 in file md5.h\n");
		fprintf(stderr, "   is incorrect.  This must be a 32 bit data type, but it\n");
		fprintf(stderr, "   is configured as a %d bit data type.\n", ((int) sizeof(uint32) * 8));
		return 2;
    }
    
    /*	If HIGHFIRST is not defined, verify that this machine is,
    	in fact, a little-endian architecture.  */
	
#ifndef HIGHFIRST
    {
		uint32 t = 0x12345678;
    
		if (*((char *) &t) != 0x78)
		{
			fprintf(stderr, "** Configuration error.  Setting for HIGHFIRST in file md5.h\n");
			fprintf(stderr, "   is incorrect.  This symbol has not been defined, yet this\n");
			fprintf(stderr, "   machine is a big-endian (most significant byte first in\n");
			fprintf(stderr, "   memory) architecture.  Please modify md5.h so HIGHFIRST is\n");
			fprintf(stderr, "   defined when building for this machine.\n");
			return 2;
		}
    }
#endif
#endif

    /*	Process command line options.  */
	Con_Printf("---------------------------------------------------------------\n");
	Con_Printf("\nDaikatana v1.3 Auto-Updater for %s.  Version %s\n\n", PLATFORM, VERSION);
	Con_Printf("---------------------------------------------------------------\n");
	NET_Init();
	CURL_HTTP_Init();

	Get_API_Version(&pakfiles[0]);

	x = 0;
	while (pakfiles[x].fileName != NULL) /* FS: Go through the pakfiles struct, check what we got */
	{
		if (x == 1)
			Sys_CheckBinaryType(&pakfiles[x]);

		Calc_MD5_File (&pakfiles[x]);
		Check_MD5_vs_Local(&pakfiles[x]);
		Con_Printf("\n\n---------------------------------------------------------------\n");
		x++;
	}

	Sys_Quit();

	return 0;
}

void Error_Shutdown(void)
{
	Con_Printf("Press any key to exit...");
	getch();
	Sys_Quit();
	exit(1);
}

qboolean Calc_MD5_File(pakfiles_t *pakfile)
{
	int j;
	FILE *in = stdin, *out = stdout;
	unsigned char buffer[16384], signature[16];
	struct MD5Context md5c;
	qboolean opened = false;
	    
	if ((in = fopen(pakfile->filepath, "rb")) == NULL)
	{
//		fprintf(stderr, "Cannot open input file %s\n", pakfile->filepath);
		pakfile->pakFileSignature[0] = '\0';
		return false;
	}
	opened = true;
#ifdef _WIN32

	    /** Warning!  On systems which distinguish text mode and
		binary I/O (MS-DOS, Macintosh, etc.) the modes in the open
        	statement for "in" should have forced the input file into
        	binary mode.  But what if we're reading from standard
		input?  Well, then we need to do a system-specific tweak
        	to make sure it's in binary mode.  While we're at it,
        	let's set the mode to binary regardless of however fopen
		set it.

		The following code, conditional on _WIN32, sets binary
		mode using the method prescribed by Microsoft Visual C 7.0
        	("Monkey C"); this may require modification if you're
		using a different compiler or release of Monkey C.	If
        	you're porting this code to a different system which
        	distinguishes text and binary files, you'll need to add
		the equivalent call for that system. */

	_setmode(_fileno(in), _O_BINARY);
#endif
    
	MD5Init(&md5c);
	while ((j = (int) fread(buffer, 1, sizeof buffer, in)) > 0)
	{
		MD5Update(&md5c, buffer, (unsigned) j);
	}
	    
	if (opened)
	{
		fclose(in);
	}
	MD5Final(signature, &md5c);

	for (j = 0; j < sizeof signature; j++)
	{
		pakfile->pakFileSignature[j] = signature[j];
		if(showfile)
		{
			Con_Printf(hexfmt, signature[j]);
		}
	}
	if (showfile)
	{
		Con_Printf("  %s", (in == stdin) ? "-" : pakfile->filepath);
		Con_Printf("\n");
	}

	return true;
}
