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

#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <evObj/EVString.h>
#include <evObj/EVPageGroup.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>

typedef struct EVPageGroup {
    EVObject header;
    EVMutableArrayRef pagesArrayRef;
} *EVPageGroup;

static void __EVPageGroupDeinit(EVPageRef pageRef)
{
    EVPageGroup group = (EVPageGroup)pageRef;
    EVRelease(group->pagesArrayRef);
}

static EVStringRef __EVPageGroupCopyDescription(EVPageGroupRef groupRef)
{
    EVPageGroup group = (EVPageGroup)groupRef;
    EVAllocatorRef allocatorRef = EVGetAllocator(groupRef);
    EVClass *cls = EVClassGetByID(group->header.typeID);
    return EVStringCreateWithFormat(allocatorRef, EV_STR("<%s %p>{pagesArray = %@, len = %zu}"), cls->name, groupRef, group->pagesArrayRef, EVPageGroupGetSize(groupRef));
}

static EVClass EVPageGroupClass = {
    .name = "EVPageGroup",
    .typeID = kEVNotATypeID,
    .init = NULL,
    .deinit = __EVPageGroupDeinit,
    .equal = NULL,
    .copyDescription = __EVPageGroupCopyDescription,
};

static void EVPageGroupRegisterClass(void)
{
    EVClassRegister(&EVPageGroupClass);
}

EVTypeID EVPageGroupGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EVPageGroupRegisterClass);
    return EVPageGroupClass.typeID;
}

EVPageGroupRef EVPageGroupCreate(EVAllocatorRef allocatorRef)
{
    EVArrayRef pagesArrayRef = EVArrayCreateMutable(allocatorRef, kEVArrayCallbacksObjectCallbacks, 1);
    if(pagesArrayRef == NULL)
    {
        return NULL;
    }

    EVPageRef pageRef = EVPageCreate(allocatorRef);
    if(pageRef == NULL)
    {
        EVRelease(pagesArrayRef);
        return NULL;
    }

    bool success = EVArrayAppendValue(pagesArrayRef, pageRef);
    EVRelease(pageRef);
    if(!success)
    {
        EVRelease(pagesArrayRef);
        return NULL;
    }

    EVPageGroupRef pageGroupRef = EVPageGroupCreateWithPages(allocatorRef, pagesArrayRef);
    EVRelease(pagesArrayRef);
    return pageGroupRef == NULL ? NULL : pageGroupRef;
}

EVPageGroupRef EVPageGroupCreateWithPages(EVAllocatorRef allocatorRef,
                                          EVArrayRef pagesArrayRef)
{
    /* validating passed array ref */
    uint64_t count = EVArrayGetCount(pagesArrayRef);
    for(uint64_t i = 0; i < count; i++)
    {
        EVPageRef pageRef = EVArrayGetValueAtIndex(pagesArrayRef, i);
        if(EVGetTypeID(pageRef) != EVPageGetTypeID())
        {
            /* not a page */
            return NULL;
        }
    }

    /* fallback allocator */
    if(allocatorRef == NULL)
    {
        allocatorRef = EVGetAllocator(pagesArrayRef);
    }

    /* we need a reference our selves */
    EVMutableArrayRef ownedPagesArrayRef = EVArrayCreateMutableCopy(allocatorRef, pagesArrayRef);
    if(ownedPagesArrayRef == NULL)
    {
        return NULL;
    }

    /* now we gotta create the object it self */
    EVPageGroup group = (EVPageGroup)EVObjectAlloc(allocatorRef, EVPageGroupGetTypeID(), sizeof(struct EVPageGroup));
    if(group == NULL)
    {
        EVRelease(ownedPagesArrayRef);
        return NULL;
    }

    group->pagesArrayRef = ownedPagesArrayRef;
    return (EVPageGroupRef)group;
}

EVArrayRef EVPageGroupCopyPages(EVAllocatorRef allocatorRef,
                                EVPageGroupRef groupRef)
{
    EVPageGroup group = (EVPageGroup)groupRef;
    if(group == NULL)
    {
        return NULL;
    }
    return EVArrayCreateCopy(allocatorRef, group->pagesArrayRef);
}

size_t EVPageGroupGetSize(EVPageGroupRef groupRef)
{
    EVPageGroup group = (EVPageGroup)groupRef;
    if(group == NULL)
    {
        return 0;
    }

    size_t size = 0;
    uint64_t count = EVArrayGetCount(group->pagesArrayRef);
    for(uint64_t i = 0; i < count; i++)
    {
        EVPageRef pageRef = EVArrayGetValueAtIndex(group->pagesArrayRef, i);
        size += EVPageGetSize(pageRef);
    }

    return size;
}

bool EVPageGroupExtend(EVPageGroupRef groupRef)
{
    EVPageGroup group = (EVPageGroup)groupRef;
    if(group == NULL)
    {
        return false;
    }

    EVAllocatorRef allocatorRef = EVGetAllocator(groupRef);
    EVPageRef pageRef = EVPageCreate(allocatorRef);
    if(pageRef == NULL)
    {
        return false;
    }

    bool success = EVArrayAppendValue(group->pagesArrayRef, pageRef);
    EVRelease(pageRef);
    return success;
}

bool EVPageGroupMerge(EVPageGroupRef groupRef)
{
    EVPageGroup group = (EVPageGroup)groupRef;
    if(group == NULL)
    {
        return false;
    }

    uint64_t count = EVArrayGetCount(group->pagesArrayRef);
    if(count <= 0)
    {
        return true;
    }

    /* allocating new huge page */
    EVAllocatorRef allocatorRef = EVGetAllocator(groupRef);
    size_t total_len = EVPageGroupGetSize(groupRef);
    EVPageRef hugePageRef = EVPageCreateWithOptions(allocatorRef, NULL, total_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(hugePageRef == NULL)
    {
        return false;
    }

    /* copying data from old pages to the new single huge page */
    uint8_t *hugePagePtr = (uint8_t*)EVPageGetPtr(hugePageRef);
    if(hugePagePtr == NULL)
    {
        /* wtf?! */
        EVRelease(hugePageRef);
        return false;
    }

    uint64_t count = EVArrayGetCount(group->pagesArrayRef);
    size_t loc = 0;
    for(uint64_t i = 0; i < count; i++)
    {
        EVPageRef pageRef = EVArrayGetValueAtIndex(group->pagesArrayRef, i);
        void *pagePtr = EVPageGetPtr(pageRef);
        size_t pageSize = EVPageGetSize(pageRef);

        memcpy(hugePagePtr + loc, pagePtr, pageSize);
        loc += pageSize;
    }

    /* first appending new page */
    bool success = EVArrayAppendValue(group->pagesArrayRef, hugePageRef);
    EVRelease(hugePageRef);
    if(!success)
    {
        return false;
    }

    /* then you remove the old ones lol */
    for(uint64_t i = 0; i < count; i++)
    {
        EVArrayRemoveValueAtIndex(group->pagesArrayRef, 0);
    }

    /* the result shall be one page */
    return true;
}

typedef enum {
    kVPXferRead,
    kVPXferWrite
} kVPXfer;

static size_t __EVPageGroupXfer(EVPageGroup groupRef,
                                size_t off,
                                uint8_t *b,
                                size_t len,
                                kVPXfer xfer)
{
    EVPageGroup group = (EVPageGroup)groupRef;
    if(group == NULL)
    {
        return 0;
    }

    size_t total = EVPageGroupGetSize(groupRef);

    /* avoiding overflow */
    if(off >= total)
    {
        return 0;
    }
    if(len > total - off)
    {
        len = total - off;
    }

    /* walk to the page that contains the starting offset */
    size_t base = 0;
    uint64_t count = EVArrayGetCount(group->pagesArrayRef);
    uint64_t index = 0;
    EVPageRef pageRef = NULL;
    for(; index < count; index++)
    {
        pageRef = EVArrayGetValueAtIndex(group->pagesArrayRef, index);
        size_t pageSize = EVPageGetSize(pageRef);
        if(!(base + pageSize <= off))
        {
            break;
        }
    }

    size_t done = 0;
    while(len > 0 && index < count)
    {
        uint8_t *pagePtr = EVPageGetPtr(pageRef);
        size_t pageSize = EVPageGetSize(pageRef);
        size_t pageOff = off - base;
        size_t avail = pageSize - pageOff;
        size_t n = (len < avail) ? len : avail;

        if(xfer == kVPXferWrite)
        {
            memcpy(pagePtr + pageOff, b + done, n);
        }
        else
        {
            memcpy(b + done, pagePtr + pageOff, n);
        }

        done += n;
        len -= n;
        off += n;
        base += pageSize;

        index++;
        if(index >= count)
        {
            break;
        }
        pageRef = EVArrayGetValueAtIndex(group->pagesArrayRef, index);
    }

    return done;
}

size_t EVPageGroupWrite(EVPageGroupRef groupRef,
                        size_t off,
                        const uint8_t *b,
                        size_t len)
{
    return __EVPageGroupXfer(groupRef, off, (uint8_t*)b, len, kVPXferWrite);
}

size_t EVPageGroupRead(EVPageGroupRef groupRef,
                       size_t off,
                       uint8_t *b,
                       size_t len)
{
    return __EVPageGroupXfer(groupRef, off, b, len, kVPXferRead);
}
