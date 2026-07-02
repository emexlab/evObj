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

#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/EFPageGroup.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>

typedef struct EFPageGroup {
    EFObject header;
    EFMutableArrayRef pagesArrayRef;
} *EFPageGroup;

static void __EFPageGroupDeinit(EFPageRef pageRef)
{
    EFPageGroup group = (EFPageGroup)pageRef;
    EFRelease(group->pagesArrayRef);
}

static EFStringRef __EFPageGroupCopyDescription(EFPageGroupRef groupRef)
{
    EFPageGroup group = (EFPageGroup)groupRef;
    EFAllocatorRef allocatorRef = EFGetAllocator(groupRef);
    EFClass *cls = EFClassGetByID(group->header.typeID);
    return EFStringCreateWithFormat(allocatorRef, EF_STR("<%s %p>{pagesArray = %@, len = %zu}"), cls->name, groupRef, group->pagesArrayRef, EFPageGroupGetSize(groupRef));
}

static EFClass EFPageGroupClass = {
    .name = "EFPageGroup",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFPageGroupDeinit,
    .equal = NULL,
    .copyDescription = __EFPageGroupCopyDescription,
};

static void EFPageGroupRegisterClass(void)
{
    EFClassRegister(&EFPageGroupClass);
}

EFTypeID EFPageGroupGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFPageGroupRegisterClass);
    return EFPageGroupClass.typeID;
}

EFPageGroupRef EFPageGroupCreate(EFAllocatorRef allocatorRef)
{
    EFArrayRef pagesArrayRef = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, 1);
    if(pagesArrayRef == NULL)
    {
        return NULL;
    }

    EFPageRef pageRef = EFPageCreate(allocatorRef);
    if(pageRef == NULL)
    {
        EFRelease(pagesArrayRef);
        return NULL;
    }

    bool success = EFArrayAppendValue(pagesArrayRef, pageRef);
    EFRelease(pageRef);
    if(!success)
    {
        EFRelease(pagesArrayRef);
        return NULL;
    }

    EFPageGroupRef pageGroupRef = EFPageGroupCreateWithPages(allocatorRef, pagesArrayRef);
    EFRelease(pagesArrayRef);
    return pageGroupRef == NULL ? NULL : pageGroupRef;
}

EFPageGroupRef EFPageGroupCreateWithPages(EFAllocatorRef allocatorRef,
                                          EFArrayRef pagesArrayRef)
{
    /* validating passed array ref */
    uint64_t count = EFArrayGetCount(pagesArrayRef);
    for(uint64_t i = 0; i < count; i++)
    {
        EFPageRef pageRef = EFArrayGetValueAtIndex(pagesArrayRef, i);
        if(EFGetTypeID(pageRef) != EFPageGetTypeID())
        {
            /* not a page */
            return NULL;
        }
    }

    /* fallback allocator */
    if(allocatorRef == NULL)
    {
        allocatorRef = EFGetAllocator(pagesArrayRef);
    }

    /* we need a reference our selves */
    EFMutableArrayRef ownedPagesArrayRef = EFArrayCreateMutableCopy(allocatorRef, pagesArrayRef);
    if(ownedPagesArrayRef == NULL)
    {
        return NULL;
    }

    /* now we gotta create the object it self */
    EFPageGroup group = (EFPageGroup)EFObjectAlloc(allocatorRef, EFPageGroupGetTypeID(), sizeof(struct EFPageGroup));
    if(group == NULL)
    {
        EFRelease(ownedPagesArrayRef);
        return NULL;
    }

    group->pagesArrayRef = ownedPagesArrayRef;
    return (EFPageGroupRef)group;
}

EFArrayRef EFPageGroupCopyPages(EFAllocatorRef allocatorRef,
                                EFPageGroupRef groupRef)
{
    EFPageGroup group = (EFPageGroup)groupRef;
    if(group == NULL)
    {
        return NULL;
    }
    return EFArrayCreateCopy(allocatorRef, group->pagesArrayRef);
}

size_t EFPageGroupGetSize(EFPageGroupRef groupRef)
{
    EFPageGroup group = (EFPageGroup)groupRef;
    if(group == NULL)
    {
        return 0;
    }

    size_t size = 0;
    uint64_t count = EFArrayGetCount(group->pagesArrayRef);
    for(uint64_t i = 0; i < count; i++)
    {
        EFPageRef pageRef = EFArrayGetValueAtIndex(group->pagesArrayRef, i);
        size += EFPageGetSize(pageRef);
    }

    return size;
}

bool EFPageGroupExtend(EFPageGroupRef groupRef)
{
    EFPageGroup group = (EFPageGroup)groupRef;
    if(group == NULL)
    {
        return false;
    }

    EFAllocatorRef allocatorRef = EFGetAllocator(groupRef);
    EFPageRef pageRef = EFPageCreate(allocatorRef);
    if(pageRef == NULL)
    {
        return false;
    }

    bool success = EFArrayAppendValue(group->pagesArrayRef, pageRef);
    EFRelease(pageRef);
    return success;
}

bool EFPageGroupMerge(EFPageGroupRef groupRef)
{
    EFPageGroup group = (EFPageGroup)groupRef;
    if(group == NULL)
    {
        return false;
    }

    uint64_t count = EFArrayGetCount(group->pagesArrayRef);
    if(count <= 0)
    {
        return true;
    }

    /* allocating new huge page */
    EFAllocatorRef allocatorRef = EFGetAllocator(groupRef);
    size_t total_len = EFPageGroupGetSize(groupRef);
    EFPageRef hugePageRef = EFPageCreateWithOptions(allocatorRef, NULL, total_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(hugePageRef == NULL)
    {
        return false;
    }

    /* copying data from old pages to the new single huge page */
    uint8_t *hugePagePtr = (uint8_t*)EFPageGetPtr(hugePageRef);
    if(hugePagePtr == NULL)
    {
        /* wtf?! */
        EFRelease(hugePageRef);
        return false;
    }

    size_t loc = 0;
    for(uint64_t i = 0; i < count; i++)
    {
        EFPageRef pageRef = EFArrayGetValueAtIndex(group->pagesArrayRef, i);
        void *pagePtr = EFPageGetPtr(pageRef);
        size_t pageSize = EFPageGetSize(pageRef);

        memcpy(hugePagePtr + loc, pagePtr, pageSize);
        loc += pageSize;
    }

    /* first appending new page */
    bool success = EFArrayAppendValue(group->pagesArrayRef, hugePageRef);
    EFRelease(hugePageRef);
    if(!success)
    {
        return false;
    }

    /* then you remove the old ones lol */
    for(uint64_t i = 0; i < count; i++)
    {
        EFArrayRemoveValueAtIndex(group->pagesArrayRef, 0);
    }

    /* the result shall be one page */
    return true;
}

typedef enum {
    kVPXferRead,
    kVPXferWrite
} kVPXfer;

static size_t __EFPageGroupXfer(EFPageGroup groupRef,
                                size_t off,
                                uint8_t *b,
                                size_t len,
                                kVPXfer xfer)
{
    EFPageGroup group = (EFPageGroup)groupRef;
    if(group == NULL)
    {
        return 0;
    }

    size_t total = EFPageGroupGetSize(groupRef);

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
    uint64_t count = EFArrayGetCount(group->pagesArrayRef);
    uint64_t index = 0;
    EFPageRef pageRef = NULL;
    for(; index < count; index++)
    {
        pageRef = EFArrayGetValueAtIndex(group->pagesArrayRef, index);
        size_t pageSize = EFPageGetSize(pageRef);
        if(!(base + pageSize <= off))
        {
            break;
        }
    }

    size_t done = 0;
    while(len > 0 && index < count)
    {
        uint8_t *pagePtr = EFPageGetPtr(pageRef);
        size_t pageSize = EFPageGetSize(pageRef);
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
        pageRef = EFArrayGetValueAtIndex(group->pagesArrayRef, index);
    }

    return done;
}

size_t EFPageGroupWrite(EFPageGroupRef groupRef,
                        size_t off,
                        const uint8_t *b,
                        size_t len)
{
    return __EFPageGroupXfer(groupRef, off, (uint8_t*)b, len, kVPXferWrite);
}

size_t EFPageGroupRead(EFPageGroupRef groupRef,
                       size_t off,
                       uint8_t *b,
                       size_t len)
{
    return __EFPageGroupXfer(groupRef, off, b, len, kVPXferRead);
}
