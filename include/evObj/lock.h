/*
 * MIT License
 *
 * Copyright (c) 2026 emexLabs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EVOBJECT_LOCK_H
#define EVOBJECT_LOCK_H

#include <evObj/defs.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#if DEBUG
#define PTHREAD_RWLOCK_DEBUG_IMP_RDLOCK(ptr) do { \
    int _e; \
    do { \
        _e = pthread_rwlock_rdlock(ptr); \
        if(_e == EAGAIN) \
        { \
            sched_yield(); \
        } \
        else if(_e) \
        { \
            panic("lock @ %p failed to rdlock with: %s", ptr, strerror(_e)); \
        } \
    } while (_e == EAGAIN); \
} while(0)
#define PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(ptr) ({ int _e = pthread_rwlock_wrlock(ptr); if (_e) panic("lock @ %p failed to wrlock with: %s", ptr, strerror(_e)); })
#define PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(ptr) ({ int _e = pthread_rwlock_unlock(ptr); if (_e) panic("lock @ %p failed to unlock with: %s", ptr, strerror(_e)); })
#else
#define PTHREAD_RWLOCK_DEBUG_IMP_RDLOCK(ptr) do { \
    int _e; \
    do { \
        _e = pthread_rwlock_rdlock(ptr); \
        if(_e == EAGAIN) \
        { \
            sched_yield(); \
        } \
    } while (_e == EAGAIN); \
} while(0)
#define PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(ptr) pthread_rwlock_wrlock(ptr);
#define PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(ptr) pthread_rwlock_unlock(ptr);
#endif /* DEBUG */

#define proc_table_rdlock() PTHREAD_RWLOCK_DEBUG_IMP_RDLOCK(&(ksurface->proc_info.struct_lock))
#define proc_table_wrlock() PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(ksurface->proc_info.struct_lock))
#define proc_table_unlock() PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(ksurface->proc_info.struct_lock))

#define tty_table_rdlock() PTHREAD_RWLOCK_DEBUG_IMP_RDLOCK(&(ksurface->tty_info.struct_lock))
#define tty_table_wrlock() PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(ksurface->tty_info.struct_lock))
#define tty_table_unlock() PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(ksurface->tty_info.struct_lock))

#define host_rdlock() PTHREAD_RWLOCK_DEBUG_IMP_RDLOCK(&(ksurface->host_info.struct_lock))
#define host_wrlock() PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(ksurface->host_info.struct_lock))
#define host_unlock() PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(ksurface->host_info.struct_lock))

#define task_rdlock() PTHREAD_RWLOCK_DEBUG_IMP_RDLOCK(&(ksurface->proc_info.task_lock))
#define task_wrlock() PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(ksurface->proc_info.task_lock))
#define task_unlock() PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(ksurface->proc_info.task_lock))

#define evo_rdlock(obj) PTHREAD_RWLOCK_DEBUG_IMP_RDLOCK(&(((evobject_strong_t *)obj)->rwlock))
#define evo_wrlock(obj) PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(((evobject_strong_t *)obj)->rwlock))
#define evo_unlock(obj) PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(((evobject_strong_t *)obj)->rwlock))

#endif /* EVOBJECT_LOCK_H */
