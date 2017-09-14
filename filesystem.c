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
#include "shared.h"
#include "dk_essentials.h"
#include "dg_misc.h"
#include "filesystem.h"
#include "unzip.h"
#include "zip.h"

static long		bytes_read = 0;
static int		files_opened = 0; 

// Knightmare added 3/8/13
#define MAX_ZIPHANDLES 8

typedef struct {
	char		name[MAX_OSPATH+MAX_QPATH];
	fsMode_t	mode;
	unzFile		*readZip;
	zipFile		*writeZip;
} fs_zHandle_t;

fs_zHandle_t	fs_zHandles[MAX_ZIPHANDLES];
// end Knightmare

/*
================
FS_HandleForZip

Finds a free fs_zHandle_t
================
*/
fs_zHandle_t *FS_HandleForZip (const char *path, zipHandle_t *z)
{
	int i;
	fs_zHandle_t	*zHandle;

	zHandle = fs_zHandles;
	for (i = 0; i < MAX_ZIPHANDLES; i++, zHandle++)
	{
		if (!zHandle->readZip && !zHandle->writeZip)
		{
			strncpy(zHandle->name, path, sizeof(zHandle->name));
			*z = i +1;
			return zHandle;
		}
	}

	Con_Printf ("FS_HandleForZip: none free");
	return 0;
}

/*
================
FS_GetZipByHandle

Returns a fs_zHandle_t * for the given zipHandle_t
================
*/
fs_zHandle_t *FS_GetZipByHandle (zipHandle_t z)
{
	if (z <= 0 || z > MAX_ZIPHANDLES)
		Con_Printf ("FS_GetZipByHandlep: out of range");

	return &fs_zHandles[z-1];
}

/*
================
FS_FOpenCompressedFile

Returns file size or -1 if not found.
Opens files directly from inside a specified zip file.
================
*/
int FS_OpenCompressedFileRead (fs_zHandle_t *zHandle, const char *zipName, const char *fileName)
{
	unz_file_info	info;

	zHandle->readZip = (void **)unzOpen(zipName);
	if (zHandle->readZip)
	{
		if ( unzLocateFile(zHandle->readZip, fileName, 2) == UNZ_OK )
		{
			if ( unzOpenCurrentFile(zHandle->readZip) == UNZ_OK ) {
				Con_DPrintf("FS_OpenCompressedFileRead: %s (found in %s)\n", fileName, zipName);
				unzGetCurrentFileInfo(zHandle->readZip, &info, NULL, 0, NULL, 0, NULL, 0);
				return info.uncompressed_size;
			}
		}
		unzClose(zHandle->readZip);
	}

	// Not found!
	Con_DPrintf ("FS_OpenCompressedFileRead: couldn't find %s\n", zHandle->name);

	return -1;
}

/*
================
FS_FOpenCompressedFile

Opens a zip file for "mode".
Returns file size or -1 if an error occurs/not found.
Opens files directly from inside a specified zip file.
================
*/
int FS_FOpenCompressedFile (const char *zipName, const char *fileName, zipHandle_t *z, fsMode_t mode)
{
	fs_zHandle_t	*zHandle;
	char			name[MAX_OSPATH+MAX_QPATH];
	int				size = -1;

	Com_sprintf (name, sizeof(name), "%s/%s", zipName, fileName);
	zHandle = FS_HandleForZip (name, z);
	strncpy(zHandle->name, name, sizeof(zHandle->name));
	zHandle->mode = mode;

	switch (mode)
	{
	case FS_READ:
		size = FS_OpenCompressedFileRead(zHandle, zipName, fileName);
		break;
	default:
		Con_DPrintf("FS_FOpenCompressedFile: bad mode (%i)", mode);
	}

	if (size != -1) {
		files_opened++;
		return size;
	}

	// Couldn't open, so free the handle
	memset (zHandle, 0, sizeof(*zHandle));

	*z = 0;
	return -1;
}

/*
================
FS_FCloseCompressedFile

Closes a zip file.
================
*/
void FS_FCloseCompressedFile (zipHandle_t z)
{
	fs_zHandle_t	*zHandle;

	zHandle = FS_GetZipByHandle(z);

	if (zHandle->readZip) {
		unzCloseCurrentFile(zHandle->readZip);
		unzClose(zHandle->readZip);
	}
	else if (zHandle->writeZip) {
		zipCloseFileInZip(zHandle->writeZip);
		zipClose(zHandle->writeZip, NULL);
	}

	memset (zHandle, 0, sizeof(*zHandle));
}

/*
================
FS_ReadCompressed

Handles partial reads from zip files
================
*/
int FS_ReadCompressed (void *buffer, int size, zipHandle_t z)
{
	fs_zHandle_t	*zHandle;
	int				remaining, r;
	byte			*buf;

	zHandle = FS_GetZipByHandle(z);
	remaining = size;
	buf = (byte *)buffer;

	while (remaining)
	{
		if (zHandle->readZip)
			r = unzReadCurrentFile(zHandle->readZip, buf, remaining);
		else
			return 0;

		if (r == 0)
		{
			Con_DPrintf("FS_ReadCompressed: 0 bytes read from %s\n", zHandle->name);
			return size - remaining;
		}
		else if (r == -1)
		{
			Con_DPrintf ("FS_ReadCompressed: -1 bytes read from %s", zHandle->name);
		}

		bytes_read += r;
		remaining -= r;
		buf += r;
	}

	return size;
}

/*
================
FS_DecompressFile
================
*/
int FS_DecompressFile (const char *fileName, const char *zipName, const char *internalName)
{
	int			size, partSize;
	zipHandle_t	z;
	FILE		*fp;
	byte		buf[8192];

	size = FS_FOpenCompressedFile (zipName, internalName, &z, FS_READ);
	if (size == -1)
	{
		return -1;
	}

	fp = fopen (fileName, "wb");
	if (!fp) {
		FS_FCloseCompressedFile (z);
		return -1;
	}

	Con_Printf("Decompressing %s...\n", zipName);
	do {
		partSize = FS_ReadCompressed (&buf, sizeof(buf), z);
		if (partSize > 0)
			fwrite (&buf, 1, partSize, fp);
	} while (partSize > 0);

	fclose (fp);
	FS_FCloseCompressedFile (z);

	Con_Printf("Finished!\n");
	return size;
}
// end Knightmare
