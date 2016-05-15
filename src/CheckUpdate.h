/**********************************************\
* CopyRight @ SDMC Co. ltd.
* Author: feeman
* File: CheckUpdate.h
*
\**********************************************/

#ifndef __CHECKUPDATE_H__
#define __CHECKUPDATE_H__

#include "CoreCycle.h"

#include <string>
using std::string;

#define JSON_KEY_RESULTCODE     "resultCode"
#define JSON_KEY_DESCRIPTION    "description"
#define JSON_KEY_TOTAL          "total"
#define JSON_KEY_DATALIST       "dataList"

#define JSON_KEY_ID             "id"
#define JSON_KEY_ALBUM_NAME     "album_name"
#define JSON_KEY_VIDEO_NAME     "video_name"
#define JSON_KEY_PRIORITY       "priority"
#define JSON_KEY_FILESIZE       "filesize"
#define JSON_KEY_VIDEO_URL      "video_url"
#define JSON_KEY_ALBUM_VERPIC   "album_verpic"
#define JSON_KEY_ALBUM_EXCEL    "album_excel"


class CheckUpdate : public Thread {
public:
    CheckUpdate();
    ~CheckUpdate();

    void Initialize();

    void Entry();

private:

    static struct ev_loop *loop;
    static struct ev_timer checkTimer;

    static string sQueryUrl;
    static void timer_cb(EV_P_ ev_timer *w, int revents);
};

#endif // __CHECKUPDATE_H__
