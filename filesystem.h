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
