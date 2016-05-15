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
    string url;           /*url of ftp*/
    string user_key;     /*username:password*/
    string file;        /*filepath*/
    string ip;
    int port;
    double uploadsize;
} FTP_OPT;

int get_file_size(const string &filepath, curl_off_t *filesize);
CURL *curl_init();
void curl_set_upload_opt(CURL *curl, const string url,
    const string user_key, int sockfd, curl_off_t uploadsize);
void curl_exit();
CURLcode curl_perform(CURL *curl);

/*upload file to ftp server*/
FTP_STATE ftp_upload(const FTP_OPT ftp_option);

#endif

