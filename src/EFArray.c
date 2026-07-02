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
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <EmexFoundation/EFArray.h>
#include <EmexFoundation/EFString.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>

typedef struct EFArray {
    EFObject header;

    Boolean isMutable;
    EFArrayCallbacks callbacks;

    EFIndex items_cap;
    EFIndex items_cnt;
    void **items;
} *EFArray;

EFArrayCallbacks kEFArrayCallbacksDefaultCallbacks = &(struct EFArrayCallbacks){
    .append = NULL,
    .remove = NULL,
    .equal = NULL,
    .copyDescription = NULL,
};

static Boolean __EFArrayAppendObjectCallback(void *ptr)
{
    EFObjectRef ref = EFRetain((EFObjectRef)ptr);
    return (ref != NULL);
}

static void __EFArrayRemoveObjectCallback(void *ptr)
{
    EFRelease((EFObjectRef)ptr);
}

static Boolean __EFArrayEqualObjectCallback(void *ptr1,
                                            void *ptr2)
{
    return EFEqual((EFObjectRef)ptr1, (EFObjectRef)ptr2);
}

static EFStringRef __EFArrayCopyDescriptionObjectCallback(EFAllocatorRef allocatorRef,
                                                          void *ptr)
{
    return EFStringCreateWithFormat(allocatorRef, EF_STR("%@"), (EFObjectRef)ptr);
}

EFArrayCallbacks kEFArrayCallbacksObjectCallbacks = &(struct EFArrayCallbacks){
    .append = __EFArrayAppendObjectCallback,
    .remove = __EFArrayRemoveObjectCallback,
    .equal = __EFArrayEqualObjectCallback,
    .copyDescription = __EFArrayCopyDescriptionObjectCallback,
};

static void __EFArrayClassDeinit(EFArrayRef arrayRef)
{
    EFArray array = (EFArray)arrayRef;
    if(array->callbacks->remove)
    {
        for(EFIndex index = 0; index < array->items_cnt; index++)
        {
            array->callbacks->remove(array->items[index]);
        }
    }
    free(array->items);
}

static Boolean __EFArrayClassEqual(EFArrayRef arrayRef1,
                                   EFArrayRef arrayRef2)
{
    EFArray array1 = (EFArray)arrayRef1;
    EFArray array2 = (EFArray)arrayRef2;

    if(array1->callbacks != array2->callbacks ||
       array1->items_cnt != array2->items_cnt)
    {
        return false;
    }
    EFArrayCallbacks callbacks = array1->callbacks;

    for(EFIndex index = 0; index < array1->items_cnt; index++)
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

static EFStringRef __EFArrayCopyDescription(EFArrayRef arrayRef)
{
    EFArray array = (EFArray)arrayRef;
    EFAllocatorRef allocatorRef = EFGetAllocator(arrayRef);
    EFClass *cls = EFClassGetByID(array->header.typeID);

    EFStringRef baseStringRef = EFStringCreateWithFormat(allocatorRef, EF_STR("<%s %p>{count = %ld, items = {"), cls->name, arrayRef, array->items_cnt);
    if(baseStringRef == NULL)
    {
        return NULL;
    }

    EFMutableStringRef mutableStringRef = EFStringCreateMutableCopy(allocatorRef, baseStringRef);
    EFRelease(baseStringRef);
    if(mutableStringRef == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < array->items_cnt; index++)
    {
        if(index > 0)
        {
            if(!EFStringAppendString(mutableStringRef, EF_STR(", ")))
            {
                EFRelease(mutableStringRef);
                return NULL;
            }
        }

        void *ptr = EFArrayGetValueAtIndex(arrayRef, index);
        EFStringRef stringRef = NULL;
        if(array->callbacks->copyDescription)
        {
            stringRef = array->callbacks->copyDescription(allocatorRef, ptr);
        }

        if(stringRef == NULL)
        {
            stringRef = EFStringCreateWithFormat(allocatorRef, EF_STR("%p"), ptr);
        }

        if(stringRef == NULL)
        {
            EFRelease(mutableStringRef);
            return NULL;
        }

        Boolean success = EFStringAppendString(mutableStringRef, stringRef);
        EFRelease(stringRef);
        if(!success)
        {
            EFRelease(mutableStringRef);
            return NULL;
        }
    }

    if(!EFStringAppendString(mutableStringRef, EF_STR("}}")))
    {
        EFRelease(mutableStringRef);
        return NULL;   
    }

    return mutableStringRef;
}

static EFClass EFArrayClass = {
    .name = "EFArray",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFArrayClassDeinit,
    .equal = __EFArrayClassEqual,
    .copyDescription = __EFArrayCopyDescription,
};

static void EFArrayRegisterClass(void)
{
    EFClassRegister(&EFArrayClass);
}

EFTypeID EFArrayGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFArrayRegisterClass);
    return EFArrayClass.typeID;
}

EFMutableArrayRef EFArrayCreateMutable(EFAllocatorRef allocatorRef,
                                       EFArrayCallbacks callbacks,
                                       uint64_t capacity)
{
    if(callbacks == NULL)
    {
        callbacks = kEFArrayCallbacksDefaultCallbacks;
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

    EFArray array = (EFArray)EFObjectAlloc(allocatorRef, EFArrayGetTypeID(), sizeof(struct EFArray));
    if(array == NULL)
    {
        free(items);
        return NULL;
    }

    array->isMutable = true;
    array->callbacks = callbacks;
    array->items_cnt = 0;
    array->items_cap = capacity;
    array->items = items;

    return (EFMutableArrayRef)array;
}

static EFArrayRef __EFArrayCreateCopy(EFAllocatorRef allocatorRef,
                                      EFArrayRef arrayRef,
                                      Boolean isMutable)
{
    if(arrayRef == NULL)
    {
        return NULL;
    }

    EFArray srcArray = (EFArray)arrayRef;
    EFMutableArrayRef copyArrayRef = EFArrayCreateMutable(allocatorRef, srcArray->callbacks, srcArray->items_cnt);
    if(copyArrayRef == NULL)
    {
        return NULL;
    }

    for(EFIndex index = 0; index < srcArray->items_cnt; index++)
    {
        if(!EFArrayAppendValue(copyArrayRef, srcArray->items[index]))
        {
            EFRelease((EFObjectRef)copyArrayRef);
            return NULL;
        }
    }

    ((EFArray)copyArrayRef)->isMutable = isMutable;

    return (EFArrayRef)copyArrayRef;
}

EFMutableArrayRef EFArrayCreateMutableCopy(EFAllocator *allocator,
                                           EFArrayRef arrayRef)
{
    return (EFMutableArrayRef)__EFArrayCreateCopy(allocator, arrayRef, true);
}

EFArrayRef EFArrayCreateCopy(EFAllocator *allocator,
                             EFArrayRef arrayRef)
{
    return (EFMutableArrayRef)__EFArrayCreateCopy(allocator, arrayRef, false);
}

EFIndex EFArrayGetCount(EFArrayRef arrayRef)
{
    if(arrayRef == NULL)
    {
        return 0;
    }

    EFArray array = (EFArray)arrayRef;
    return array->items_cnt;
}

void *EFArrayGetValueAtIndex(EFArrayRef arrayRef,
                             EFIndex index)
{
    if(arrayRef == NULL)
    {
        return NULL;
    }

    /* wrap around check */
    EFArray array = (EFArray)arrayRef;
    if(array->items_cnt <= index)
    {
        return NULL;
    }

    return array->items[index];
}

Boolean __EFArrayResizeIfNeededForOneMoreIndex(EFArray array)
{
    EFIndex needed_cap = array->items_cnt + 1;
    if(array->items_cap >= needed_cap)
    {
        return true;
    }

    /* need to reallocate */
    EFIndex new_cap = array->items_cap + 5;
    if(new_cap < array->items_cap)
    {
        /* wrapped around */
        return false;
    }

    /* actual reallocation */
    void *new_ptr = realloc(array->items, (size_t)(new_cap * sizeof(void*)));
    if(new_ptr == NULL)
    {
        return false;
    }
    array->items = new_ptr;
    array->items_cap = new_cap;

    return true;
}

Boolean EFArrayAppendValue(EFArrayRef arrayRef,
                           void *ptr)
{
    EFArray array = (EFArray)arrayRef;
    if(arrayRef == NULL || ptr == NULL || !array->isMutable || array->items_cnt >= __LONG_MAX__ || !__EFArrayResizeIfNeededForOneMoreIndex(array))
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
    EFIndex idx = (array->items_cnt)++;
    array->items[idx] = ptr;

    return true;
}

Boolean EFArrayInsertValueAtIndex(EFArrayRef arrayRef,
                                  EFIndex index,
                                  void *ptr)
{
    EFArray array = (EFArray)arrayRef;
    if(array == NULL || ptr == NULL || !array->isMutable || index > array->items_cnt || array->items_cnt >= __LONG_MAX__ || !__EFArrayResizeIfNeededForOneMoreIndex(array))
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
    memmove(&array->items[index + 1], &array->items[index], (size_t)((array->items_cnt - index) * sizeof(void*)));
    array->items[index] = ptr;
    array->items_cnt++;

    return true;
}

void EFArrayRemoveValueAtIndex(EFArrayRef arrayRef,
                               EFIndex index)
{
    EFArray array = (EFArray)arrayRef;
    if(array == NULL || !array->isMutable || index >= array->items_cnt)
    {
        return;
    }

    if(array->callbacks->remove != NULL)
    {
        array->callbacks->remove(array->items[index]);
    }

    memmove(&array->items[index], &array->items[index + 1], (size_t)((array->items_cnt - index - 1) * sizeof(void*)));
    array->items_cnt--;
}
