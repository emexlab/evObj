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

#ifndef EVOBJECT_DEFS_H
#define EVOBJECT_DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <pthread.h>

#define DEFINE_EVOBJECT_MAIN_EVENT_HANDLER(name) int64_t evobject_event_handler_##name##_main(evobject_t **evarr, evobject_event_type_t type)
#define GET_EVOBJECT_MAIN_EVENT_HANDLER(name) evobject_event_handler_##name##_main

#define EVOBJECT_EVENT_MAX 128

/* enumeration of kernel virt object base types */
enum evObjBaseType {
    evObjBaseTypeObject = 0,                        /* normal allocated object with referencing */
    evObjBaseTypeObjectSnapshot = 1,                /* snapshot of object also with referencing, but with seperate memory */
};

/* enumeration of kernel virt object events */
enum evObjEvent {
    evObjEventNone = 0,                             /* nothing, but for event registration relevant, they cannot escape getting deinit or unregistration notif */
    evObjEventInit = 1ull << 0,                     /* object initilizes                            MARK: important for main event handler */
    evObjEventDeinit = 1ull << 1,                   /* object deinitilizes                          MARK: important for main event handler */
    evObjEventCopy = 1ull << 2,                     /* object copies into new object                MARK: important for main event handler */
    evObjEventSnapshot = 1ull << 3,                 /* object snapshots into snapshotted object     MARK: important for main event handler */
    evObjEventInvalidate = 1ull << 4,               /* object becomes invalidated */
    evObjEventUnregister = 1ull << 5,               /* object event handler gets unregistered, only called on the affected handler */
    evObjEventCustom0 = 1ull << 6,                  /* custom object events */
    evObjEventCustom1 = 1ull << 7,
    evObjEventCustom2 = 1ull << 8,
    evObjEventCustom3 = 1ull << 9,
    evObjEventCustom4 = 1ull << 10,
    evObjEventCustom5 = 1ull << 11,
    evObjEventCustom6 = 1ull << 12,
    evObjEventCustom7 = 1ull << 13,
    evObjEventCustom8 = 1ull << 14,
    evObjEventCustom9 = 1ull << 15,
    evObjEventCustom10 = 1ull << 16
};

/* enumeration of kernel virt object states */
enum evObjState {
    evObjStateNormal = 0,                           /* object is in normal state */
    evObjStateInvalid                               /* object is invalidated and cannot be retained, only released, its used to mark a object as meaningless */
};

/* enumeration for type of snapshotting */
enum evObjSnap {
    evObjSnapStatic = 0,                            /* dont create reference back nor set orig pointer */
    evObjSnapReferenced,                            /* creates new reference and sets orig pointer */
    evObjSnapConsumeReference                       /* consumes callers reference and sets orig pointer */
};

/* kernel virt object types */
typedef struct evobject     evobject_t;             /* weak object type (needs retain on use) */
typedef struct evobject     evobject_strong_t;      /* strong object (referenced for calle) */
typedef struct evobject     evobject_snapshot_t;    /* snapshot of object (references object usually) */
typedef struct rcu_evobject rcu_evobject_t;         /* weak rcu object */
typedef struct rcu_evobject rcu_evobject_strong_t;  /* strong rcu object */

/* kernel virt object event type */
typedef struct evevent      evobject_event_t;

/* kernel virt object enumeration types */
typedef enum evObjBaseType  evobject_base_type_t;
typedef enum evObjEvent     evobject_event_type_t;
typedef enum evObjState     evobject_state_t;
typedef enum evObjSnap      evobject_snapshot_options_t;

typedef int64_t (*evobject_main_event_handler_t)(evobject_t**, evobject_event_type_t);
typedef bool (*evobject_event_handler_t)(evobject_event_type_t, uint64_t, evobject_event_t*);

struct evevent {
    evobject_event_t *previous;                     /* pointer to previous event */
    evobject_event_t *next;                         /* pointer to next event */
    evobject_t *owner;                              /* pointer of who owns the event */
    evobject_event_handler_t handler;               /* pointer to handler */
    evobject_event_type_t mask;                     /* event mask decides for what the handler does things */
    pthread_mutex_t in_use;                         /* usage marker (can cause freeze if no reference to the object exists anymore) */
    void *ctx;                                      /* pointer to payload MARK: if heap allocated, deallocate it on unregistration */
};

struct evobject {
    /* type of object */
    evobject_base_type_t base_type;

    /*
     * reference count of an object if
     * it hits zero it will release
     * automatically.
     */
    _Atomic int refcount;

    /*
     * the object state value marks a
     * object as effectively useless if its state
     * is invalid, any new retains will fail cuz it
     * doesnt matter anymore what a kernel operation
     * might wanna do with this object as its literally
     * marked as not useful anymore.
     */
    _Atomic evobject_state_t state;

    /* state handler for each object */
    evobject_main_event_handler_t main_handler;

    /* events */
    pthread_rwlock_t event_rwlock;
    uint64_t event_count;
    evobject_event_t *event;

    /*
     * main read-write lock of this structure,
     * mainly used when modifying kcproc.
     */
    pthread_rwlock_t rwlock;

    /* reference back to original (for snapshot) */
    evobject_strong_t *orig;
};

#endif /* EVOBJECT_DEFS_H */
