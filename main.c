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

/*
  Daikatana Auto-Updater.

  Based on MD5.EXE by John Walker: http://www.fourmilab.ch/
*/

#define VERSION     "0.3a"

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

char dk_updName[10] = "dk_upd.exe";

typedef struct
{
	char *fileName;
	char *md5FileName;
	char downloadfile[MAX_QPATH];
	char *filepath;
	unsigned char pakFileSignature[16];
	char pakHttp_md5[HTTP_SIG_SIZE];
	const char *description;
}
pakfiles_t;

pakfiles_t pakfiles[]=
{
	{dk_updName, "dk_upd.md5", "", dk_updName, "", "", "Daikatana v1.3 Auto-Updater"},
	{"daikatana.exe", "dk_"__PLATFORM_EXT__".md5", "", "daikatana.exe", "", "", "Daikatana v1.3 Binary"}, /* FS: Keep this before the PAKs because the latest build should have the latest pak4.pak */
	{"pak4.pak", "pak4.md5", "pak4.pak", "data/pak4.pak", "", "", "Widescreen HUD, Script Fixes, etc."},
	{"pak6.pak", "pak6.md5", "pak6.zip", "data/pak6.pak", "", "", "Map Updates (Recommended)"},
	{"pak5.pak", "pak5.md5", "pak5.zip", "data/pak5.pak", "", "", "32-bit Textures (Optional)"},
	0
};

/* FS: Runtime Vars */
qboolean Debug = false;
qboolean showfile = false;
qboolean silent = false;
qboolean skipPrompts = false;
qboolean forceFailure = false;
char *hexfmt = "%02x";

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

	Com_sprintf(url, sizeof(url), "http://dk.toastednet.org/DK13/%s", pakfile->md5FileName);
	CURL_HTTP_StartMD5Checksum_Download (url, pakfile->pakHttp_md5);
	err = Download_Loop();

	if(err == HTTP_MD5_DL_FAILED)
	{
		memset((char *)pakfile->pakHttp_md5, 0, sizeof(pakfile->pakHttp_md5));
	}
	else
	{
		pakfile->pakHttp_md5[32] = '\0';
		Get_HTTP_Binary_Link(pakfile);
	}
}

void Get_HTTP_Binary_Link(pakfiles_t *pakfile)
{
	char url[MAX_URLLENGTH];
	char fixedDownloadName[MAX_OSPATH];

	int err = 0;

	if(!strstr(pakfile->fileName, ".exe") && !strstr(pakfile->fileName, ".EXE"))
		return;

	COM_StripExtension(pakfile->md5FileName, fixedDownloadName);

	Com_sprintf(url, sizeof(url), "http://dk.toastednet.org/DK13/%s.txt", fixedDownloadName);
	CURL_HTTP_StartMD5Checksum_Download (url, pakfile->downloadfile);
	err = Download_Loop();

	if(err == HTTP_MD5_DL_FAILED)
	{
		memset((char *)pakfile->downloadfile, 0, sizeof(pakfile->downloadfile));
	}
	else
	{
		int x = 0;

		for(x = 0; x < sizeof(pakfile->downloadfile); x++)
		{
			if((pakfile->downloadfile[x] == ' ') || (pakfile->downloadfile[x] == '\n')) /* FS: Echo in Windows command prompt adds a space and newline and fudges this up */
			{
				pakfile->downloadfile[x] = '\0';
				break;
			}
		}
	}
}

qboolean Check_MD5_Signatures (pakfiles_t *pakfile)
{
	int i;
	char convertedSignature[33];

	for(i = 0; i<16; i++)
	{
		sprintf(convertedSignature+i*2, "%02x", pakfile->pakFileSignature[i]);
	}

	if(pakfile->pakHttp_md5[0] == '\0')
	{
		Con_Printf("No HTTP Signature!\n");
		return true;
	}
	else
	{
		if(!forceFailure && !stricmp(convertedSignature, pakfile->pakHttp_md5))
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

	if(pakfile->description && pakfile->description[0] != '\0')
	{
		Con_Printf("\nFile Description: %s\n", pakfile->description);
	}

	if (pakfile->downloadfile[0] == '\0')
	{
		Con_DPrintf("Download URL invalid!\n");
		return;
	}

	Con_Printf("Do you want download %s? y/n", pakfile->downloadfile);

	if(!skipPrompts)
	{
		c = getch(); /* FS: FIXME: May not be portable */
	}

	Con_Printf("\n");

	if(skipPrompts || c == 'y' || c == 'Y')
	{
		char url[MAX_URLLENGTH];
		char fileName[MAX_QPATH];

		if(!binary)
		{
			Sys_Mkdir("data");
		}

		Com_sprintf(url, sizeof(url), "http://dk.toastednet.org/DK13/%s", pakfile->downloadfile);
		if(!binary)
		{
			Com_sprintf(fileName, sizeof(fileName), "data/%s", pakfile->downloadfile);
		}
		else
		{
			Com_sprintf(fileName, sizeof(fileName), "%s", pakfile->downloadfile);
		}

		CURL_HTTP_StartDownload(url, fileName);

		Download_Loop();
	}
}

void Check_MD5_vs_Local (pakfiles_t *pakfile)
{
	Con_Printf("%s: ", pakfile->fileName);

	if(pakfile->pakFileSignature[0] == 0)
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

void ParseCommandLine (int argc, char **argv)
{
	int i = 0;

	for (i = 1; i < argc; i++) 
	{
		if(!_strnicmp(argv[i], "-debug", 6))
		{
			Debug = true;
		}

		if(!_strnicmp(argv[i], "-showfile", 9))
		{
			showfile = true;
		}

		if(!_strnicmp(argv[i], "-auto", 5))
		{
			skipPrompts = true;
		}

		if(!_strnicmp(argv[i], "-silent", 7))
		{
			skipPrompts = true;
			silent = true;
		}

		if(!_strnicmp(argv[i], "-nopak5", 7))
		{
			pakfiles[3].fileName = NULL;
		}

		if(!_strnicmp(argv[i], "-force", 6))
		{
			forceFailure = true;
		}

		if(!_strnicmp(argv[i], "-help", 5) || !_strnicmp(argv[i], "-?", 2))
		{
			Con_Printf("Automatic updater for Daikatana v1.3 on the %s platform.\nAvailable paramters:\n \
				\n-debug to show verbose debugging output. \
				\n-showfile to show verbose output about MD5 comparisions. \
				\n-auto to skip prompts and update with no user intervention. \
				\n-silent for silent updates.  Implies -auto. \
				\n-nopak5 to skip checking pak5.pak (32-bit textures). \
				\n-force to force updates.  Useful for corrupt downloads.\n", PLATFORM);
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

	while(pakfiles[x].fileName != NULL) /* FS: Go through the pakfiles struct, check what we got */
	{
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
