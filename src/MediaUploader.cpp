
#include "MediaUploader.h"
#include "FtpManager.h"

#include "CoreCycle.h"

#include <pthread.h>
#include <assert.h>
#include <sstream>
using namespace std;

extern T_CoreCycle *gCoreCycle;

static int DoFtpUpload(Task *i_pTask);

void* ProcessTask(void* i_theTask)
{
    pthread_detach(pthread_self());

    Task *theTask = (Task *)i_theTask;

    // when DoFtpUpload returns, the task finishes.
    int status = DoFtpUpload(theTask);

    if (status == FTP_UPLOAD_SUCCESS)
    {
        LOG_INFO("Task id[%d] upload success: %s, status: %d",
                theTask->m_u32Taskid,
                theTask->m_strVideoUrl.c_str(),
                status);
    }
    else
    {
        LOG_ERROR("###### Task id[%d] upload failed: %s, status: %d",
                  theTask->m_u32Taskid,
                  theTask->m_strVideoUrl.c_str(),
                  status);
    }

    // Remove from the TaskSet
    gCoreCycle->taskSet.erase(theTask->m_u32Taskid);
    LOG_INFO("Task id[%d] upload success, remove from the task Set", theTask->m_u32Taskid);

    // when finishing uploading file, send message to notify upload success
    string strReport;

    {
        stringstream ss;
        strReport = "{ \"id\": ";
        ss.clear();
        ss << theTask->m_u32Taskid;
        strReport += ss.str() + string(", ")
                     + string("\"status\": ");
    }

    {
        stringstream ss;
        ss.clear();
        ss << status;
        strReport += ss.str() + string(", ")
                    + string("\"video_url\": ");
    }

    string strCDNVideoURL = string("http://");
    strCDNVideoURL += gCoreCycle->configSettings->Read(string("cdn_acceleration_addr"), string("cdn.sdmc.tv"));

    string::size_type pos = 0;
    if ( (pos = theTask->m_strVideoUrl.find("http://")) != string::npos )
    {
        pos += sizeof("http://") - 1;
    }
    else if ( pos = theTask->m_strVideoUrl.find("https://") != string::npos )
    {
        pos += sizeof("https://") - 1;
    }

    string::size_type posSlash = theTask->m_strVideoUrl.find("/", pos);
    strCDNVideoURL += theTask->m_strVideoUrl.substr(posSlash);

    strReport += string("\"");
    strReport += strCDNVideoURL + string("\"")
                    + string(" }");

    string postUrl = string("http://")
                   + gCoreCycle->configSettings->Read(string("web_server_ip_port"), string("10.10.121.13:80"))
                   + gCoreCycle->configSettings->Read(string("callback_uri"), string("/cdn/callback"));

    LOG_INFO("CDN upload callback, URL: %s, post data: %s", postUrl.c_str(), strReport.c_str());

    HTTPRequest theRequest;
    ERR_CODE eCode = theRequest.sendRequest(httpPostMethod, postUrl, strReport);
    if (eCode != ERR_NoErr)
    {
        LOG_ERROR("###### CDN upload callback post falied, task id: %d, post response: %s",
                theTask->m_u32Taskid,
                theRequest.body.c_str());
    }

    /* NOTE: Need to release the TASK memory allocated on the heap */
    delete theTask;
    theTask = NULL;

    pthread_exit(0);
}

static int DoFtpUpload(Task* i_pTask)
{
    FTP_OPT ftpOpt;

    string filesUpload[] =
    {
        i_pTask->m_strVideoUrl,
        i_pTask->m_strAlbumVerPic,
        i_pTask->m_strAlbumExcel
    };

    int size = sizeof(filesUpload) / sizeof(filesUpload[0]);
    for (int i = 0; i < size; i++)
    {
        if (filesUpload[i].length() == 0)
        {
            continue;
        }

        ftpOpt.file_location = filesUpload[i];
        ftpOpt.file_size = (off_t) i_pTask->m_u64FileSize;

        ftpOpt.ftp_url = gCoreCycle->configSettings->Read(string("ftp_server_ip"), string("0.0.0.0"));
        if (ftpOpt.file_location[0] == '/')
        {
            // local file
            ftpOpt.ftp_url += ftpOpt.file_location;
        }
        else
        {
            string::size_type posSlash, posQuestion;
            string strScheme = "";
            if (ftpOpt.file_location.find("http://") != string::npos)
            {
                strScheme = "http://";
            }
            else if (ftpOpt.file_location.find("https://") != string::npos)
            {
                strScheme = "https://";
            }

            int len = strScheme.length();
            posSlash = ftpOpt.file_location.find("/", len);
            posQuestion = ftpOpt.file_location.find("?", len);

            if (posSlash == string::npos)
            {
                LOG_ERROR("###### Invalid url:%s", ftpOpt.file_location.c_str());
                continue;
            }

            if (posQuestion == string::npos)
            {
                ftpOpt.ftp_url += ftpOpt.file_location.substr(posSlash);
            }
            else
            {
                ftpOpt.ftp_url += ftpOpt.file_location.substr(posSlash, posQuestion - posSlash);
            }

        }

        ftpOpt.user_key = gCoreCycle->configSettings->Read(string("ftp_server_user_passwd"), string("root:123456"));

        LOG_INFO("Ftp options: [\nfile location: %s\nfile size: %lu\nftp user key: %s\nftp url: %s\n]",
                ftpOpt.file_location.c_str(),
                ftpOpt.file_size,
                ftpOpt.user_key.c_str(),
                ftpOpt.ftp_url.c_str());

        FTP_STATE status = ftp_upload(ftpOpt);

        if (status != FTP_UPLOAD_SUCCESS)
        {
            LOG_ERROR("###### Upload\"%s\" failed", ftpOpt.file_location.c_str());
            return (int) status;
        }

        LOG_INFO("Upload \"%s\" success", ftpOpt.file_location.c_str());
    }

    // all files upload success
    return (int) FTP_UPLOAD_SUCCESS;
}

