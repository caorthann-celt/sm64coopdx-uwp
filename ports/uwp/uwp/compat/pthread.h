#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* pthread_t;
typedef struct pthread_attr_t { int unused; } pthread_attr_t;
typedef void* pthread_mutex_t;
typedef struct pthread_mutexattr_t { int unused; } pthread_mutexattr_t;

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_MUTEX_ERRORCHECK 1
#define PTHREAD_MUTEX_INITIALIZER NULL

int pthread_attr_init(pthread_attr_t* attr);
int pthread_attr_setdetachstate(pthread_attr_t* attr, int state);
int pthread_attr_setstack(pthread_attr_t* attr, void* stackaddr, size_t stacksize);
int pthread_attr_destroy(pthread_attr_t* attr);
int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int pthread_join(pthread_t thread, void** retval);
int pthread_detach(pthread_t thread);
void pthread_exit(void* retval);
int pthread_cancel(pthread_t thread);
int pthread_mutexattr_init(pthread_mutexattr_t* attr);
int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type);
int pthread_mutexattr_destroy(pthread_mutexattr_t* attr);
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

#ifdef __cplusplus
}
#endif
