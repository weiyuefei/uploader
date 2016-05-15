/***********************************************\
* Copyright (C) SDMC Co. ltd.
* Author: frrman
* File: CheckUpdate.cpp
\***********************************************/

#include "CheckUpdate.h"

#include <assert.h>
#include <iostream>
#include <sstream>
using namespace std;

extern T_CoreCycle *gCoreCycle;

static void ProcessBody(string body);
static bool GetJsonIntValue(const Json::Value &value, int *outValue);
static bool GetJsonDoubleValue(const Json::Value &value, double *outValue);

// ** Make the init action finished in Initialize **
// ** function, or else it will cause coredump **
// ** for the reason doesn't know yet. **
// updated by wyf: the real reason is that gCoreCycle
// is NULL, while we used it without a check.
string CheckUpdate::sQueryUrl;

struct ev_loop* CheckUpdate::loop;
struct ev_timer CheckUpdate::checkTimer;

CheckUpdate::CheckUpdate() {}
CheckUpdate::~CheckUpdate() {}

void CheckUpdate::Initialize()
{
    Thread::Initialize();

    loop = ev_default_loop(0);
    assert(loop != NULL);

    sQueryUrl = "http://";
    sQueryUrl += gCoreCycle->configSettings->Read(string("web_server_ip_port"), string("www.test.com"));
    sQueryUrl += gCoreCycle->configSettings->Read(string("check_update_uri"), string("/cdn/task"));
}

/*
This is the easiest way, and involves using ev_timer_again instead of ev_timer_start.

To implement this, configure an ev_timer with a repeat value of 60 and then call ev_timer_again
at start and each time you successfully read or write some data. If you go into an idle state
where you do not expect data to travel on the socket, you can ev_timer_stop the timer, and
ev_timer_again will automatically restart it if need be.

That means you can ignore both the ev_timer_start function and the after argument to ev_timer_set,
and only ever use the repeat member and ev_timer_again.

At start:

   ev_init (timer, callback);
   timer->repeat = 60.;
   ev_timer_again (loop, timer);

Each time there is some activity:

   ev_timer_again (loop, timer);

It is even possible to change the time-out on the fly, regardless of whether the watcher is active or not:

   timer->repeat = 30.;
   ev_timer_again (loop, timer);

This is slightly more efficient then stopping/starting the timer each time you want to modify its
timeout value, as libev does not have to completely remove and re-insert the timer from/into its
internal data structure.

It is, however, even simpler than the "obvious" way to do it.
*/

void CheckUpdate::Entry()
{
    ev_init(&checkTimer, timer_cb);

    float timeout = gCoreCycle->configSettings->Read("check_update_timeout", 60.0);

    LOG_INFO("CheckUpdate timeout value: %f", timeout);

#ifdef DEBUG
    cerr << "check update timeout: " << timeout << endl;
#endif

    checkTimer.repeat = timeout;
    ev_timer_again(loop, &checkTimer);

    ev_run(loop, 0);
}

void CheckUpdate::timer_cb(EV_P_ ev_timer * w, int revents)
{
    // need to know the query url
    HTTPRequest theRequest;
    string strBody;

    LOG_INFO("Send query post request, post url: %s", sQueryUrl.c_str());

    // invalid use of member 'm_strQueryUrl' in a static member function
    ERR_CODE retcode = theRequest.sendRequest(httpPostMethod, sQueryUrl);
    if (retcode != ERR_NoErr)
    {
        LOG_ERROR("###### Query failed, url: %s", sQueryUrl.c_str());
        goto next;
    }

    strBody = theRequest.body;

    LOG_INFO("Query response: [%s]", strBody.c_str());

    /* The body is a json string as the following:

    {
        "resultCode": "0", --int
        "description": "成功", -- string
        "total": "1000", -- int
        "dataList": [
            {
                "id": "1",
                "album_name": "老炮儿",
                "video_name": "老炮儿",
                "priority": "1",
                "filesize": "201455555",
                "video_url": "http://cdn.sdmc.tv/dianying/laopaoer.ts",
                "album_verpic": "http: //cdn.sdmc.tv/dianying/laopaoer.jpg",
                "album_excel": "http: //cdn.sdmc.tv/dianying/laopaoer.xls"
            },
            {
                "id": "2",
                "album_name": "老炮儿",
                "video_name": "老炮儿",
                "priority": "1",
                "filesize": "201455555",
                "video_url": "http: //cdn.sdmc.tv/dianying/laopaoer.ts",
                "album_verpic": "http: //cdn.sdmc.tv/dianying/laopaoer.jpg",
                "album_excel": "http: //cdn.sdmc.tv/dianying/laopaoer.xls"
            }
        ]
    }

    */

    ProcessBody(strBody);

next:
    ev_timer_again(loop, &checkTimer);
}

static void ProcessBody(string body)
{
    Json::Reader reader;
    Json::Value value;
    if (!reader.parse(body, value))
    {
        LOG_ERROR("###### Json parsed failed");
        return;
    }

    int resultCode;
    bool success = GetJsonIntValue(value[JSON_KEY_RESULTCODE], &resultCode);
    if (!success)
    {
        LOG_ERROR("###### Json get \"resultCode\"failed");
        return;
    }

    int total;
    success = GetJsonIntValue(value[JSON_KEY_TOTAL], &total);
    if (!success)
    {
        LOG_ERROR("###### Json get \"total\" failed");
        return;
    }

    if (total < 0)
    {
        LOG_ERROR("###### Illegal \"total\": %d, which is less than 0", total);
        return;
    }

    if (total == 0)
    {
        LOG_INFO("Json get total=0, which means no updates.");
        return;
    }

    // parse array
    const Json::Value dataList = value[JSON_KEY_DATALIST];
    assert(total == dataList.size());

    for (int i = 0; i < total; i++)
    {
        int taskID, priority;
        double fileSize;

        if (!GetJsonIntValue(dataList[i][JSON_KEY_ID], &taskID))
        {
            LOG_ERROR("###### Json get \"id\" failed");
            return;
        }

        // If the task is in process, don't add to task list any more
        if (gCoreCycle->taskSet.count(taskID) != 0)
        {
            LOG_INFO("****** Task id[%d] is already in the uploading process", taskID);
            continue;
        }

        if (!GetJsonIntValue(dataList[i][JSON_KEY_PRIORITY], &priority))
        {
            LOG_ERROR("###### Json get \"priority\" failed");
            return;
        }
        if (!GetJsonDoubleValue(dataList[i][JSON_KEY_FILESIZE], &fileSize))
        {
            LOG_ERROR("###### Json get \"filesize\" failed");
            return;
        }

        Task *theTask = new Task; /* NOTE: it is released by the task thread */
        if (theTask == NULL)
        {
            LOG_ERROR("###### Out of memory");
            return;
        }

        theTask->m_u32Taskid = (UInt32) taskID;
        theTask->m_strVideoUrl = dataList[i][JSON_KEY_VIDEO_URL].asString();
        theTask->m_u32Priority = (UInt32) priority;
        theTask->m_strVideoName = dataList[i][JSON_KEY_VIDEO_NAME].asString();
        theTask->m_u64FileSize = (UInt64) fileSize;
        theTask->m_strAlbumName = dataList[i][JSON_KEY_ALBUM_NAME].asString();

        if (!dataList[i][JSON_KEY_ALBUM_VERPIC].isNull())
        {
            theTask->m_strAlbumVerPic = dataList[i][JSON_KEY_ALBUM_VERPIC].asString();

        }

        if (!dataList[i][JSON_KEY_ALBUM_EXCEL].isNull())
        {
            theTask->m_strAlbumExcel = dataList[i][JSON_KEY_ALBUM_EXCEL].asString();
        }

        LOG_INFO("Create upload task, task info: [\ntask id=%d\npriority=%d\nfilesize=%lu\nvideo url=%s\nvideo name=%s\nAlbum name=%s\n]",
                 theTask->m_u32Taskid,
                 theTask->m_u32Priority,
                 theTask->m_u64FileSize,
                 theTask->m_strVideoUrl.c_str(),
                 theTask->m_strVideoName.c_str(),
                 theTask->m_strAlbumName.c_str());

        gCoreCycle->taskSet.insert(theTask->m_u32Taskid);
        LOG_INFO("Add task id[%d] to Task Set", theTask->m_u32Taskid);

        pthread_t pid;
        pthread_create(&pid, NULL, ProcessTask, theTask);

    }
}

static bool GetJsonIntValue(const Json::Value &value, int *outValue)
{
    Json::ValueType eValueType = value.type();
    int result = 0;
    bool succ = true;

    string strResult;
    istringstream iss;

    switch (eValueType)
    {
        case Json::stringValue:
            strResult = value.asString();
            iss.clear();
            iss.str(strResult);
            iss >> result;

            break;

        case Json::intValue:
            result = value.asInt();
            break;

        default:
            LOG_ERROR("###### Invalid value type: %d", eValueType);
            succ = false;
            break;
    }

    if (succ)
    {
        *outValue = result;
    }

    return succ;
}


static bool GetJsonDoubleValue(const Json::Value &value, double *outValue)
{
    Json::ValueType eValueType = value.type();
    double result = 0;
    bool succ = true;

    string strResult;
    istringstream iss;

    switch (eValueType)
    {
        case Json::stringValue:
            strResult = value.asString();
            iss.clear();
            iss.str(strResult);
            iss >> result;

            break;

        case Json::realValue:
            result = value.asDouble();
            break;

        case Json::intValue:
            result = (double) value.asInt();
            break;

        default:
            LOG_ERROR("###### Invalid json value type: %d", eValueType);
            succ = false;
            break;
    }

    if (succ)
    {
        *outValue = result;
    }

    return succ;
}

