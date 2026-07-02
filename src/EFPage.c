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

#include <pthread.h>
#include <sys/mman.h>
#include <EmexFoundation/EFPage.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(__unix)
    #include <unistd.h>
#else
    #error "EFGetPageSize: unsupported platform"
#endif

static EFIndex __EFPageLength = 0;

static void __EFPageFindOutPageLength(void)
{
#if defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    __EFPageLength = (EFIndex)si.dwPageSize;
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(__unix)
    long ps = sysconf(_SC_PAGESIZE);
    __EFPageLength = (EFIndex)((ps > 0) ? (EFIndex)ps : 4096); /* fallback, sysconf rarely fails but be defensive */
#endif
}

EFIndex __EFPageGetPageLength(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, __EFPageFindOutPageLength);
    return __EFPageLength;
}

typedef struct EFPage {
    EFObject header;
    UInt8 *mem;
    EFIndex length;
} *EFPage;

static void __EFPageDeinit(EFPageRef pageRef)
{
    EFPage page = (EFPage)pageRef;
    if(page->mem != MAP_FAILED)
    {
        munmap(page->mem, page->length);
    }
}

static EFStringRef __EFPageCopyDescription(EFPageRef pageRef)
{
    EFPage page = (EFPage)pageRef;
    EFAllocatorRef allocatorRef = EFGetAllocator(pageRef);
    EFClass *cls = EFClassGetByID(page->header.typeID);
    return EFStringCreateWithFormat(allocatorRef, EF_STR("<%s %p>{mem = %p, length = %zu}"), cls->name, pageRef, page->mem, page->length);
}

static EFClass EFPageClass = {
    .name = "EFPage",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFPageDeinit,
    .equal = NULL,
    .copyDescription = __EFPageCopyDescription,
};

static void EFPageRegisterClass(void)
{
    EFClassRegister(&EFPageClass);
}

EFTypeID EFPageGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFPageRegisterClass);
    return EFPageClass.typeID;
}

EFPageRef EFPageCreate(EFAllocatorRef allocatorRef)
{
    return EFPageCreateWithOptions(allocatorRef, NULL, __EFPageGetPageLength(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

EFPageRef EFPageCreateWithOptions(EFAllocatorRef allocatorRef,
                                  void *addr,
                                  size_t length,
                                  int prot,
                                  int flags,
                                  int fd,
                                  off_t offset)
{
    EFPage page = (EFPage)EFObjectAlloc(allocatorRef, EFPageGetTypeID(), sizeof(struct EFPage));
    if(page == NULL)
    {
        return NULL;
    }
    
    page->length = length;
    page->mem = mmap(addr, length, prot, flags, fd, offset);
    if(page->mem == MAP_FAILED)
    {
        EFRelease(page);
        return NULL;
    }

    return (EFPageRef)page;
}

EFIndex EFPageGetLength(EFPageRef pageRef)
{
    EFPage page = (EFPage)pageRef;
    if(page == NULL)
    {
        return 0;
    }
    return page->length;
}

void *EFPageGetPtr(EFPageRef pageRef)
{
    EFPage page = (EFPage)pageRef;
    if(page == NULL)
    {
        return NULL;
    }
    return page->mem;
}
