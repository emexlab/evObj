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

typedef uint64_t EVTypeID;

/* kernel virt object types */
typedef struct evobject     EVObject;               /* weak object type (needs retain on use) */
typedef void * EVObjectRef;  /* so the compiler shuts up */

typedef struct evobject {
    /*
     * the typeID of the class of that
     * object, similar to CFRuntime.
     */
    uint64_t typeID;

    /*
     * reference count of an object if
     * it hits zero it will release
     * automatically.
     */
    _Atomic int refcount;
} EVObject;

typedef void (*evobject_init_handler_t)(EVObjectRef ref);
typedef void (*evobject_deinit_handler_t)(EVObjectRef ref);
typedef EVObject *(*evobject_copy_handler_t)(EVObjectRef ref);
typedef bool (*evobject_equal_handler_t)(EVObjectRef ref1, EVObjectRef ref2);

typedef struct evclass {
    /* properties  */
    const char *name;
    size_t size;                    /* must be bigger than the header it self */
    EVTypeID typeID;

    /* handlers */
    evobject_init_handler_t init;
    evobject_deinit_handler_t deinit;
    evobject_copy_handler_t copy;
    evobject_equal_handler_t equal;
} EVClass;

#endif /* EVOBJECT_DEFS_H */
