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
#include <assert.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>
#include <evObj/EVNumber.h>

typedef struct EVNumber {
    EVObject header;
    kEVNumberType type;
    union {
        int8_t s8;
        int16_t s16;
        int32_t s32;
        int64_t s64;
    };
} *EVNumber;

static int64_t __EVNumberGetSInt64Value(EVNumber number)
{
    if(number == NULL)
    {
        return 0;
    }
    
    switch(number->type)
    {
        case kEVNumberTypeSInt8:
            return (int64_t)number->s8;
        case kEVNumberTypeSInt16:
            return(int64_t)number->s16;
        case kEVNumberTypeSInt32:
            return(int64_t)number->s32;
        case kEVNumberTypeSInt64:
            return(int64_t)number->s64;
        default:
            return 0;
    }
}

static bool __EVNumberEqual(EVNumberRef ref1,
                            EVNumberRef ref2)
{
    EVNumber a = (EVNumber)ref1;
    EVNumber b = (EVNumber)ref2;
    
    if(a->type != b->type)
    {
        /* not the same type of number =3 */
        return false;
    }

    /* comparing the number it self */
    switch(a->type)
    {
        case kEVNumberTypeSInt8:
            return a->s8 == b->s8;
        case kEVNumberTypeSInt16:
            return a->s16 == b->s16;
        case kEVNumberTypeSInt32:
            return a->s32 == b->s32;
        case kEVNumberTypeSInt64:
            return a->s64 == b->s64;
        default:
            return false;
    }
}

static EVClass EVNumberClass = {
    .name = "EVNumber",
    .typeID = kEVNotATypeID,
    .size = sizeof(struct EVNumber),
    .init = NULL,
    .deinit = NULL,
    .copy = NULL,
    .equal = __EVNumberEqual,
};

static void EVNumberRegisterClass(void)
{
    EVClassRegister(&EVNumberClass);
}

EVTypeID EVNumberGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EVNumberRegisterClass);
    return EVNumberClass.typeID;
}

EVNumberRef EVNumberCreate(EVAllocator *allocator,
                           kEVNumberType type,
                           const void *value)
{
    if(value == NULL)
    {
        return NULL;
    }

    EVNumber num = EVObjectAlloc(allocator, EVNumberGetTypeID());
    if(num == NULL)
    {
        return NULL;
    }

    switch(type)
    {
        case kEVNumberTypeSInt8:
            num->s8  = *(const int8_t *)value;
            break;
        case kEVNumberTypeSInt16:
            num->s16 = *(const int16_t *)value;
            break;
        case kEVNumberTypeSInt32:
            num->s32 = *(const int32_t *)value;
            break;
        case kEVNumberTypeSInt64:
            num->s64 = *(const int64_t *)value;
            break;
        default:
            /* possibly bad type */
            EVRelease(num);
            return NULL;
    }

    return num;
}

int EVNumberGetByteSize(EVNumberRef numberRef)
{
    if(numberRef == NULL)
    {
        return 0;
    }

    EVNumber num = (EVNumber)numberRef;
    switch(num->type)
    {
        case kEVNumberTypeSInt8:
            return sizeof(uint8_t);
        case kEVNumberTypeSInt16:
            return sizeof(uint16_t);
        case kEVNumberTypeSInt32:
            return sizeof(uint32_t);
        case kEVNumberTypeSInt64:
            return sizeof(uint64_t);
        default:
            return 0;
    }
}

kEVNumberType EVNumberGetType(EVNumberRef numberRef)
{
    if(numberRef == NULL)
    {
        return kEVNumberTypeSInt8;
    }

    EVNumber num = (EVNumber)numberRef;
    return num->type;
}

bool EVNumberGetValue(EVNumberRef numberRef,
                      kEVNumberType type,
                      void *value)
{
    if(numberRef == NULL || value == NULL)
    {
        return false;
    }

    EVNumber num = (EVNumber)numberRef;
    switch(num->type)
    {
        case kEVNumberTypeSInt8:
            *(int8_t *)value = (uint8_t)__EVNumberGetSInt64Value(num);
            return true;
        case kEVNumberTypeSInt16:
            *(int16_t *)value = (uint16_t)__EVNumberGetSInt64Value(num);
            return true;
        case kEVNumberTypeSInt32:
            *(int32_t *)value = (uint32_t)__EVNumberGetSInt64Value(num);
            return true;
        case kEVNumberTypeSInt64:
            *(int64_t *)value = (uint64_t)__EVNumberGetSInt64Value(num);
            return true;
        default:
            return false;
    }
}

kEVNumberComparisonResult EVNumberCompare(EVNumberRef numberRef,
                                          EVNumberRef otherNumberRef)
{
    EVNumber num = (EVNumber)numberRef;
    EVNumber otherNum = (EVNumber)otherNumberRef;

    if(num == NULL || otherNum == NULL)
    {
        /* nothing to compare */
        return kEVNumberComparisonResultEqualTo;
    }

    int64_t int1 = __EVNumberGetSInt64Value(num);
    int64_t int2 = __EVNumberGetSInt64Value(otherNum);

    return (int1 == int2) ? kEVNumberComparisonResultEqualTo : (int1 > int2) ? kEVNumberComparisonResultGreaterThan : kEVNumberComparisonResultLessThan;
}
