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

#include <stdlib.h>
#include <string.h>
#include <EmexFoundation/EFData.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>

typedef struct EFData {
    EFObject header;
    Boolean isMutable;
    Boolean isInlined;    /* meaning the object has the buffer in it self */
    UInt8 *buffer;       /* it is neither inlined nor undeallocatable if mutable */
    size_t length;
} *EFData;

static void __EFDataInit(EFDataRef dataRef)
{
    EFData data = (EFData)dataRef;

    /* we first automatically expect it to be at the inline */
    data->isMutable = false;
    data->isInlined = true;
    data->buffer = (UInt8*)((const char*)data + sizeof(struct EFData));
}

static void __EFDataDeinit(EFDataRef dataRef)
{
    EFData data = (EFData)dataRef;
    if(data->isMutable)
    {
        free(data->buffer);
    }
}

static EFClass EFDataClass = {
    .name = "EFData",
    .typeID = kEFNotATypeID,
    .init = __EFDataInit,
    .deinit = __EFDataDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

static void EFDataRegisterClass(void)
{
    EFClassRegister(&EFDataClass);
}

EFTypeID EFDataGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFDataRegisterClass);
    return EFDataClass.typeID;
}

EFDataRef EFDataCreateWithCBuffer(EFAllocatorRef allocatorRef,
                                  const UInt8 *bytes,
                                  size_t length)
{
    if(bytes == NULL)
    {
        return NULL;
    }

    EFData data = (EFData)EFObjectAlloc(allocatorRef, EFDataGetTypeID(), sizeof(struct EFData) + length);
    memcpy(data->buffer, bytes, length);
    data->length = length;

    return (EFDataRef)data;
}

EFDataRef EFDataCreateWithCBufferNoCopy(EFAllocatorRef allocatorRef,
                                        const UInt8 *bytes,
                                        size_t length)
{
    if(bytes == NULL)
    {
        return NULL;
    }

    EFData data = (EFData)EFObjectAlloc(allocatorRef, EFDataGetTypeID(), sizeof(struct EFData));
    data->isInlined = false;
    data->buffer = (UInt8*)bytes;
    data->length = length;

    return (EFDataRef)data;
}

EFMutableDataRef EFDataCreateMutable(EFAllocatorRef allocatorRef,
                                     size_t capacity)
{
    return NULL;
}

EFDataRef EFDataCreateCopy(EFAllocatorRef allocatorRef,
                           EFDataRef dataRef)
{
    EFData data = (EFData)dataRef;
    if(data == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        allocatorRef = EFGetAllocator(dataRef);
    }
    
    return EFDataCreateWithCBuffer(allocatorRef, data->buffer, data->length);
}

EFMutableDataRef EFDataCreateMutableCopy(EFAllocatorRef allocatorRef, EFDataRef dataRef)
{
    return NULL;
}
