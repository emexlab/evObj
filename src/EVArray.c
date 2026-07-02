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

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <evObj/EVArray.h>
#include <evObj/EVString.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>

typedef struct EVArray {
    EVObject header;

    bool mutable;
    EVArrayCallbacks callbacks;

    uint64_t items_cap;
    uint64_t items_cnt;
    void **items;
} *EVArray;

EVArrayCallbacks kEVArrayCallbacksDefaultCallbacks = &(struct EVArrayCallbacks){
    .append = NULL,
    .remove = NULL,
    .equal = NULL,
    .copyDescription = NULL,
};

static bool __EVArrayAppendObjectCallback(void *ptr)
{
    EVObjectRef ref = EVRetain((EVObjectRef)ptr);
    return (ref != NULL);
}

static void __EVArrayRemoveObjectCallback(void *ptr)
{
    EVRelease((EVObjectRef)ptr);
}

static bool __EVArrayEqualObjectCallback(void *ptr1,
                                         void *ptr2)
{
    return EVEqual((EVObjectRef)ptr1, (EVObjectRef)ptr2);
}

static EVStringRef __EVArrayCopyDescriptionObjectCallback(EVAllocatorRef allocatorRef,
                                                          void *ptr)
{
    return EVStringCreateWithFormat(allocatorRef, EV_STR("%@"), (EVObjectRef)ptr);
}

EVArrayCallbacks kEVArrayCallbacksObjectCallbacks = &(struct EVArrayCallbacks){
    .append = __EVArrayAppendObjectCallback,
    .remove = __EVArrayRemoveObjectCallback,
    .equal = __EVArrayEqualObjectCallback,
    .copyDescription = __EVArrayCopyDescriptionObjectCallback,
};

static void __EVArrayClassDeinit(EVArrayRef arrayRef)
{
    EVArray array = (EVArray)arrayRef;
    if(array->callbacks->remove)
    {
        for(uint64_t index = 0; index < array->items_cnt; index++)
        {
            array->callbacks->remove(array->items[index]);
        }
    }
    free(array->items);
}

static Boolean __EVArrayClassEqual(EVArrayRef arrayRef1,
                                   EVArrayRef arrayRef2)
{
    EVArray array1 = (EVArray)arrayRef1;
    EVArray array2 = (EVArray)arrayRef2;

    if(array1->callbacks != array2->callbacks ||
       array1->items_cnt != array2->items_cnt)
    {
        return false;
    }
    EVArrayCallbacks callbacks = array1->callbacks;

    for(uint64_t index = 0; index < array1->items_cnt; index++)
    {
        if(array1->items[index] == array2->items[index])
        {
            continue;
        }

        if(callbacks->equal && !callbacks->equal(array1->items[index], array2->items[index]))
        {
            return false;
        }
    }

    return true;
}

static EVStringRef __EVArrayCopyDescription(EVArrayRef arrayRef)
{
    EVArray array = (EVArray)arrayRef;
    EVAllocatorRef allocatorRef = EVGetAllocator(arrayRef);
    EVClass *cls = EVClassGetByID(array->header.typeID);

    EVStringRef baseStringRef = EVStringCreateWithFormat(allocatorRef, EV_STR("<%s %p>{count = %llu, items = {"), cls->name, arrayRef, array->items_cnt);
    if(baseStringRef == NULL)
    {
        return NULL;
    }

    EVMutableStringRef mutableStringRef = EVStringCreateMutableCopy(allocatorRef, baseStringRef);
    EVRelease(baseStringRef);
    if(mutableStringRef == NULL)
    {
        return NULL;
    }

    for(uint64_t index = 0; index < array->items_cnt; index++)
    {
        if(index > 0)
        {
            if(!EVStringAppendString(mutableStringRef, EV_STR(", ")))
            {
                EVRelease(mutableStringRef);
                return NULL;
            }
        }

        void *ptr = EVArrayGetValueAtIndex(arrayRef, index);
        EVStringRef stringRef = NULL;
        if(array->callbacks->copyDescription)
        {
            stringRef = array->callbacks->copyDescription(allocatorRef, ptr);
        }

        if(stringRef == NULL)
        {
            stringRef = EVStringCreateWithFormat(allocatorRef, EV_STR("%p"), ptr);
        }

        if(stringRef == NULL)
        {
            EVRelease(mutableStringRef);
            return NULL;
        }

        bool success = EVStringAppendString(mutableStringRef, stringRef);
        EVRelease(stringRef);
        if(!success)
        {
            EVRelease(mutableStringRef);
            return NULL;
        }
    }

    if(!EVStringAppendString(mutableStringRef, EV_STR("}}")))
    {
        EVRelease(mutableStringRef);
        return NULL;   
    }

    return mutableStringRef;
}

static EVClass EVArrayClass = {
    .name = "EVArray",
    .typeID = kEVNotATypeID,
    .init = NULL,
    .deinit = __EVArrayClassDeinit,
    .equal = __EVArrayClassEqual,
    .copyDescription = __EVArrayCopyDescription,
};

static void EVArrayRegisterClass(void)
{
    EVClassRegister(&EVArrayClass);
}

EVTypeID EVArrayGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EVArrayRegisterClass);
    return EVArrayClass.typeID;
}

EVMutableArrayRef EVArrayCreateMutable(EVAllocatorRef allocatorRef,
                                       EVArrayCallbacks callbacks,
                                       uint64_t capacity)
{
    if(callbacks == NULL)
    {
        callbacks = kEVArrayCallbacksDefaultCallbacks;
    }

    void *items = NULL; /* freeing NULL is allowed as a UNIX semantic */
    if(capacity > 0)
    {
        items = calloc(capacity, sizeof(void*));
        if(items == NULL)
        {
            return NULL;
        }
    }

    EVArray array = (EVArray)EVObjectAlloc(allocatorRef, EVArrayGetTypeID(), sizeof(struct EVArray));
    if(array == NULL)
    {
        free(items);
        return NULL;
    }

    array->mutable = true;
    array->callbacks = callbacks;
    array->items_cnt = 0;
    array->items_cap = capacity;
    array->items = items;

    return (EVMutableArrayRef)array;
}

static EVArrayRef __EVArrayCreateCopy(EVAllocatorRef allocatorRef,
                                      EVArrayRef arrayRef,
                                      bool mutable)
{
    if(arrayRef == NULL)
    {
        return NULL;
    }

    EVArray srcArray = (EVArray)arrayRef;
    EVMutableArrayRef copyArrayRef = EVArrayCreateMutable(allocatorRef, srcArray->callbacks, srcArray->items_cnt);
    if(copyArrayRef == NULL)
    {
        return NULL;
    }

    for(uint64_t index = 0; index < srcArray->items_cnt; index++)
    {
        if(!EVArrayAppendValue(copyArrayRef, srcArray->items[index]))
        {
            EVRelease((EVObjectRef)copyArrayRef);
            return NULL;
        }
    }

    ((EVArray)copyArrayRef)->mutable = mutable;

    return (EVArrayRef)copyArrayRef;
}

EVMutableArrayRef EVArrayCreateMutableCopy(EVAllocator *allocator,
                                           EVArrayRef arrayRef)
{
    return (EVMutableArrayRef)__EVArrayCreateCopy(allocator, arrayRef, true);
}

EVArrayRef EVArrayCreateCopy(EVAllocator *allocator,
                             EVArrayRef arrayRef)
{
    return (EVMutableArrayRef)__EVArrayCreateCopy(allocator, arrayRef, false);
}

uint64_t EVArrayGetCount(EVArrayRef arrayRef)
{
    if(arrayRef == NULL)
    {
        return 0;
    }

    EVArray array = (EVArray)arrayRef;
    return array->items_cnt;
}

void *EVArrayGetValueAtIndex(EVArrayRef arrayRef,
                             uint64_t index)
{
    if(arrayRef == NULL)
    {
        return NULL;
    }

    /* wrap around check */
    EVArray array = (EVArray)arrayRef;
    if(array->items_cnt <= index)
    {
        return NULL;
    }

    return array->items[index];
}

bool __EVArrayResizeIfNeededForOneMoreIndex(EVArray array)
{
    uint64_t needed_cap = array->items_cnt + 1;
    if(array->items_cap >= needed_cap)
    {
        return true;
    }

    /* need to reallocate */
    uint64_t new_cap = array->items_cap + 5;
    if(new_cap < array->items_cap)
    {
        /* wrapped around */
        return false;
    }

    /* actual reallocation */
    void *new_ptr = realloc(array->items, new_cap * sizeof(void*));
    if(new_ptr == NULL)
    {
        return false;
    }
    array->items = new_ptr;
    array->items_cap = new_cap;

    return true;
}

bool EVArrayAppendValue(EVArrayRef arrayRef,
                        void *ptr)
{
    EVArray array = (EVArray)arrayRef;
    if(arrayRef == NULL || ptr == NULL || !array->mutable || array->items_cnt >= UINT64_MAX || !__EVArrayResizeIfNeededForOneMoreIndex(array))
    {
        return false;
    }

    if(array->callbacks->append != NULL)
    {
        if(!array->callbacks->append(ptr))
        {
            return false;
        }
    }

    /* append */
    uint64_t idx = (array->items_cnt)++;
    array->items[idx] = ptr;

    return true;
}

bool EVArrayInsertValueAtIndex(EVArrayRef arrayRef,
                               uint64_t index,
                               void *ptr)
{
    EVArray array = (EVArray)arrayRef;
    if(array == NULL || ptr == NULL || !array->mutable || index > array->items_cnt || array->items_cnt >= UINT64_MAX || !__EVArrayResizeIfNeededForOneMoreIndex(array))
    {
        return false;
    }

    if(array->callbacks->append != NULL)
    {
        if(!array->callbacks->append(ptr))
        {
            return false;
        }
    }

    /* insert */
    memmove(&array->items[index + 1], &array->items[index], (array->items_cnt - index) * sizeof(void*));
    array->items[index] = ptr;
    array->items_cnt++;

    return true;
}

void EVArrayRemoveValueAtIndex(EVArrayRef arrayRef,
                               uint64_t index)
{
    EVArray array = (EVArray)arrayRef;
    if(array == NULL || !array->mutable || index >= array->items_cnt)
    {
        return;
    }

    if(array->callbacks->remove != NULL)
    {
        array->callbacks->remove(array->items[index]);
    }

    memmove(&array->items[index], &array->items[index + 1], (array->items_cnt - index - 1) * sizeof(void*));
    array->items_cnt--;
}
