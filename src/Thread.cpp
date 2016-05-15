
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "Thread.h"

void Thread::Initialize()
{
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
    pthread_attr_init(&m_threadAttr);
    pthread_attr_setscope(&m_threadAttr, PTHREAD_SCOPE_SYSTEM);
#endif
}

Thread::Thread()
: m_b16Joined(false),
  m_b16StopRequested(false)
{
}

Thread::~Thread()
{
    //this->StopAndWaitForThread();
}

void Thread::Start()
{
    pthread_attr_t* theAttrP;
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
    theAttrP = 0;
#else
    theAttrP = NULL;
#endif

    int err = pthread_create((pthread_t *)&m_threadID, theAttrP, _Entry, (void*)this);
    assert(err == 0);
}

void Thread::StopAndWaitForThread()
{
    m_b16StopRequested = true;
    if (!m_b16Joined)
        Join();
}

void Thread::Join()
{
    assert(!m_b16Joined);
    m_b16Joined = true;

    void *retVal;
    pthread_join((pthread_t)m_threadID, &retVal);
}

void* Thread::_Entry(void *inThread)
{
    Thread* theThread = (Thread *)inThread;
    theThread->m_threadID = (pthread_t)pthread_self();

    // Run the thread
    theThread->Entry();
    return NULL;
}
