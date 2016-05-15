/*******************************************************\
* CopyRight (C) SDMC Tech Co. ltd.
* File: main.cpp
* Description: The main entry of uploader
* Author: feeman
\*******************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <assert.h>

#include "MediaUploader.h"
#include "Config.h"
#include "CheckUpdate.h"
#include "CoreCycle.h"
#include "version.h"

#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::endl;
using std::cerr;

T_CoreCycle *gCoreCycle;

static int show_help;
static int show_version;
static int show_configure;
static char *conf_file = NULL;

#define DEFAULT_PREFIX      "/home/uploader"
#define DEFAULT_CONF_PATH   "/home/uploader/conf/uploader.conf"
#define DEFAULT_LOG_PATH    "/home/uploader/logs"
#define DEFAULT_LOG_LEVEL   "error"

static int get_options(int argc, char *const *argv);
static void show_version_info();
static bool log_init();

int main(int argc, char **argv)
{
    T_CoreCycle *cycle;

    if (get_options(argc, argv) != 0)
    {
        LOG_ERROR("###### get_options faile");
        return 1;
    }

    if (show_help || show_version)
    {
        show_version_info();
        return 0;
    }

    cycle = new T_CoreCycle;
    if (cycle == NULL)
    {
        LOG_ERROR("###### Out of memory");
        return 0;
    }

    gCoreCycle = cycle; /* Important: You MUST assign here instead of before while */
                        /* or else it will cause coredump. */

    if (conf_file == NULL)
    {
        cycle->strConfigPath.assign(DEFAULT_CONF_PATH);
    }
    else
    {
        cycle->strConfigPath.assign(conf_file);
    }

    // paser the config file
    Config *configSettings = new Config(cycle->strConfigPath);
    if (configSettings == NULL)
    {
        LOG_ERROR("###### Out of memory");
        return -1;
    }
    cycle->configSettings = configSettings;
    LOG_INFO("Read configs success");

    if (show_configure)
    {
        configSettings->ShowConfigSettings();
        delete configSettings;
        configSettings = NULL;
        delete cycle;
        cycle = NULL;
        exit(0);
    }

    if (!log_init())
    {
        LOG_ERROR("Logger init failed, exit");
        exit(0);
    }

#if __solaris__ || __linux__ || __hpux__

    struct rlimit rl;

    // set it to the absolute maximum that
    // the operating system allows.
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;

    setrlimit (RLIMIT_NOFILE, &rl);

    LOG_INFO("Set resource limit to unlimited");
#endif

    // start the update checking thread
    CheckUpdate mainThread;
    mainThread.Initialize();
    mainThread.Start();
    LOG_INFO("Start the timer thread");

    /* wait the thread starting up */
    ::sleep(1);

    LOG_INFO("MediaFileUploader starts successfully");

    while (1)
    {
        ::sleep(1000);
    }

    delete cycle;
    cycle = NULL;
    delete configSettings;
    configSettings = NULL;

    return 0;
}


static int get_options(int argc, char *const *argv)
{
    char *p;
    int   i;

    for (i = 1; i < argc; i++)
    {
        p = argv[i]; // p points to the string

        if (*p++ != '-')
        {
            LOG_ERROR("###### Invalid option: \"%s\"", argv[i]);
            return -1;
        }

        while (*p)
        {
            switch (*p++)
            {

            case '?':
            case 'h':
                show_version = 1;
                show_help = 1;
                break;

            case 'v':
                show_version = 1;
                break;

            case 'V':
                show_configure = 1;
                break;

            case 'c':
                if (*p)
                {
                    conf_file = p;
                    goto next;
                }

                if (argv[++i])
                {
                    conf_file = argv[i];
                    goto next;
                }

                LOG_ERROR("###### option \"-c\" requires file name");
                return -1;

            default:
                LOG_ERROR("##### Invalid option: \"%c\"", *(p - 1));
                return -1;
            }
        }

        next:

            continue;
    }

    return 0;
}

static void show_version_info()
{
    cerr << "MediaFileUploader Version: " << UPLOADER_VERSION << ", Build date: " << UPLOADER_BUILD_DATE << endl;

    if (show_help)
    {
        cerr <<
            "Usage: MediaFileUploader [-?hv] [-c filename] " << endl << endl;
        cerr << "Options:" << endl;
        cerr << "  -?,-h         : this help" << endl;
        cerr << "  -v            : show version and exit" << endl;
        cerr << "  -V            : show version and configure options then exit"
             << endl;
        cerr << "  -c filename   : set configuration file (default: " << DEFAULT_CONF_PATH
             << ")"  << endl;
    }
}

static bool log_init()
{
    assert(gCoreCycle != NULL);
    assert(gCoreCycle->configSettings != NULL);

    string strLogLevel =
        gCoreCycle->configSettings->Read(string("log_level"), string(DEFAULT_LOG_LEVEL));
    string strLogPath =
        gCoreCycle->configSettings->Read(string("log_path"), string(DEFAULT_LOG_PATH));

    LogLevel level;
    if (strLogLevel.compare("debug") == 0)
    {
        level = LL_DEBUG;
    }
    else if (strLogLevel.compare("notice") == 0)
    {
        level = LL_NOTICE;
    }
    else if (strLogLevel.compare("info") == 0)
    {
        level = LL_INFO;
    }
    else if (strLogLevel.compare("warn") == 0)
    {
        level = LL_WARN;
    }
    else if (strLogLevel.compare("error") == 0)
    {
        level = LL_ERROR;
    }
    else
    {
        LOG_ERROR("###### Illegal log level: %s", strLogLevel.c_str());
        return false;
    }

    return logger_init(level, NULL, strLogPath.c_str());
}
