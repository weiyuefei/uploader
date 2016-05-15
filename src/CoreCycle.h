#ifndef __CORECYCLE_H__
#define __CORECYCLE_H__

#include "Config.h"
#include "MediaUploader.h"
//#include "CheckUpdate.h"
#include "FtpManager.h"
#include "HTTPRequest.h"
#include "OSTypeDef.h"
#include "Thread.h"
#include "Task.h"
#include "Logger.h"

#include "ev.h"
#include "json/json.h"
#include "curl/curl.h"

#include <string>
using std::string;
#include <set>
using std::set;

struct CoreCycle
{
    string strConfigPath;
    Config *configSettings;
    set<int> taskSet;

    CoreCycle()
    : strConfigPath(""),
      configSettings(NULL) { }
};

typedef CoreCycle T_CoreCycle;

#endif
