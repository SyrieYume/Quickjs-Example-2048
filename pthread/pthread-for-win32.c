/**
 * 使用Windows的线程模型实现几个pthread的API
 * 没仔细测试过，可能有bug
 */

#include <windows.h>
#include <stdlib.h>
#include <errno.h>

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 0x04
#define CLOCK_REALTIME 0

// 魔法数字：用于将Windows纪元（1601年1月1日）转换为Unix纪元（1970年1月1日）
#define UNIX_EPOCH_OFFSET 116444736000000000ULL


typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

typedef int pthread_condattr_t; 
typedef int pthread_mutexattr_t;

typedef struct {
    unsigned p_state;
    void *stack;
    size_t s_size;
    struct { int sched_priority; } param;
} pthread_attr_t;

typedef HANDLE pthread_t;

typedef struct {
    void *(*start_routine)(void *);
    void *arg;
} thread_info_t;

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
    long tv_sec;
    long tv_nsec;
};
#endif

typedef int clockid_t;


#define WINPTHREAD_API


WINPTHREAD_API int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a) {
    InitializeCriticalSection(m);
    return 0;
}

WINPTHREAD_API int pthread_mutex_destroy(pthread_mutex_t *m) {
    DeleteCriticalSection(m);
    return 0;
}

WINPTHREAD_API int pthread_mutex_lock(pthread_mutex_t *m) {
    EnterCriticalSection(m);
    return 0;
}

WINPTHREAD_API int pthread_mutex_unlock(pthread_mutex_t *m) {
    LeaveCriticalSection(m);
    return 0;
}


WINPTHREAD_API int pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *a) {
    InitializeConditionVariable(cv);
    return 0;
}

WINPTHREAD_API int pthread_cond_destroy(pthread_cond_t *cv) {
    return 0;
}

WINPTHREAD_API int pthread_cond_signal(pthread_cond_t *cv) {
    WakeConditionVariable(cv);
    return 0;
}

WINPTHREAD_API int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *external_mutex) {
    if (SleepConditionVariableCS(cv, external_mutex, INFINITE))
        return 0;
    return EINVAL;
}

WINPTHREAD_API int pthread_attr_init(pthread_attr_t *attr) {
    if (attr == NULL)
        return EINVAL;
    attr->p_state = PTHREAD_CREATE_JOINABLE;
    return 0;
}

WINPTHREAD_API int pthread_attr_setdetachstate(pthread_attr_t *a, int flag) {
    if (!a || (flag != PTHREAD_CREATE_JOINABLE && flag != PTHREAD_CREATE_DETACHED))
        return EINVAL;

    a->p_state = flag;
    return 0;
}

DWORD WINAPI _win_thread_wrapper(LPVOID lpParameter) {
    thread_info_t *ti = (thread_info_t *)lpParameter;
    if (ti == NULL)
        return -1;

    void *result = ti->start_routine(ti->arg);

    free(ti); 

    return (DWORD)(DWORD_PTR)result;
}

WINPTHREAD_API int pthread_create(pthread_t *th, const pthread_attr_t *attr, void *(* func)(void *), void *arg) {
    thread_info_t *ti = (thread_info_t *)malloc(sizeof(thread_info_t));
    if (ti == NULL)
        return ENOMEM;

    ti->start_routine = func;
    ti->arg = arg;

    HANDLE hThread = CreateThread(NULL, 0, _win_thread_wrapper, ti, 0, NULL);

    if (hThread == NULL) {
        free(ti);
        return errno == EAGAIN ? EAGAIN : EINVAL;
    }

    if (attr && attr->p_state == PTHREAD_CREATE_DETACHED) {
        CloseHandle(hThread);
        *th = NULL;
    }
    else *th = hThread;

    return 0;
}

WINPTHREAD_API int pthread_attr_destroy(pthread_attr_t *attr) {
    if (!attr)
        return EINVAL;

    attr->p_state = -1;
    return 0;
}

WINPTHREAD_API int __cdecl clock_gettime(clockid_t clock_id, struct timespec *tp) {
    if (clock_id != CLOCK_REALTIME) {
        errno = EINVAL;
        return -1;
    }

    FILETIME ft;
    ULARGE_INTEGER uli;

    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    uli.QuadPart -= UNIX_EPOCH_OFFSET;
    tp->tv_sec = (long)(uli.QuadPart / 10000000L);
    tp->tv_nsec = (long)(uli.QuadPart % 10000000L) * 100;

    return 0;
}


WINPTHREAD_API int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *external_mutex, const struct timespec *t) {
    struct timespec current_ts;
    DWORD timeout_ms;

    if (clock_gettime(CLOCK_REALTIME, &current_ts) != 0)
        return EINVAL;

    long long target_ms = (long long)t->tv_sec * 1000 + t->tv_nsec / 1000000;
    long long current_ms = (long long)current_ts.tv_sec * 1000 + current_ts.tv_nsec / 1000000;
    
    if (target_ms <= current_ms) 
        timeout_ms = 0;
     else 
        timeout_ms = (DWORD)(target_ms - current_ms);

    if (SleepConditionVariableCS(cv, external_mutex, timeout_ms)) 
        return 0;
    
    if (GetLastError() == ERROR_TIMEOUT)
        return ETIMEDOUT;

    return EINVAL;
}
