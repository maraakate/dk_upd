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
#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

// Knightmare added 3/8/13
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND
} fsMode_t;

// Knightmare added 3/8/13
typedef int zipHandle_t;
int			FS_FOpenCompressedFile (const char *zipName, const char *fileName, zipHandle_t *z, fsMode_t mode);
void		FS_FCloseCompressedFile (zipHandle_t z);
int			FS_DecompressFile (const char *fileName, const char *zipName, const char *internalName);
int			FS_ReadCompressed (void *buffer, int size, zipHandle_t z);
int			FS_FReadCompressed (void *buffer, int size, int count, zipHandle_t z);

#endif // __FILESYSTEM_H
