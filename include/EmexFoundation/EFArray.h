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

#ifndef EFARRAY_H
#define EFARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <EmexFoundation/runtime/EFBase.h>

typedef EFObjectRef EFArrayRef;
typedef EFObjectRef EFMutableArrayRef;
typedef EFObjectRef EFStringRef;

typedef bool (*evarray_append_callback)(void *ptr);
typedef void (*evarray_remove_callback)(void *ptr);
typedef bool (*evarray_equal_callback)(void *ptr1, void *ptr2);
typedef EFStringRef (*evarray_copyDescription_callback)(EFAllocatorRef allocatorRef, void *ptr);

typedef struct EFArrayCallbacks {
    evarray_append_callback append;
    evarray_remove_callback remove;
    evarray_equal_callback equal;
    evarray_copyDescription_callback copyDescription;
} *EFArrayCallbacks;

extern EFArrayCallbacks kEFArrayCallbacksDefaultCallbacks;
extern EFArrayCallbacks kEFArrayCallbacksObjectCallbacks;

EFTypeID EFArrayGetTypeID(void);

EFMutableArrayRef EFArrayCreateMutable(EFAllocatorRef allocatorRef, EFArrayCallbacks callbacks, uint64_t capacity);

EFMutableArrayRef EFArrayCreateMutableCopy(EFAllocator *allocator, EFArrayRef arrayRef);
EFArrayRef EFArrayCreateCopy(EFAllocator *allocator, EFArrayRef arrayRef);

uint64_t EFArrayGetCount(EFArrayRef arrayRef);
void *EFArrayGetValueAtIndex(EFArrayRef arrayRef, uint64_t index);

bool EFArrayAppendValue(EFArrayRef arrayRef, void *value);

bool EFArrayInsertValueAtIndex(EFArrayRef arrayRef, uint64_t index, void *ptr);
void EFArrayRemoveValueAtIndex(EFArrayRef arrayRef, uint64_t index);

#endif /* EFARRAY_H */
