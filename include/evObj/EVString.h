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

#ifndef EVSTRING_H
#define EVSTRING_H

#include <stdint.h>
#include <stdbool.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/EVArray.h>

typedef enum: uint8_t {
    kEVStringEncodingUTF7,
    kEVStringEncodingASCII = kEVStringEncodingUTF7,
    kEVStringEncodingUTF8,
} kEVStringEncoding;

typedef EVObjectRef EVStringRef;

typedef struct __EVString {
    EVObject header;
    kEVStringEncoding encoding;
    bool is_inlined;    /* meaning the object has the string buffer in it self */
    char *buf;
    size_t len;
} *__EVString;

#define EV_STR(cStr) EV_STR_ENC(cStr, kEVStringEncodingUTF8)

#define EV_STR_ENC(cStr, enc) (__extension__ ({ \
    static struct __EVString _evk = { \
        .encoding = (enc), \
        .is_inlined = false, \
        .buf = (char *)("" cStr ""), \
        .len = sizeof("" cStr "") - 1, \
        .header.is_stack_obj = true, \
    }; \
    (EVStringRef)&_evk; \
}))

EVTypeID EVStringGetTypeID(void);

EVStringRef EVStringCreateWithCString(EVAllocatorRef allocatorRef, const char *str, kEVStringEncoding encoding);
EVStringRef EVStringCreateWithCStringNoCopy(EVAllocatorRef allocatorRef, const char *str, kEVStringEncoding encoding);
EVStringRef EVStringCreateWithCBuffer(EVAllocatorRef allocatorRef, const uint8_t *buf, size_t len, kEVStringEncoding encoding);
/*
 * EVStringRef EVStringCreateWithCBufferNoCopy(EVAllocatorRef allocatorRef, uint8_t *buf, size_t len, kEVStringEncoding encoding);
 */
EVStringRef EVStringCreateCopy(EVAllocatorRef allocatorRef, EVStringRef stringRef);

const char *EVStringGetCStringPtr(EVStringRef stringRef, kEVStringEncoding encoding);
size_t EVStringGetLength(EVStringRef stringRef);
bool EVStringGetCString(EVStringRef stringRef, char *str, size_t str_len, kEVStringEncoding encoding);

EVArrayRef EVStringComponentsSplitBySeparator(EVStringRef stringRef, EVStringRef separatorStringRef);

#endif /* EVSTRING_H */
