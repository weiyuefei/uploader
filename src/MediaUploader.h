
#ifndef __MEDIAUPLOADER_H__
#define __MEDIAUPLOADER_H__

//#include "Thread.h"
//#include "ev.h"
#include "Task.h"
//#include "ThreadPool.h"
//#include "OSTypeDef.h"
#include "HTTPRequest.h"

//#include <list>
//using std::list;

void *ProcessTask(void * theTask);

#if 0
class MediaUploader : public Thread {
public:

    MediaUploader();
    ~MediaUploader();

    void Initialize();

    void Entry();

    void AddTask(void *i_pData);

    void ReleaseResource()
    {
        delete async;
        ev_loop_destroy(loop);
        pthread_mutex_destroy(sLock);
        delete sLock;

        delete theTaskList;
    }

private:

    static list<Task *> *theTaskList;
    static pthread_mutex_t *sLock;

    struct ev_loop *loop;
    struct ev_async *async;
    UInt32 m_u32TaskNums;

    static void AsyncCallback(EV_P_ ev_async *w, int revents);

    friend class ThreadPool<MediaUploader>;
};
#endif

#endif
