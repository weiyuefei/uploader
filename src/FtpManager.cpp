/***********************************************************\
* Copyright (C) SDMC Co. ltd.
* Author: feeman
* File: FtpManager.cpp
\***********************************************************/

#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
using namespace std;

#include "FtpManager.h"
#include "HTTPRequest.h"
#include "Logger.h"

/* Get the file size of a local file */
static off_t get_file_size(const char *pFilePath)
{
	struct stat statbuff;
	if (stat(pFilePath, &statbuff) < 0)
    {
		return -1;
	}

    return statbuff.st_size;
}

size_t read_cb(char *buffer, size_t size, size_t nitems, void *instream)
{
    FILE_HANDLE *pFileHandle = (FILE_HANDLE *)instream;

    size_t numbytes;

    if (!pFileHandle->bIsLocal)
    {
        size_t MAX_DATA_SIZE = size * nitems;
        numbytes = recv(pFileHandle->fh.remote_fh, buffer, MAX_DATA_SIZE - 1, 0);
        if (numbytes == -1)
        {
            LOG_ERROR("###### The peer close thje connection");
            return 0; // stop the transfer
        }

        LOG_DEBUG("*** We read %d bytes from socket buffer", numbytes);
    }
    else
    {
        numbytes = fread(buffer, size, nitems, pFileHandle->fh.local_fh);

        LOG_DEBUG("*** We read %d bytes from file", numbytes);
    }

    return numbytes;
}

CURL *curl_init()
{
    //curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if(NULL == curl)
    {
        LOG_ERROR("###### curl_easy_init failed");
    }
    return curl;
}

void curl_set_upload_opt(CURL *curl, const string url,
                         const string user_key,
                         FILE_HANDLE *file_handle,
                         curl_off_t uploadsize)
{
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERPWD, user_key.c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    
    // use the default read callback function
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_cb);
    curl_easy_setopt(curl, CURLOPT_READDATA, file_handle);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);

    // for upload a big file larger than 2G, use CURLOPT_INFILESIZE_LARGE
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, uploadsize);
    curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
}

void curl_exit(CURL *curl)
{
    curl_easy_cleanup(curl);
    //curl_global_cleanup();
}

CURLcode curl_perform(CURL *curl)
{
    CURLcode ret = curl_easy_perform(curl);
    if (ret != 0)
    {
        LOG_ERROR("###### curl_easy_perform failed, error info: %s", curl_easy_strerror(ret));
    }
    return ret;
}

FTP_STATE ftp_upload(FTP_OPT ftp_option)
{
    FTP_STATE state;
    CURL *curl;
    FILE *fp;

    FILE_HANDLE *pFileHandle = new FILE_HANDLE;
    if (pFileHandle == NULL)
    {
        LOG_ERROR("###### out of memory");
        return FTP_UPLOAD_FAILED;
    }

    bool bIsLocal = ( (ftp_option.file_location[0] == '/') ? true : false );
    if (bIsLocal)
    {
        fp = fopen(ftp_option.file_location.c_str(), "r");
        if (NULL == fp)
        {
            delete pFileHandle;
            LOG_ERROR("###### Open uploading file failed");
            return FTP_UPLOAD_FAILED;
        }

        pFileHandle->fh.local_fh = fp;

        off_t file_size = get_file_size(ftp_option.file_location.c_str());
        if (file_size == -1)
        {
            LOG_WARN("****** Failed to get file \"%s\" size, use the original value: %lu",
                        ftp_option.file_location.c_str(), ftp_option.file_size);
        }
        else
        {
            LOG_INFO("Get file \"%s\" size: %lu",
                        ftp_option.file_location.c_str(),
                        file_size);

            ftp_option.file_size = file_size;
        }
    }
    else
    {
        HTTPRequest theRequest;
        bool success = theRequest.parse(ftp_option.file_location);
        if (!success)
        {
            delete pFileHandle;
            LOG_ERROR("###### Parse url error: %s", ftp_option.file_location.c_str());
            return FTP_UPLOAD_FAILED;
        }

        pFileHandle->fh.remote_fh = (SOCKET) theRequest.connectTo();
        if (pFileHandle->fh.remote_fh == -1)
        {
            delete pFileHandle;
            LOG_ERROR("###### connect failed");
            return FTP_UPLOAD_FAILED;
        }

        off_t content_length = theRequest.getContentLength();
        if (content_length == -1)
        {
            LOG_WARN("****** Failed to parse Content-Length, use the original value: %lu",
                    ftp_option.file_size);
        }
        else
        {
            ftp_option.file_size = content_length;
            LOG_INFO("Get the Content-Length: %lu", content_length);
        }

    }

    curl = curl_init();
    if (NULL == curl)
    {
        if (bIsLocal)
        {
            fclose(pFileHandle->fh.local_fh);
        }
        else
        {
            close(pFileHandle->fh.remote_fh);
        }

        delete pFileHandle;
        return FTP_UPLOAD_FAILED;
    }

    pFileHandle->bIsLocal = bIsLocal;
    curl_set_upload_opt(curl, ftp_option.ftp_url, ftp_option.user_key,
                        pFileHandle, (curl_off_t) ftp_option.file_size);
    if (CURLE_OK == curl_perform(curl))
    {
        state = FTP_UPLOAD_SUCCESS;
    }
    else
    {
        state = FTP_UPLOAD_FAILED;
    }

    curl_exit(curl);

    if (bIsLocal)
    {
        fclose(pFileHandle->fh.local_fh);
    }
    else
    {
        close(pFileHandle->fh.remote_fh);
    }
    delete pFileHandle;

    return state;
}
