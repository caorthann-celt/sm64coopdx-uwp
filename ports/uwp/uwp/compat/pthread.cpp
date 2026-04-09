#include "pthread.h"

#include <Windows.h>

struct PthreadStart {
    void* (*routine)(void*);
    void* arg;
};

static DWORD WINAPI pthread_trampoline(void* param) {
    PthreadStart* start = static_cast<PthreadStart*>(param);
    void* (*routine)(void*) = start->routine;
    void* arg = start->arg;
    delete start;
    routine(arg);
    return 0;
}

static SRWLOCK* pthread_get_lock(pthread_mutex_t* mutex) {
    if (mutex == nullptr) {
        return nullptr;
    }

    SRWLOCK* lock = static_cast<SRWLOCK*>(*mutex);
    if (lock == nullptr) {
        lock = new SRWLOCK;
        InitializeSRWLock(lock);
        *mutex = lock;
    }

    return lock;
}

int pthread_attr_init(pthread_attr_t* attr) {
    (void) attr;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t* attr, int state) {
    (void) attr;
    (void) state;
    return 0;
}

int pthread_attr_setstack(pthread_attr_t* attr, void* stackaddr, size_t stacksize) {
    (void) attr;
    (void) stackaddr;
    (void) stacksize;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t* attr) {
    (void) attr;
    return 0;
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    (void) attr;
    if (thread == nullptr || start_routine == nullptr) {
        return 1;
    }

    PthreadStart* start = new PthreadStart { start_routine, arg };
    HANDLE handle = CreateThread(nullptr, 0, pthread_trampoline, start, 0, nullptr);
    if (handle == nullptr) {
        delete start;
        return 1;
    }

    *thread = handle;
    return 0;
}

int pthread_join(pthread_t thread, void** retval) {
    (void) retval;
    if (thread == nullptr) {
        return 1;
    }

    WaitForSingleObject(static_cast<HANDLE>(thread), INFINITE);
    CloseHandle(static_cast<HANDLE>(thread));
    return 0;
}

int pthread_detach(pthread_t thread) {
    if (thread != nullptr) {
        CloseHandle(static_cast<HANDLE>(thread));
    }
    return 0;
}

void pthread_exit(void* retval) {
    (void) retval;
    ExitThread(0);
}

int pthread_cancel(pthread_t thread) {
    (void) thread;
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t* attr) {
    (void) attr;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type) {
    (void) attr;
    (void) type;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t* attr) {
    (void) attr;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    (void) attr;
    if (mutex == nullptr) {
        return 1;
    }

    SRWLOCK* lock = new SRWLOCK;
    InitializeSRWLock(lock);
    *mutex = lock;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    if (mutex != nullptr && *mutex != nullptr) {
        delete static_cast<SRWLOCK*>(*mutex);
        *mutex = nullptr;
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    SRWLOCK* lock = pthread_get_lock(mutex);
    if (lock == nullptr) {
        return 1;
    }

    AcquireSRWLockExclusive(lock);
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    SRWLOCK* lock = pthread_get_lock(mutex);
    if (lock == nullptr) {
        return 1;
    }

    return TryAcquireSRWLockExclusive(lock) ? 0 : 1;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    SRWLOCK* lock = pthread_get_lock(mutex);
    if (lock == nullptr) {
        return 1;
    }

    ReleaseSRWLockExclusive(lock);
    return 0;
}
