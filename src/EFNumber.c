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
#include <assert.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>
#include <EmexFoundation/EFNumber.h>

typedef struct EFNumber {
    EFObject header;
    kEFNumberType type;
    union {
        int8_t s8;
        int16_t s16;
        int32_t s32;
        int64_t s64;
    };
} *EFNumber;

static int64_t __EFNumberGetSInt64Value(EFNumber number)
{
    if(number == NULL)
    {
        return 0;
    }
    
    switch(number->type)
    {
        case kEFNumberTypeSInt8:
            return (int64_t)number->s8;
        case kEFNumberTypeSInt16:
            return(int64_t)number->s16;
        case kEFNumberTypeSInt32:
            return(int64_t)number->s32;
        case kEFNumberTypeSInt64:
            return(int64_t)number->s64;
        default:
            return 0;
    }
}

static Boolean __EFNumberEqual(EFNumberRef ref1,
                               EFNumberRef ref2)
{
    EFNumber a = (EFNumber)ref1;
    EFNumber b = (EFNumber)ref2;
    
    if(a->type != b->type)
    {
        /* not the same type of number =3 */
        return false;
    }

    /* comparing the number it self */
    switch(a->type)
    {
        case kEFNumberTypeSInt8:
            return a->s8 == b->s8;
        case kEFNumberTypeSInt16:
            return a->s16 == b->s16;
        case kEFNumberTypeSInt32:
            return a->s32 == b->s32;
        case kEFNumberTypeSInt64:
            return a->s64 == b->s64;
        default:
            return false;
    }
}

static EFClass EFNumberClass = {
    .name = "EFNumber",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = __EFNumberEqual,
    .copyDescription = NULL,
};

static void EFNumberRegisterClass(void)
{
    EFClassRegister(&EFNumberClass);
}

EFTypeID EFNumberGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFNumberRegisterClass);
    return EFNumberClass.typeID;
}

EFNumberRef EFNumberCreate(EFAllocatorRef allocatorRef,
                           kEFNumberType type,
                           const void *value)
{
    if(value == NULL)
    {
        return NULL;
    }

    EFNumber num = EFObjectAlloc(allocatorRef, EFNumberGetTypeID(), sizeof(struct EFNumber));
    if(num == NULL)
    {
        return NULL;
    }

    switch(type)
    {
        case kEFNumberTypeSInt8:
            num->s8  = *(const int8_t *)value;
            break;
        case kEFNumberTypeSInt16:
            num->s16 = *(const int16_t *)value;
            break;
        case kEFNumberTypeSInt32:
            num->s32 = *(const int32_t *)value;
            break;
        case kEFNumberTypeSInt64:
            num->s64 = *(const int64_t *)value;
            break;
        default:
            /* possibly bad type */
            EFRelease(num);
            return NULL;
    }

    return num;
}

EFIndex EFNumberGetByteSize(EFNumberRef numberRef)
{
    if(numberRef == NULL)
    {
        return 0;
    }

    EFNumber num = (EFNumber)numberRef;
    switch(num->type)
    {
        case kEFNumberTypeSInt8:
            return sizeof(uint8_t);
        case kEFNumberTypeSInt16:
            return sizeof(uint16_t);
        case kEFNumberTypeSInt32:
            return sizeof(uint32_t);
        case kEFNumberTypeSInt64:
            return sizeof(uint64_t);
        default:
            return 0;
    }
}

kEFNumberType EFNumberGetType(EFNumberRef numberRef)
{
    if(numberRef == NULL)
    {
        return kEFNumberTypeSInt8;
    }

    EFNumber num = (EFNumber)numberRef;
    return num->type;
}

Boolean EFNumberGetValue(EFNumberRef numberRef,
                         kEFNumberType type,
                         void *value)
{
    if(numberRef == NULL || value == NULL)
    {
        return false;
    }

    EFNumber num = (EFNumber)numberRef;
    switch(num->type)
    {
        case kEFNumberTypeSInt8:
            *(int8_t *)value = (uint8_t)__EFNumberGetSInt64Value(num);
            return true;
        case kEFNumberTypeSInt16:
            *(int16_t *)value = (uint16_t)__EFNumberGetSInt64Value(num);
            return true;
        case kEFNumberTypeSInt32:
            *(int32_t *)value = (uint32_t)__EFNumberGetSInt64Value(num);
            return true;
        case kEFNumberTypeSInt64:
            *(int64_t *)value = (uint64_t)__EFNumberGetSInt64Value(num);
            return true;
        default:
            return false;
    }
}

kEFNumberComparisonResult EFNumberCompare(EFNumberRef numberRef,
                                          EFNumberRef otherNumberRef)
{
    EFNumber num = (EFNumber)numberRef;
    EFNumber otherNum = (EFNumber)otherNumberRef;

    if(num == NULL || otherNum == NULL)
    {
        /* nothing to compare */
        return kEFNumberComparisonResultEqualTo;
    }

    int64_t int1 = __EFNumberGetSInt64Value(num);
    int64_t int2 = __EFNumberGetSInt64Value(otherNum);

    return (int1 == int2) ? kEFNumberComparisonResultEqualTo : (int1 > int2) ? kEFNumberComparisonResultGreaterThan : kEFNumberComparisonResultLessThan;
}
