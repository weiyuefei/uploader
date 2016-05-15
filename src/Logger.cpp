/*********************************************************\
* Copyright (C) SDMC Co. ltd.
* Author: feeman
* File: Logger.cpp
\*********************************************************/

#include "Logger.h"
#include <sys/file.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

Log_Writer LOG_W;

static const char *LogLevelString[] =
{
    "DEBUG",
    "INFO",
    "NOTICE",
    "WARN",
    "ERROR"
};

__thread char Log_Writer::m_buffer[_LOG_BUFFSIZE];

bool logger_init(LogLevel l, const char* p_modulename, const char* p_logdir)
{
	if (::access(p_logdir, 0) == -1)
	{
		if (::mkdir(p_logdir, S_IREAD | S_IWRITE ) < 0)
		{
			fprintf(stderr, "create folder failed\n");
            return false;
		}
	}

	char _location_str[_LOG_PATH_LEN];
	snprintf(_location_str, _LOG_PATH_LEN, "%s/uploader.log", p_logdir);
	LOG_W.loginit(l, _location_str);

	return true;
}

bool Log_Writer::loginit(LogLevel l, const  char *filelocation, bool append, bool issync)
{
	MACRO_RET(NULL != fp, false);
    m_sysLevel = l;
    m_isappend = append;
    m_issync = issync;

	if (strlen(filelocation) >= (sizeof(m_filelocation) -1))
	{
		fprintf(stderr, "the path of log file is too long:%d limit:%d\n", strlen(filelocation), sizeof(m_filelocation) -1);
		exit(0);
	}

	strncpy(m_filelocation, filelocation, sizeof(m_filelocation));
	m_filelocation[sizeof(m_filelocation) -1] = '\0';

	if ('\0' == m_filelocation[0])
	{
		fp = stdout;
		fprintf(stderr, "now all the running-information are going to put to stderr\n");
		return true;
	}

	fp = fopen(m_filelocation, append ? "a":"w");
	if (fp == NULL)
	{
		fprintf(stderr, "cannot open log file,file location is %s\n", m_filelocation);
		exit(0);
	}

	setvbuf (fp,  (char *)NULL, _IOLBF, 0);
	fprintf(stderr, "log info will write to %s\n", m_filelocation);
	return true;
}

int Log_Writer::premakestr(char* m_buffer, LogLevel l)
{
    time_t now;
	now = time(&now);
	struct tm vtm;
    localtime_r(&now, &vtm);
    return snprintf(m_buffer, _LOG_BUFFSIZE, "%04d/%02d/%02d %02d:%02d:%02d [%s]",
                    1900 + vtm.tm_year,
                    vtm.tm_mon + 1,
                    vtm.tm_mday,
                    vtm.tm_hour,
                    vtm.tm_min,
                    vtm.tm_sec,
                    LogLevelString[l - 1]);
}

bool Log_Writer::log(LogLevel l, const char* logformat,...)
{
	MACRO_RET(!checklevel(l), false);
	int _size;
	int prestrlen = 0;

	char * star = m_buffer;
	prestrlen = premakestr(star, l);
	star += prestrlen;

	va_list args;
	va_start(args, logformat);
	_size = vsnprintf(star, _LOG_BUFFSIZE - prestrlen, logformat, args);
	va_end(args);

	if (NULL == fp)
	{
		fprintf(stderr, "%s", m_buffer);
	}
	else
	{
		_write(m_buffer, prestrlen + _size);
	}

	return true;
}

bool Log_Writer::_write(char *_pbuffer, int len)
{
	if (0 != access(m_filelocation, W_OK))
	{
		pthread_mutex_lock(&m_mutex);

		if (0 != access(m_filelocation, W_OK))
		{
			logclose();
			loginit(m_sysLevel, m_filelocation, m_isappend, m_issync);
		}
		pthread_mutex_unlock(&m_mutex);
	}

	if (1 == fwrite(_pbuffer, len, 1, fp)) //only write 1 item
	{
		if (m_issync)
		{
          	fflush(fp);
		}

		*_pbuffer = '\0';
    }
    else
	{
        int x = errno;
	    fprintf(stderr, "Failed to write to logfile. errno:%s, message:%s", strerror(x), _pbuffer);
	    return false;
	}

	return true;
}

bool Log_Writer::logclose()
{
	if (fp == NULL)
	{
		return true;
	}

	fflush(fp);
	fclose(fp);
	fp = NULL;
	return true;
}
