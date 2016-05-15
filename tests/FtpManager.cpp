/***********************************************************\
* Copyright (C) SDMC Co. ltd.
* Author: feeman
* File: FtpManager.cpp
\***********************************************************/

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>
using namespace std;

#include "FtpManager.h"
int connect_to_server(const char *ip, int port);

/*
size_t read_callback(char *buffer, size_t size, size_t nitems, void *instream);

CURLcode curl_easy_setopt(CURL *handle, CURLOPT_READFUNCTION, read_callback);

*/

size_t read_cb(char *buffer, size_t size, size_t nitems, void *instream)
{
    int sockfd = *(int *)instream;
    // read from the socket
    curl_off_t nread;
    size_t MAX_DATA_SIZE = size * nitems;
    size_t numbytes = recv(sockfd, buffer, MAX_DATA_SIZE-1, 0);
    if (numbytes == -1)
    {
        printf("recv error\n");
        return 0; // stop the transfer
    }

    nread = (curl_off_t) numbytes;

    fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
          " bytes from file\n", nread);

    return numbytes;
}

int get_file_size(const string &filepath, curl_off_t *filesize)
{
    struct stat statbuff;
    if (stat(filepath.c_str(), &statbuff) < 0)
    {
        return -1;
    }

    if (filesize == NULL)
    {
        return -1;
    }

    *filesize = (curl_off_t) statbuff.st_size;

    return 0;
}

CURL *curl_init()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if(NULL == curl)
    {
        fprintf(stderr, "Init curl failed.\n");
    }
    return curl;
}

void curl_set_upload_opt(CURL *curl, const string url,
                         const string user_key,
                         int sockfd,
                         curl_off_t uploadsize)
{
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERPWD, user_key.c_str());
    // use the default read callback function
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_cb);
    curl_easy_setopt(curl, CURLOPT_READDATA, &sockfd);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);

    // for upload a big file larger than 2G, use CURLOPT_INFILESIZE_LARGE
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, uploadsize);
    curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
}

void curl_exit(CURL *curl)
{
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

CURLcode curl_perform(CURL *curl)
{
    CURLcode ret = curl_easy_perform(curl);
    if(ret != 0)
    {
        //fprintf(stderr, "Perform curl failed, error info: %s\n", curl_easy_strerror(ret));
        LOG_ERROR("###### curl_easy_perform failed, error info: %s", curl_easy_strerror(ret));
    }
    return ret;
}

FTP_STATE ftp_upload(const FTP_OPT ftp_option)
{
    FTP_STATE state;
    CURL *curl;
    //FILE *fp = fopen(ftp_option.file.c_str(), "r");
    int sockfd = connect_to_server(ftp_option.ip.c_str(), ftp_option.port);
    if (-1 == sockfd)
    {
        return FTP_UPLOAD_FAILED;
    }

    curl = curl_init();
    if (NULL == curl)
    {
        close(sockfd);
        return FTP_UPLOAD_FAILED;
    }
/*
    curl_off_t uploadsize;
    if (get_file_size(ftp_option.file, &uploadsize) == -1)
    {
        fclose(fp);
        return FTP_UPLOAD_FAILED;
    }
    */

    curl_set_upload_opt(curl, ftp_option.url, ftp_option.user_key, sockfd, (curl_off_t)ftp_option.uploadsize);
    if (CURLE_OK == curl_perform(curl))
    {
        state = FTP_UPLOAD_SUCCESS;
    }
    else
    {
        state = FTP_UPLOAD_FAILED;
    }

    curl_exit(curl);
    close(sockfd);
    return state;
}

int connect_to_server(const char *ip, int port)
{
    int sockfd;
    struct hostent *he;
    struct sockaddr_in their_addr; // connector'saddress information

    if ((he = gethostbyname(ip)) == NULL) { // get the host info

        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    bzero(&their_addr, sizeof(their_addr));

    their_addr.sin_family = AF_INET; // host byte order
    their_addr.sin_port = htons(port); // short, network byte order
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(their_addr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    printf("connect remote server success\n");

    return sockfd;
}
