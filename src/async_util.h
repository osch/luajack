#ifndef LUAJACK_ASYNC_UTIL_H
#define LUAJACK_ASYNC_UTIL_H

/////////////////////////////////////////////////////////////////////////////////////////////

#if    defined(LUAJACK_ASYNC_USE_WIN32) \
    && (   defined(LUAJACK_ASYNC_USE_APPLE) \
        || defined(LUAJACK_ASYNC_USE_GNU))
  #error "LUAJACK_ASYNC: Invalid compile flag combination"
#endif
#if    defined(LUAJACK_ASYNC_USE_APPLE) \
    && (   defined(LUAJACK_ASYNC_USE_WIN32) \
        || defined(LUAJACK_ASYNC_USE_GNU))
  #error "LUAJACK_ASYNC: Invalid compile flag combination"
#endif
#if    defined(LUAJACK_ASYNC_USE_GNU) \
    && (   defined(LUAJACK_ASYNC_USE_WIN32) \
        || defined(LUAJACK_ASYNC_USE_APPLE))
  #error "LUAJACK_ASYNC: Invalid compile flag combination"
#endif
 
/////////////////////////////////////////////////////////////////////////////////////////////

#if    !defined(LUAJACK_ASYNC_USE_WIN32) \
    && !defined(LUAJACK_ASYNC_USE_APPLE) \
    && !defined(LUAJACK_ASYNC_USE_GNU)

    #if defined(WIN32)
        #define LUAJACK_ASYNC_USE_WIN32
    #elif defined(__APPLE__)
        #define LUAJACK_ASYNC_USE_APPLE
    #elif defined(__GNUC__)
        #define LUAJACK_ASYNC_USE_GNU
    #else
        #error "LUAJACK_ASYNC: unknown platform"
    #endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef LUAJACK_ASYNC_USE_WIN32
    #define LUAJACK_ASYNC_USE_WINTHREAD
#else
    #define LUAJACK_ASYNC_USE_PTHREAD
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

#if defined(LUAJACK_ASYNC_USE_WIN32) || defined(LUAJACK_ASYNC_USE_WINTHREAD)
    #include <windows.h>
#endif
#if defined(LUAJACK_ASYNC_USE_APPLE)
    #include <libkern/OSAtomic.h>
#endif
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    #include <errno.h>
    #include <sys/time.h>
    #include <pthread.h>
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

typedef int bool;
#ifndef true
    #define true  1
    #define false 0
#endif

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool atomic_set_ptr_if_equal(void** ptr, void* oldPtr, void* newPtr)
{
#if defined(LUAJACK_ASYNC_USE_WIN32)
    return oldValue == InterlockedCompareExchangePointer(ptr, newPtr, oldPtr);
#elif defined(LUAJACK_ASYNC_USE_APPLE)
    return OSAtomicCompareAndSwapPtrBarrier(oldPtr, newPtr, ptr);
#elif defined(LUAJACK_ASYNC_USE_GNU)
    return __sync_bool_compare_and_swap(ptr, oldPtr, newPtr);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

#if defined(LUAJACK_ASYNC_USE_WIN32)
typedef LONG AtomicCounter;
#elif defined(LUAJACK_ASYNC_USE_APPLE)
typedef int32_t AtomicCounter;
#elif defined(LUAJACK_ASYNC_USE_GNU)
typedef int AtomicCounter;
#endif

static inline int atomic_inc(AtomicCounter* value)
{
#if defined(LUAJACK_ASYNC_USE_WIN32)
    return InterlockedIncrement(value);
#elif defined(LUAJACK_ASYNC_USE_APPLE)
    return OSAtomicIncrement32Barrier(value);
#elif defined(LUAJACK_ASYNC_USE_GNU)
    return __sync_add_and_fetch(value, 1);
#endif
}

static inline int atomic_dec(AtomicCounter* value)
{
#if defined(LUAJACK_ASYNC_USE_WIN32)
    return InterlockedDecrement(value);
#elif defined(LUAJACK_ASYNC_USE_APPLE)
    return OSAtomicDecrement32Barrier(value);
#elif defined(LUAJACK_ASYNC_USE_GNU)
    return __sync_sub_and_fetch(value, 1);
#endif
}

static inline bool atomic_set_if_equal(AtomicCounter* value, int oldValue, int newValue)
{
#if defined(LUAJACK_ASYNC_USE_WIN32)
    return oldValue == InterlockedCompareExchange(value, newValue, oldValue);
#elif defined(LUAJACK_ASYNC_USE_APPLE)
    return OSAtomicCompareAndSwap32Barrier(oldValue, newValue, value);
#elif defined(LUAJACK_ASYNC_USE_GNU)
    return __sync_bool_compare_and_swap(value, oldValue, newValue);
#endif
}

static inline int atomic_get(AtomicCounter* value)
{
#if defined(LUAJACK_ASYNC_USE_WIN32)
    return InterlockedCompareExchange(value, 0, 0);
#elif defined(LUAJACK_ASYNC_USE_APPLE)
    return OSAtomicAdd32Barrier(0, value);
#elif defined(LUAJACK_ASYNC_USE_GNU)
    return __sync_add_and_fetch(value, 0);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////


typedef struct
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    pthread_mutexattr_t  attr;
    pthread_mutex_t      lock;

#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    CRITICAL_SECTION      lock;
#endif
} Lock;


/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_lock_init(Lock* lock)
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_mutexattr_init(&lock->attr);
    if (rc == 0) {
        rc = pthread_mutexattr_settype(&lock->attr, PTHREAD_MUTEX_RECURSIVE);
    }
    if (rc == 0) {
        rc = pthread_mutex_init(&lock->lock, &lock->attr);
    }
    return (rc == 0);
    
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    InitializeCriticalSection(&lock->lock);
    return true;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline void async_lock_destruct(Lock* lock)
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    pthread_mutex_destroy(&lock->lock);
    pthread_mutexattr_destroy(&lock->attr);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    DeleteCriticalSection(&lock->lock);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_lock_acquire(Lock* lock) 
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_mutex_lock(&lock->lock);
    return (rc == 0);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    EnterCriticalSection(&lock->lock);
    return true;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_lock_release(Lock* lock)
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_mutex_unlock(&lock->lock);
    return (rc == 0);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    LeaveCriticalSection(&lock->lock);
    return true;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    pthread_mutexattr_t   attr;
    pthread_mutex_t       mutex;
    pthread_cond_t        condition;

#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    CRITICAL_SECTION      mutex;
    volatile int          waitingCounter;
    HANDLE                event;
#endif
} Mutex;


/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_mutex_init(Mutex* mutex)
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_mutexattr_init(&mutex->attr);
    if (rc == 0) {
        rc = pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_RECURSIVE);
    }
    if (rc == 0) {
        rc = pthread_mutex_init(&mutex->mutex, &mutex->attr);
    }
    if (rc == 0) {
        rc = pthread_cond_init(&mutex->condition, NULL);
    }
    return (rc == 0);
    
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    InitializeCriticalSection(&mutex->mutex);

    mutex->waitingCounter = 0;

    mutex->event = CreateEvent (NULL,  // no security
                                FALSE, // auto-reset event
                                FALSE, // non-signaled initially
                                NULL); // unnamed
    return (mutex->event != NULL);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline void async_mutex_destruct(Mutex* mutex)
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    pthread_cond_destroy(&mutex->condition);
    pthread_mutex_destroy(&mutex->mutex);
    pthread_mutexattr_destroy(&mutex->attr);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    CloseHandle(mutex->event);
    DeleteCriticalSection(&mutex->mutex);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_mutex_lock(Mutex* mutex) 
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_mutex_lock(&mutex->mutex);
    return (rc == 0);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    EnterCriticalSection(&mutex->mutex);
    return true;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_mutex_unlock(Mutex* mutex) 
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_mutex_unlock(&mutex->mutex);
    return (rc == 0);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    LeaveCriticalSection(&mutex->mutex);
    return true;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_mutex_wait(Mutex* mutex) 
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_cond_wait(&mutex->condition, &mutex->mutex);
    return (rc == 0);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    mutex->waitingCounter += 1;
    LeaveCriticalSection(&mutex->mutex);
    
    DWORD rc = WaitForSingleObject(mutex->event, INFINITE);
    
    EnterCriticalSection(&mutex->mutex);
    mutex->waitingCounter -= 1;
    
    return (rc == WAIT_OBJECT_0);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_mutex_wait_millis(Mutex* mutex, int timeoutMillis)
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    struct timespec abstime;
    struct timeval tv;  gettimeofday(&tv, NULL);
    
    abstime.tv_sec = tv.tv_sec + timeoutMillis / 1000;
    abstime.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (timeoutMillis % 1000);
    abstime.tv_sec += abstime.tv_nsec / (1000 * 1000 * 1000);
    abstime.tv_nsec %= (1000 * 1000 * 1000);
    
    int rc = pthread_cond_timedwait(&mutex->condition, &mutex->mutex, &abstime);
    
    if (rc == 0) {
        return true;
    }
    else if (rc == ETIMEDOUT) {
        return false;
    }
    else {
        return false;
    }
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    mutex->waitingCounter += 1;
    LeaveCriticalSection(&mutex->mutex);

    DWORD rc = WaitForSingleObject(mutex->event, timeoutMillis);

    EnterCriticalSection(&mutex->mutex);
    mutex->waitingCounter -= 1;
    
    if (rc == WAIT_OBJECT_0) {
        return true;
    } else if (rc == WAIT_TIMEOUT) {
        return false;
    } else {
        return false;
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

static inline bool async_mutex_notify(Mutex* mutex) 
{
#if defined(LUAJACK_ASYNC_USE_PTHREAD)
    int rc = pthread_cond_signal(&mutex->condition);
    return (rc == 0);
#elif defined(LUAJACK_ASYNC_USE_WINTHREAD)
    if (mutex->waitingCounter > 0) {
        BOOL wasOk = SetEvent(mutex->event);
        return wasOk;
    } else {
        return true;
    }
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////

#endif // LUAJACK_ASYNC_UTIL_H
