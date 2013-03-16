#ifndef MUTEX_HPP
#define MUTEX_HPP

#ifndef WIN32
#include <pthread.h>
#else
#include <Windows.h>
#endif


class Mutex
{
private:

#ifndef WIN32
    pthread_mutex_t m_mtx;
#else
   HANDLE m_mtx;
#endif

public:
    Mutex()
    { 
#ifndef WIN32
     pthread_mutex_init(&m_mtx, NULL); 
#else
    m_mtx = CreateMutex(NULL, FALSE, NULL);            
#endif
      
    }

    ~Mutex()
    { 
#ifndef WIN32
          pthread_mutex_destroy(&m_mtx); 
#else
       CloseHandle(m_mtx);
#endif
    }

    void Lock()
    { 
#ifndef WIN32
       pthread_mutex_lock(&m_mtx); 
#else
       WaitForSingleObject(m_mtx, INFINITE);
#endif
    }

    void Unlock()
    { 
#ifndef WIN32
       pthread_mutex_unlock(&m_mtx); 
#else
       ReleaseMutex(m_mtx);
#endif
    }
};

class MutexLocker
{
private:
    Mutex &m_mtx;

public:
    MutexLocker(Mutex &mtx) : m_mtx(mtx)
    { mtx.Lock(); }

    virtual ~MutexLocker()
    { m_mtx.Unlock(); }
};

#endif
