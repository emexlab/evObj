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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EFENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EFBASE_H
#define EFBASE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <pthread.h>

#define kEFNotATypeID   ((uint64_t)0)
#define EF_MAX_CLASSES  1024

/* normal types */
typedef unsigned long EFOptionFlags;
typedef unsigned long EFHashCode;
typedef long EFIndex;
typedef struct {
    EFIndex location;
    EFIndex length;
} EFRange;
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
typedef double EFTimeInterval;
typedef EFTimeInterval EFAbsoluteTime;
/* no OSStatus??? this is modern, aint macintosh carbon era shit, my water aint carbonised */

/* special types */
typedef unsigned long EFTypeID;
typedef void * EFObjectRef;  /* so the compiler shuts up */
typedef struct EFAllocator EFAllocator;
typedef void * EFAllocatorRef;

typedef EFObjectRef EFStringRef;

typedef void (*evobject_init_handler_t)(EFObjectRef ref);
typedef void (*evobject_deinit_handler_t)(EFObjectRef ref);
typedef EFObjectRef (*evobject_copy_handler_t)(EFObjectRef ref);
typedef Boolean (*evobject_equal_handler_t)(EFObjectRef ref1, EFObjectRef ref2);
typedef EFStringRef (*evobject_copy_description_handler_t)(EFObjectRef ref);

typedef EFObjectRef (*evallocator_alloc_handler_t)(EFAllocatorRef allocatorRef, EFTypeID typeID, size_t size);
typedef void (*evallocator_dealloc_handler_t)(EFAllocatorRef allocatorRef, EFObjectRef ref);

typedef struct {
    /* properties  */
    const char *name;
    EFTypeID typeID;

    /* handlers */
    evobject_init_handler_t init;
    evobject_deinit_handler_t deinit;
    evobject_equal_handler_t equal;
    evobject_copy_description_handler_t copyDescription;
} EFClass;

typedef struct EFAllocator {
    /* properties  */
    const char *name;
    void *info; /* a more complex allocator in the future will need this */

    /* handlers */
    evallocator_alloc_handler_t allocate;
    evallocator_dealloc_handler_t deallocate;
} EFAllocator;

typedef struct {
    /*
     * the typeID of the class of that
     * object, similar to CFRuntime.
     */
    EFTypeID typeID;

    /* self explainatory */
    EFAllocator *allocator;

    /* set once */
    Boolean is_stack_obj;

    /*
     * reference count of an object if
     * it hits zero it will release
     * automatically.
     */
    _Atomic EFIndex refcount;
} EFObject;

EFTypeID EFGetTypeID(EFObjectRef ref);
Boolean EFEqual(EFObjectRef ref1, EFObjectRef ref2);

EFObjectRef EFRetain(EFObjectRef ref);
void EFRelease(EFObjectRef ref);
EFIndex EFGetRetainCount(EFObjectRef ref);

EFTypeID EFClassRegister(EFClass *cls);
EFClass *EFClassGetByID(EFTypeID id);

EFAllocatorRef EFGetAllocator(EFObjectRef ref);

EFStringRef EFCopyDescription(EFObjectRef ref);

void EFLog(EFStringRef formatStringRef, ...);

#endif /* EFBASE_H */
