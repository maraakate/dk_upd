#ifndef __HTTP_DL_H
#define __HTTP_DL_H

#define HTTP_OK 200
#define HTTP_REST 206
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404

void CURL_HTTP_Init (void);
void CURL_HTTP_Shutdown (void);
int CURL_HTTP_StartDownload (const char *url, char *filename);
void CURL_HTTP_StartMD5Checksum_Download (const char *url, void *stream);
int CURL_HTTP_Update (void);
void CURL_HTTP_Reset (void);

#define HTTP_MD5_DL_FAILED 666

typedef enum
{ none, md5, file, zipfile } curl_dltype_t;

#endif // __HTTP_DL_H
