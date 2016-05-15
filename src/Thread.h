
#ifndef __THREAD_H__
#define __THREAD_H__

#include <errno.h>
#include <pthread.h>
#include "OSTypeDef.h"

class Thread {
public:

    // Call before any other Thread functions
    void Initialize();

    Thread();
    virtual ~Thread();

    // Devived class must implement the own Entry function
    virtual void Entry() = 0;
    void Start();

    void Join();
    void StopAndWaitForThread();

private:
    pthread_t m_threadID;
    Bool16 m_b16Joined;
    Bool16 m_b16StopRequested;

#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
    pthread_attr_t m_threadAttr;
#endif

    static void* _Entry(void* inThread);
};

#endif
