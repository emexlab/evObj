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

#ifndef EVBASE_H
#define EVBASE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <pthread.h>

#define kEVNotATypeID   ((uint64_t)0)
#define EV_MAX_CLASSES  1024

typedef uint64_t EVTypeID;
typedef void * EVObjectRef;  /* so the compiler shuts up */
typedef struct EVAllocator EVAllocator;
typedef void * EVAllocatorRef;

typedef void (*evobject_init_handler_t)(EVObjectRef ref);
typedef void (*evobject_deinit_handler_t)(EVObjectRef ref);
typedef EVObjectRef (*evobject_copy_handler_t)(EVObjectRef ref);
typedef bool (*evobject_equal_handler_t)(EVObjectRef ref1, EVObjectRef ref2);

typedef EVObjectRef (*evallocator_alloc_handler_t)(EVAllocatorRef allocatorRef, EVTypeID typeID, size_t size);
typedef void (*evallocator_dealloc_handler_t)(EVAllocatorRef allocatorRef, EVObjectRef ref);

typedef struct {
    /* properties  */
    const char *name;
    EVTypeID typeID;

    /* handlers */
    evobject_init_handler_t init;
    evobject_deinit_handler_t deinit;
    evobject_equal_handler_t equal;
} EVClass;

typedef struct EVAllocator {
    /* properties  */
    const char *name;

    /* handlers */
    evallocator_alloc_handler_t allocate;
    evallocator_dealloc_handler_t deallocate;
} EVAllocator;

typedef struct {
    /*
     * the typeID of the class of that
     * object, similar to CFRuntime.
     */
    EVTypeID typeID;

    /* self explainatory */
    EVAllocator *allocator;

    /* set once */
    bool is_stack_obj;

    /*
     * reference count of an object if
     * it hits zero it will release
     * automatically.
     */
    _Atomic int refcount;
} EVObject;

EVTypeID EVGetTypeID(EVObjectRef ref);
bool EVEqual(EVObjectRef ref1, EVObjectRef ref2);

EVObjectRef EVRetain(EVObjectRef ref);
void EVRelease(EVObjectRef ref);
int EVGetRetainCount(EVObjectRef ref);

EVTypeID EVClassRegister(EVClass *cls);
EVClass *EVClassGetByID(EVTypeID id);

EVAllocatorRef EVGetAllocator(EVObjectRef ref);

#endif /* EVBASE_H */
