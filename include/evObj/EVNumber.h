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

#ifndef EVNUMBER_H
#define EVNUMBER_H

#include <stdint.h>
#include <stdbool.h>
#include <evObj/runtime/EVBase.h>

typedef enum: uint8_t {
    kEVNumberTypeSInt8,
    kEVNumberTypeSInt16,
    kEVNumberTypeSInt32,
    kEVNumberTypeSInt64,
} kEVNumberType;

typedef enum: uint8_t {
    kEVNumberComparisonResultLessThan,
    kEVNumberComparisonResultEqualTo,
    kEVNumberComparisonResultGreaterThan,
} kEVNumberComparisonResult;

typedef EVObjectRef EVNumberRef;

EVTypeID EVNumberGetTypeID(void);

EVNumberRef EVNumberCreate(EVAllocatorRef allocatorRef, kEVNumberType type, const void *value);

int EVNumberGetByteSize(EVNumberRef numberRef);
kEVNumberType EVNumberGetType(EVNumberRef numberRef);
bool EVNumberGetValue(EVNumberRef numberRef, kEVNumberType type, void *value);

kEVNumberComparisonResult EVNumberCompare(EVNumberRef numberRef, EVNumberRef otherNumberRef);

#endif /* EVNUMBER_H */
