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
#ifndef __HTTP_DL_H
#define __HTTP_DL_H

#define HTTP_OK 200
#define HTTP_REST 206
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404

void CURL_HTTP_Init (void);
void CURL_HTTP_Shutdown (void);
void CURL_HTTP_StartDownload (const char *url, char *filename);
void CURL_HTTP_StartMD5Checksum_Download (const char *url, void *stream);
int CURL_HTTP_Update (void);
void CURL_HTTP_Reset (void);

#define HTTP_MD5_DL_FAILED 666

typedef enum
{ none, md5, file, zipfile } curl_dltype_t;

#endif // __HTTP_DL_H
