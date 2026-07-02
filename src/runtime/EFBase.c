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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>
#include <EmexFoundation/EFString.h>

static _Atomic(EFClass *) ev_class_table[EF_MAX_CLASSES];
static _Atomic(uint64_t) ev_class_next = 1;

EFTypeID EFGetTypeID(EFObjectRef ref)
{
    return ((EFObject*)ref)->typeID;
}

Boolean EFEqual(EFObjectRef ref1,
                EFObjectRef ref2)
{
    if(ref1 == ref2)
    {
        return true;
    }
    if(ref1 == NULL || ref2 == NULL)
    {
        return false;
    }

    /* types must match */
    EFTypeID typeID = EFGetTypeID(ref1);
    if(typeID != EFGetTypeID(ref2))
    {
        return false;
    }

    EFClass *class = EFClassGetByID(typeID);
    if(class->equal != NULL)
    {
        return class->equal(ref1, ref2);
    }

    /* no handler == not equal */
    return false;
}

EFObjectRef EFRetain(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    if(object->is_stack_obj)
    {
        return ref;
    }
    atomic_fetch_add_explicit(&object->refcount, 1, memory_order_relaxed);
    return ref;
}

void EFRelease(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    if(object->is_stack_obj)
    {
        return;
    }

    /* releasing and trying to get the old reference count */
    EFIndex old = atomic_fetch_sub_explicit(&object->refcount, 1, memory_order_release);
    if(old == 1)
    {
        atomic_thread_fence(memory_order_acquire);
        /* trigger handler */
        EFClass *class = EFClassGetByID(object->typeID);
        assert(class != NULL);
        if(class->deinit != NULL)
        {
            class->deinit(ref);
        }
        EFObjectDealloc(object);
    }
    else if(old <= 0)
    {
        fprintf(stderr, "EFRelease: fatal error occured, reference underflow\n");
        exit(1);
    }
}

EFIndex EFGetRetainCount(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    return atomic_load(&object->refcount);
}

EFTypeID EFClassRegister(EFClass *cls)
{
    assert(cls != NULL);
    uint64_t id = atomic_fetch_add_explicit(&ev_class_next, 1, memory_order_relaxed);
    if(id >= EF_MAX_CLASSES)
    {
        return kEFNotATypeID;
    }

    cls->typeID = id;
    atomic_store_explicit(&ev_class_table[id], cls, memory_order_release);
    return id;
}

EFClass *EFClassGetByID(EFTypeID id)
{
    if(id == kEFNotATypeID || id >= EF_MAX_CLASSES)
    {
        return NULL;
    }
    return atomic_load_explicit(&ev_class_table[id], memory_order_acquire);
}

EFAllocatorRef EFGetAllocator(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    assert(object != NULL);
    return object->allocator;
}

EFStringRef EFCopyDescription(EFObjectRef ref)
{
    EFObject *object = (EFObject*)ref;
    if(object == NULL)
    {
        return EF_STR("<nil>");
    }

    EFClass *cls = EFClassGetByID(object->typeID);
    if(cls == NULL)
    {
        return EF_STR("<nil>");
    }

    if(cls->copyDescription)
    {
        EFStringRef descriptionRef = cls->copyDescription(ref);
        if(descriptionRef != NULL)
        {
            return descriptionRef;
        }
    }

    EFStringRef descriptionFallbackRef = EFStringCreateWithFormat(object->allocator, EF_STR("<%s %p>"), cls->name, ref);
    if(descriptionFallbackRef == NULL)
    {
        return EF_STR("<nil>");
    }

    return descriptionFallbackRef;
}

void EFLog(EFStringRef formatStringRef, ...)
{
    va_list arguments;
    va_start(arguments, formatStringRef);
    EFStringRef resultRef = EFStringCreateWithFormatAndArguments(NULL, formatStringRef, arguments);
    va_end(arguments);
    if(resultRef == NULL)
    {
        return;
    }

    const char *resultCptr = EFStringGetCStringPtr(resultRef, kEFStringEncodingUTF8);
    if(resultCptr)
    {
        fprintf(stderr, "%s", resultCptr);
    }

    EFRelease(resultRef);
}
