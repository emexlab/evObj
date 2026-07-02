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

/* normal types */
typedef unsigned long EVOptionFlags;
typedef unsigned long EVHashCode;
typedef long EVIndex;
typedef struct {
    EVIndex location;
    EVIndex length;
} EVRange;
typedef unsigned char Boolean;
typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned int UInt32;
typedef signed int SInt32;
typedef uint64_t UInt64;
typedef int64_t SInt64;
typedef float Float32;
typedef double Float64;
typedef double EVTimeInterval;
typedef EVTimeInterval EVAbsoluteTime;
/* no OSStatus??? this is modern, aint macintosh carbon era shit, my water aint carbonised */

/* special types */
typedef unsigned long EVTypeID;
typedef void * EVObjectRef;  /* so the compiler shuts up */
typedef struct EVAllocator EVAllocator;
typedef void * EVAllocatorRef;

typedef EVObjectRef EVStringRef;

typedef void (*evobject_init_handler_t)(EVObjectRef ref);
typedef void (*evobject_deinit_handler_t)(EVObjectRef ref);
typedef EVObjectRef (*evobject_copy_handler_t)(EVObjectRef ref);
typedef Boolean (*evobject_equal_handler_t)(EVObjectRef ref1, EVObjectRef ref2);
typedef EVStringRef (*evobject_copy_description_handler_t)(EVObjectRef ref);

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
    evobject_copy_description_handler_t copyDescription;
} EVClass;

typedef struct EVAllocator {
    /* properties  */
    const char *name;
    void *info; /* a more complex allocator in the future will need this */

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
    Boolean is_stack_obj;

    /*
     * reference count of an object if
     * it hits zero it will release
     * automatically.
     */
    _Atomic EVIndex refcount;
} EVObject;

EVTypeID EVGetTypeID(EVObjectRef ref);
Boolean EVEqual(EVObjectRef ref1, EVObjectRef ref2);

EVObjectRef EVRetain(EVObjectRef ref);
void EVRelease(EVObjectRef ref);
EVIndex EVGetRetainCount(EVObjectRef ref);

EVTypeID EVClassRegister(EVClass *cls);
EVClass *EVClassGetByID(EVTypeID id);

EVAllocatorRef EVGetAllocator(EVObjectRef ref);

EVStringRef EVCopyDescription(EVObjectRef ref);

void EVLog(EVStringRef formatStringRef, ...);

#endif /* EVBASE_H */
