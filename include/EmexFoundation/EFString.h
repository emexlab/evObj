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

#ifndef EFSTRING_H
#define EFSTRING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/EFArray.h>

typedef enum: uint8_t {
    kEFStringEncodingUTF7,
    kEFStringEncodingASCII = kEFStringEncodingUTF7,
    kEFStringEncodingUTF8,
} kEFStringEncoding;

typedef EFObjectRef EFStringRef;
typedef EFObjectRef EFMutableStringRef;

typedef struct __EFString {
    EFObject header;
    kEFStringEncoding encoding;
    Boolean is_mutable;
    Boolean is_inlined; /* meaning the buffer pointer points to after the string object */
    char *buf;          /* it is neither inlined nor undeallocatable if mutable */
    EFIndex len;
} *__EFString;

#define EF_STR(cStr) EF_STR_ENC(cStr, kEFStringEncodingUTF8)

#define EF_STR_ENC(cStr, enc) (__extension__ ({ \
    static struct __EFString _evk = { \
        .encoding = (enc), \
        .is_mutable = false, \
        .is_inlined = false, \
        .buf = (char *)("" cStr ""), \
        .len = (EFIndex)(sizeof("" cStr "") - 1), \
        .header.is_stack_obj = true, \
    }; \
    _evk.header.typeID = EFStringGetTypeID(), \
    (EFStringRef)&_evk; \
}))

EFTypeID EFStringGetTypeID(void);

EFStringRef EFStringCreateWithCBuffer(EFAllocatorRef allocatorRef, const uint8_t *buf, size_t len, kEFStringEncoding encoding);
EFStringRef EFStringCreateWithCBufferNoCopy(EFAllocatorRef allocatorRef, const uint8_t *buf, size_t len, kEFStringEncoding encoding);
EFStringRef EFStringCreateWithCString(EFAllocatorRef allocatorRef, const char *str, kEFStringEncoding encoding);
EFStringRef EFStringCreateWithCStringNoCopy(EFAllocatorRef allocatorRef, const char *str, kEFStringEncoding encoding);
EFStringRef EFStringCreateWithFormatAndArguments(EFAllocatorRef allocatorRef, EFStringRef format, va_list arguments);
EFStringRef EFStringCreateWithFormat(EFAllocatorRef allocatorRef, EFStringRef format, ...);
EFStringRef EFStringCreateCopy(EFAllocatorRef allocatorRef, EFStringRef stringRef);
EFMutableStringRef EFStringCreateMutableCopy(EFAllocatorRef allocatorRef, EFStringRef stringRef);

const char *EFStringGetCStringPtr(EFStringRef stringRef, kEFStringEncoding encoding);
EFIndex EFStringGetLength(EFStringRef stringRef);
Boolean EFStringGetCString(EFStringRef stringRef, char *str, size_t str_len, kEFStringEncoding encoding);

EFArrayRef EFStringComponentsSplitBySeparator(EFStringRef stringRef, EFStringRef separatorStringRef);

Boolean EFStringTrimWhitespace(EFMutableStringRef mutableStringRef);
Boolean EFStringAppendString(EFMutableStringRef mutableStringRef, EFStringRef stringRef);

#endif /* EFSTRING_H */
