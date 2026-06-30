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

#ifndef EVARRAY_H
#define EVARRAY_H

#include <stdint.h>
#include <stdbool.h>
#include <evObj/runtime/EVBase.h>

typedef EVObjectRef EVArrayRef;
typedef EVObjectRef EVMutableArrayRef;

typedef bool (*evarray_append_callback)(void *ptr);
typedef void (*evarray_remove_callback)(void *ptr);
typedef bool (*evarray_equal_callback)(void *ptr1, void *ptr2);

typedef struct EVArrayCallbacks {
    evarray_append_callback append;
    evarray_remove_callback remove;
    evarray_equal_callback equal;
} *EVArrayCallbacks;

extern EVArrayCallbacks kEVArrayCallbacksDefaultCallbacks;
extern EVArrayCallbacks kEVArrayCallbacksObjectCallbacks;

EVTypeID EVArrayGetTypeID(void);

EVMutableArrayRef EVArrayCreateMutable(EVAllocatorRef allocatorRef, EVArrayCallbacks callbacks, uint64_t capacity);

EVMutableArrayRef EVArrayCreateMutableCopy(EVAllocator *allocator, EVArrayRef arrayRef);
EVArrayRef EVArrayCreateCopy(EVAllocator *allocator, EVArrayRef arrayRef);

uint64_t EVArrayGetCount(EVArrayRef arrayRef);
void *EVArrayGetValueAtIndex(EVArrayRef arrayRef, uint64_t index);

bool EVArrayAppendValue(EVArrayRef arrayRef, void *value);

bool EVArrayInsertValueAtIndex(EVArrayRef arrayRef, uint64_t index, void *ptr);
void EVArrayRemoveValueAtIndex(EVArrayRef arrayRef, uint64_t index);

#endif /* EVARRAY_H */
