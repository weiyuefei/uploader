/**************************************************\
* Copyright (C) SDMC Co. ltd.
* Author: feeman
* File: Logger.h
\**************************************************/

#ifndef   __LOGGER_H__
#define   __LOGGER_H__

#include <stdio.h>
#include <pthread.h>

#define   _LOG_BUFFSIZE     1024*1024*4
#define   _SYS_BUFFSIZE     1024*1024*8
#define	  _LOG_PATH_LEN     250
#define   _LOG_MODULE_LEN   32

typedef enum LogLevel
{
	LL_DEBUG        = 1,
	LL_INFO         = 2,
	LL_NOTICE       = 3,
	LL_WARN         = 4,
	LL_ERROR        = 5
} LogLevel;

class Log_Writer
{
public:
	Log_Writer()
	{
		m_sysLevel = LL_NOTICE;
		fp = NULL;
		m_issync = false;
		m_isappend = true;
		m_filelocation[0] = '\0';
		pthread_mutex_init(&m_mutex, NULL);
	}

	~Log_Writer()
    {
		logclose();
	}

	bool loginit(LogLevel l, const char *filelocation, bool append = true, bool issync = false);
	bool log(LogLevel l, const char *logformat,...);

    LogLevel get_level()
	{
        return m_sysLevel;
	}

	bool logclose();

private:

    bool checklevel(LogLevel l)
	{
        return (l >= m_sysLevel);
	}

	int premakestr(char* m_buffer, LogLevel l);
	bool _write(char *_pbuffer, int len);

private:

	LogLevel m_sysLevel;
	FILE* fp;
	bool m_issync;
	bool m_isappend;
	char m_filelocation[_LOG_PATH_LEN];
	pthread_mutex_t m_mutex;
	static __thread char m_buffer[_LOG_BUFFSIZE];
};

extern Log_Writer LOG_W;

bool logger_init(LogLevel l, const char* p_modulename, const char* p_logdir);


// Some useful macro

#define LOG_ERROR(log_fmt, log_arg...) \
    do { \
        LOG_W.log(LL_ERROR,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_WARN(log_fmt, log_arg...) \
    do { \
        LOG_W.log(LL_WARN,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_NOTICE(log_fmt, log_arg...) \
    do{ \
        LOG_W.log(LL_NOTICE,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_INFO(log_fmt, log_arg...) \
    do{ \
        LOG_W.log(LL_INFO,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_DEBUG(log_fmt, log_arg...) \
    do{ \
        LOG_W.log(LL_DEBUG,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)


#define MACRO_RET(condition, return_val) {\
    if (condition) {\
        return return_val;\
    }\
}

#define MACRO_WARN(condition, log_fmt, log_arg...) {\
    if (condition) {\
        LOG_WARN( log_fmt, ##log_arg);\
    }\
}

#define MACRO_WARN_RET(condition, return_val, log_fmt, log_arg...) {\
    if ((condition)) {\
        LOG_WARN( log_fmt, ##log_arg);\
		return return_val;\
    }\
}

#endif
