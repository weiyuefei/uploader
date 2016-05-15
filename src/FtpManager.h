/******************************************************\
* Copyright (C) SDMC Co. ltd.
* Author: feeman
* File: FtpManager.h
\******************************************************/

#ifndef __FTP_MANAGER_H__
#define __FTP_MANAGER_H__

#include "curl/curl.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
using std::string;

/*FTP OPERATION CODE*/
typedef enum FTP_STATE
{
    FTP_UPLOAD_SUCCESS = 1,
    FTP_UPLOAD_FAILED  = 2
} FTP_STATE;

/*FTP OPERATIONS OPTIONS*/
typedef struct FTP_OPT
{
    string ftp_url;                 /*url of ftp*/
    string user_key;            /*username:password*/
    string file_location;       /*filepath*/
    off_t  file_size;
} FTP_OPT;

typedef int SOCKET;

typedef struct FILE_HANDLE
{
    union UNION_TAG
    {
        FILE   * local_fh; // take up 8 bytes
        SOCKET   remote_fh; // take up 4 bytes
    } fh;

    bool bIsLocal;
} FILE_HANDLE;


CURL *curl_init();
void curl_set_upload_opt(CURL *curl, const string url,
                        const string user_key, FILE_HANDLE *file_handle, curl_off_t uploadsize);
void curl_exit();
CURLcode curl_perform(CURL *curl);

/*upload file to ftp server*/
FTP_STATE ftp_upload(FTP_OPT ftp_option);

#endif

