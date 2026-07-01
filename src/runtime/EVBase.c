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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>
#include <evObj/EVString.h>

static _Atomic(EVClass *) ev_class_table[EV_MAX_CLASSES];
static _Atomic(uint64_t) ev_class_next = 1;

EVTypeID EVGetTypeID(EVObjectRef ref)
{
    return ((EVObject*)ref)->typeID;
}

bool EVEqual(EVObjectRef ref1,
             EVObjectRef ref2)
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
    EVTypeID typeID = EVGetTypeID(ref1);
    if(typeID != EVGetTypeID(ref2))
    {
        return false;
    }

    EVClass *class = EVClassGetByID(typeID);
    if(class->equal != NULL)
    {
        return class->equal(ref1, ref2);
    }

    /* no handler == not equal */
    return false;
}

EVObjectRef EVRetain(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    assert(object != NULL);
    if(object->is_stack_obj)
    {
        return ref;
    }
    atomic_fetch_add_explicit(&object->refcount, 1, memory_order_relaxed);
    return ref;
}

void EVRelease(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    assert(object != NULL);
    if(object->is_stack_obj)
    {
        return;
    }

    /* releasing and trying to get the old reference count */
    int old = atomic_fetch_sub_explicit(&object->refcount, 1, memory_order_release);
    if(old == 1)
    {
        atomic_thread_fence(memory_order_acquire);
        /* trigger handler */
        EVClass *class = EVClassGetByID(object->typeID);
        assert(class != NULL);
        if(class->deinit != NULL)
        {
            class->deinit(ref);
        }
        EVObjectDealloc(object);
    }
    else if(old <= 0)
    {
        fprintf(stderr, "EVRelease: fatal error occured, reference underflow\n");
        exit(1);
    }
}

int EVGetRetainCount(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    assert(object != NULL);
    return atomic_load(&object->refcount);
}

EVTypeID EVClassRegister(EVClass *cls)
{
    assert(cls != NULL);
    uint64_t id = atomic_fetch_add_explicit(&ev_class_next, 1, memory_order_relaxed);
    if(id >= EV_MAX_CLASSES)
    {
        return kEVNotATypeID;
    }

    cls->typeID = id;
    atomic_store_explicit(&ev_class_table[id], cls, memory_order_release);
    return id;
}

EVClass *EVClassGetByID(EVTypeID id)
{
    if(id == kEVNotATypeID || id >= EV_MAX_CLASSES)
    {
        return NULL;
    }
    return atomic_load_explicit(&ev_class_table[id], memory_order_acquire);
}

EVAllocatorRef EVGetAllocator(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    assert(object != NULL);
    return object->allocator;
}

EVStringRef EVCopyDescription(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    if(object == NULL)
    {
        return EV_STR("<nil>");
    }

    EVClass *cls = EVClassGetByID(object->typeID);
    if(cls == NULL)
    {
        return EV_STR("<nil>");
    }

    if(cls->copyDescription)
    {
        EVStringRef descriptionRef = cls->copyDescription(ref);
        if(descriptionRef != NULL)
        {
            return descriptionRef;
        }
    }

    EVStringRef descriptionFallbackRef = EVStringCreateWithFormat(object->allocator, EV_STR("<%s %p>"), cls->name, ref);
    if(descriptionFallbackRef == NULL)
    {
        return EV_STR("<nil>");
    }

    return descriptionFallbackRef;
}

void EVLog(EVStringRef formatStringRef, ...)
{
    va_list arguments;
    va_start(arguments, formatStringRef);
    EVStringRef resultRef = EVStringCreateWithFormatAndArguments(NULL, formatStringRef, arguments);
    va_end(arguments);

    if(resultRef == NULL)
    {
        return;
    }

    const char *resultCptr = EVStringGetCStringPtr(resultRef, kEVStringEncodingUTF8);
    if(resultCptr)
    {
        fprintf(stderr, "%s", resultCptr);
    }
}
