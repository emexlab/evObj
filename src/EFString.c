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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <EmexFoundation/runtime/EFBase.h>
#include <EmexFoundation/runtime/EFAllocator.h>
#include <EmexFoundation/EFString.h>

typedef __EFString EFString;

static Boolean __EFStringValidateEncoding(kEFStringEncoding encoding,
                                          const char *buffer,
                                          size_t length)
{
    switch(encoding)
    {
        case kEFStringEncodingUTF7:
            for(size_t i = 0; i < length; i++)
            {
                if((unsigned char)buffer[i] > 0x7F)
                {
                    return false;
                }
            }
            return true;
        case kEFStringEncodingUTF8: /* thanks to: https://www.rfc-editor.org/rfc/rfc3629.html */
        {
            size_t i = 0;
            while(i < length)
            {
                unsigned char c = (unsigned char)buffer[i];
                int seqlen;
                unsigned int cp;
                if(c < 0x80)
                {
                    seqlen = 1;
                    cp = c;
                }
                else if((c & 0xE0)==0xC0)
                {
                    seqlen = 2;
                    cp = c & 0x1F;
                }
                else if((c & 0xF0)==0xE0)
                {
                    seqlen = 3;
                    cp = c & 0x0F;
                }
                else if((c & 0xF8)==0xF0)
                {
                    seqlen = 4;
                    cp = c & 0x07;
                }
                else
                {
                    return false;
                }

                if(i + (size_t)seqlen > length)
                {
                    return false;
                }
                for(int k = 1; k < seqlen; ++k)
                {
                    unsigned char cc = (unsigned char)buffer[i+k];
                    if((cc & 0xC0) != 0x80)
                    {
                        return false;
                    }
                    cp = (cp << 6) | (cc & 0x3F);
                }
                static const unsigned int min_cp[5] = { 0, 0, 0x80, 0x800, 0x10000 };
                if(cp < min_cp[seqlen])
                {
                    return false;
                }
                if(cp > 0x10FFFF)
                {
                    return false;
                }
                if(cp >= 0xD800 && cp <= 0xDFFF)
                {
                    return false;
                }
                i += (size_t)seqlen;
            }
            return true;
        }
    }

    return false;
}

static void __EFStringDeinit(EFStringRef stringRef)
{
    EFString string = (EFString)stringRef;
    if(string->is_mutable)
    {
        free(string->buffer);
    }
}

static Boolean __EFStringEqual(EFStringRef stringRef1,
                               EFStringRef stringRef2)
{
    EFString string1 = (EFString)stringRef1;
    EFString string2 = (EFString)stringRef2;

    /* size must match */
    if(string1->length != string2->length)
    {
        return false;
    }

    /* they share the same length */
    size_t length = string1->length;

    /* strings must comply to their encodings */
    if(string1->encoding != string2->encoding)
    {
        Boolean string1_complies = __EFStringValidateEncoding(string2->encoding, string1->buffer, length);
        Boolean string2_complies = __EFStringValidateEncoding(string1->encoding, string2->buffer, length);
        if(!string1_complies || !string2_complies)
        {
            return false;
        }
    }

    return (memcmp(string1->buffer, string2->buffer, length) == 0);
}

static EFStringRef __EFStringCopyDescription(EFStringRef stringRef)
{
    return EFRetain(stringRef); /* just return our selves */
}

static EFClass EFStringClass = {
    .name = "EFString",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __EFStringDeinit,
    .equal = __EFStringEqual,
    .copyDescription = __EFStringCopyDescription,
};

static void EFStringRegisterClass(void)
{
    EFClassRegister(&EFStringClass);
}

EFTypeID EFStringGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EFStringRegisterClass);
    return EFStringClass.typeID;
}

static inline EFStringRef __EFStringCreate(EFAllocatorRef allocatorRef,
                                           const UInt8 *buffer,
                                           size_t length,
                                           kEFStringEncoding encoding,
                                           Boolean is_inlined,
                                           Boolean is_mutable)
{
    if(buffer == NULL)
    {
        return NULL;
    }

    length = strnlen((const char*)buffer, length);
    if(!__EFStringValidateEncoding(encoding, (const char*)buffer, length))
    {
        return NULL;
    }

    EFString string = EFObjectAlloc(allocatorRef, EFStringGetTypeID(), sizeof(struct __EFString) + (is_inlined ? length + 1 : 0));
    if(string == NULL)
    {
        return NULL;
    }

    if(is_mutable)
    {
        string->buffer = malloc(length + 1);
        is_inlined = false; /* must be false */
        goto needs_copy;    /* skips past the string->buffer pointer assignment of a inlined string */
    }
    else if(is_inlined)
    {
        string->buffer = (char*)((const char*)string + sizeof(struct __EFString));
    needs_copy:
        memcpy(string->buffer, buffer, length);
        string->buffer[length] = '\0';
    }
    else
    {
        string->buffer = (char*)buffer;
    }

    /* always assigned with the same values */
    string->length = length;
    string->encoding = encoding;
    string->is_inlined = !is_mutable && is_inlined; /* is_inlined is only possible when is_mutable is not enabled */
    string->is_mutable = is_mutable;

    return (EFStringRef)string;
}

static inline EFStringRef __EFStringCreateWithCString(EFAllocatorRef allocatorRef,
                                                      const char *str,
                                                      kEFStringEncoding encoding,
                                                      Boolean is_inlined)
{
    if(str == NULL)
    {
        return NULL;
    }

    size_t length = strlen(str);
    return __EFStringCreate(allocatorRef, (const UInt8*)str, length, encoding, is_inlined, false);
}

static inline EFStringRef __EFStringCreateCopy(EFAllocatorRef allocatorRef,
                                               EFStringRef stringRef,
                                               Boolean is_mutable)
{
    EFString string = (EFString)stringRef;
    if(string == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        /* falling back to the same allocator used to allocate the source x3 */
        allocatorRef = EFGetAllocator(stringRef);
    }

    return __EFStringCreate(allocatorRef, (const UInt8*)string->buffer, string->length, string->encoding, string->is_inlined, is_mutable);
}

EFStringRef EFStringCreateWithCBuffer(EFAllocatorRef allocatorRef,
                                      const UInt8 *buffer,
                                      size_t length,
                                      kEFStringEncoding encoding)
{
    return __EFStringCreate(allocatorRef, buffer, length, encoding, true, false);
}

EFStringRef EFStringCreateWithCBufferNoCopy(EFAllocatorRef allocatorRef,
                                            const UInt8 *buffer,
                                            size_t length,
                                            kEFStringEncoding encoding)
{
    return __EFStringCreate(allocatorRef, buffer, length, encoding, false, false);
}

EFStringRef EFStringCreateWithCString(EFAllocatorRef allocatorRef,
                                      const char *str,
                                      kEFStringEncoding encoding)
{
    return __EFStringCreateWithCString(allocatorRef, str, encoding, true);
}

EFStringRef EFStringCreateWithCStringNoCopy(EFAllocatorRef allocatorRef,
                                            const char *str,
                                            kEFStringEncoding encoding)
{
    return __EFStringCreateWithCString(allocatorRef, str, encoding, false);
}

typedef struct {
    char *data;
    size_t length;
    size_t cap;
    Boolean failed;
} __EFFmtBuf;

static inline void __evfb_ensure(__EFFmtBuf *b,
                                 size_t extra)
{
    if(b->failed)
    {
        return;
    }
    if(b->length + extra + 1 > b->cap)
    {
        size_t nc = b->cap ? b->cap : 64;
        while(nc < b->length + extra + 1)
        {
            nc *= 2;
        }
        char *p = realloc(b->data, nc);
        if(p == NULL)
        {
            b->failed = true;
            return;
        }
        b->data = p; b->cap = nc;
    }
}
static inline void __evfb_bytes(__EFFmtBuf *b,
                                const char *s,
                                size_t n)
{
    __evfb_ensure(b, n); if (b->failed) return;
    memcpy(b->data + b->length, s, n); b->length += n;
}

static inline void __evfb_char(__EFFmtBuf *b,
                               char c)
{
    __evfb_ensure(b, 1); if (b->failed) return; b->data[b->length++] = c;
}

enum {
    LEN_NONE,
    LEN_hh,
    LEN_h,
    LEN_l,
    LEN_ll,
    LEN_j,
    LEN_z,
    LEN_t,
    LEN_L
};

#define __EF_EMIT(TYPE) do { \
    TYPE _v = va_arg(*ap, TYPE); \
    int _n = snprintf(NULL, 0, spec, _v); \
    if(_n < 0) \
    { \
        b->failed = true; \
        return; \
    } \
    __evfb_ensure(b, (size_t)_n); if (b->failed) return; \
    snprintf(b->data + b->length, (size_t)_n + 1, spec, _v); \
    b->length += (size_t)_n; \
} while (0)

static inline void __EFEmitValue(__EFFmtBuf *b,
                                 const char *spec,
                                 char conv,
                                 int lenmod,
                                 va_list *ap)
{
    switch(conv)
    {
        case 'd':
        case 'i':
            switch (lenmod)
            {
                case LEN_l: __EF_EMIT(long); break;
                case LEN_ll: __EF_EMIT(long long); break;
                case LEN_j: __EF_EMIT(intmax_t); break;
                case LEN_z: __EF_EMIT(size_t); break;
                case LEN_t: __EF_EMIT(ptrdiff_t); break;
                default: __EF_EMIT(int); break;
        } break;
    case 'o':
    case 'u':
    case 'x':
    case 'X':
        switch(lenmod)
        {
            case LEN_l: __EF_EMIT(unsigned long); break;
            case LEN_ll: __EF_EMIT(unsigned long long); break;
            case LEN_j: __EF_EMIT(uintmax_t); break;
            case LEN_z: __EF_EMIT(size_t); break;
            case LEN_t: __EF_EMIT(ptrdiff_t); break;
            default: __EF_EMIT(unsigned int); break;
        } break;
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
        if(lenmod == LEN_L)
        {
            __EF_EMIT(long double);
        }
        else
        {
            __EF_EMIT(double);
        }
        break;
    case 'c': __EF_EMIT(int); break;
    case 's': __EF_EMIT(const char *); break;
    case 'p': __EF_EMIT(void *); break;
    default: __evfb_bytes(b, spec, strlen(spec)); break;
    }
}

EFStringRef EFStringCreateWithFormatAndArguments(EFAllocatorRef allocatorRef,
                                                 EFStringRef formatStringRef,
                                                 va_list arguments)
{
    if(formatStringRef == NULL)
    {
        return NULL;
    }

    const char *fmt = EFStringGetCStringPtr(formatStringRef, kEFStringEncodingUTF8);
    if(fmt == NULL)
    {
        return NULL;
    }

    __EFFmtBuf b = {0};
    va_list ap;
    va_copy(ap, arguments);

    const char *p = fmt;
    while(*p)
    {
        if(*p != '%')
        {
            __evfb_char(&b, *p++);
            continue;
        }
        const char *start = p++;
        if(*p == '%')
        {
            __evfb_char(&b, '%');
            p++;
            continue;
        }

        char spec[64]; int si = 0; spec[si++] = '%';
        while(*p && strchr("-+ 0#'", *p))
        {
            if(si < 58)
            {
                spec[si++] = *p; p++;
            }
        }

        if(*p == '*')
        {
            int w = va_arg(ap, int);
            si += snprintf(spec+si, sizeof spec - si, "%d", w);
            p++;
        }
        else
        {
            while(isdigit((unsigned char)*p))
            {
                if(si < 58)
                {
                    spec[si++] = *p;
                }
                p++;
            }
        }

        if(*p == '.')
        {
            if(si < 58)
            {
                spec[si++] = '.';
            }
            p++;
            if(*p == '*')
            {
                int pr = va_arg(ap, int);
                si += snprintf(spec+si, sizeof spec - si, "%d", pr);
                p++;
            }
            else
            {
                while(isdigit((unsigned char)*p))
                {
                    if(si < 58)
                    {
                        spec[si++] = *p;
                    }
                    p++;
                }
            }
        }

        int lenmod = LEN_NONE;
        if(*p == 'h')
        {
            if(p[1]=='h')
            {
                lenmod=LEN_hh;
                spec[si++]='h';
                spec[si++]='h';
                p+=2;
            }
            else
            {
                lenmod=LEN_h;
                spec[si++]='h';
                p++;
            }
        }
        else if(*p == 'l')
        {
            if(p[1]=='l')
            {
                lenmod=LEN_ll;
                spec[si++]='l';
                spec[si++]='l';
                p+=2;
            }
            else
            {
                lenmod=LEN_l;
                spec[si++]='l';
                p++;
            }
        }
        else if(*p == 'j')
        {
            lenmod=LEN_j;
            spec[si++]='j';
            p++;
        }
        else if(*p == 'z')
        {
            lenmod=LEN_z;
            spec[si++]='z';
            p++;
        }
        else if(*p == 't')
        {
            lenmod=LEN_t;
            spec[si++]='t';
            p++;
        }
        else if(*p == 'L')
        {
            lenmod=LEN_L;
            spec[si++]='L';
            p++;
        }

        char conv = *p;
        if(conv == '\0')
        {
            __evfb_bytes(&b, start, (size_t)(p - start));
            break;
        }
        if(conv == 'n')
        {
            b.failed = true;
            break;
        }

        if(conv == '@')
        {
            p++;
            EFObjectRef obj = va_arg(ap, EFObjectRef);
            EFStringRef descriptionRef = EFCopyDescription(obj);
            const char *d = EFStringGetCStringPtr(descriptionRef, kEFStringEncodingUTF8);
            __evfb_bytes(&b, d, strlen(d));
            EFRelease(descriptionRef);
            continue;
        }

        spec[si++] = conv; spec[si] = '\0'; p++;
        __EFEmitValue(&b, spec, conv, lenmod, &ap);
    }

    va_end(ap);

    if(b.failed)
    {
        free(b.data);
        return NULL;
    }

    EFStringRef resultRef = EFStringCreateWithCBuffer(allocatorRef, (const UInt8 *)(b.data ? b.data : ""), b.length, kEFStringEncodingUTF8);
    free(b.data);
    return resultRef;
}

EFStringRef EFStringCreateWithFormat(EFAllocatorRef allocatorRef,
                                     EFStringRef formatStringRef,
                                     ...)
{
    va_list arguments;
    va_start(arguments, formatStringRef);
    EFStringRef resultRef = EFStringCreateWithFormatAndArguments(allocatorRef, formatStringRef, arguments);
    va_end(arguments);
    return resultRef;
}

EFStringRef EFStringCreateCopy(EFAllocatorRef allocatorRef,
                               EFStringRef stringRef)
{
    return __EFStringCreateCopy(allocatorRef, stringRef, false);
}

EFMutableStringRef EFStringCreateMutableCopy(EFAllocatorRef allocatorRef,
                                             EFStringRef stringRef)
{
    return __EFStringCreateCopy(allocatorRef, stringRef, true);
}

const char *EFStringGetCStringPtr(EFStringRef stringRef,
                                  kEFStringEncoding encoding)
{
    if(stringRef == NULL)
    {
        return NULL;
    }

    EFString string = (EFString)stringRef;
    if(!__EFStringValidateEncoding(encoding, string->buffer, string->length))
    {
        return NULL;
    }

    return string->buffer;
}

EFIndex EFStringGetLength(EFStringRef stringRef)
{
    if(stringRef == NULL)
    {
        return 0;
    }

    EFString string = (EFString)stringRef;
    return string->length;
}

Boolean EFStringGetCString(EFStringRef stringRef,
                           char *str,
                           size_t str_len,
                           kEFStringEncoding encoding)
{
    const char *str_ptr = EFStringGetCStringPtr(stringRef, encoding);
    if(str_ptr == NULL)
    {
        return false;
    }

    EFIndex length = EFStringGetLength(stringRef);
    if((length + 1) > (EFIndex)str_len)
    {
        /* buffer is too small */
        return false;
    }

    memcpy(str, str_ptr, length);
    str[length] = '\0';
    return true;
}

EFArrayRef EFStringComponentsSplitBySeparator(EFStringRef stringRef,
                                              EFStringRef separatorStringRef)
{
    EFString string = (EFString)stringRef;
    EFString separatorString = (EFString)separatorStringRef;

    if(string == NULL || separatorString == NULL)
    {
        return NULL;
    }

    /* the separator has to comply to the encoding of the string */
    if(string->encoding != separatorString->encoding &&
        !__EFStringValidateEncoding(string->encoding, separatorString->buffer, separatorString->length))
    {
        return NULL;
    }

    /* the string must be bigger than the separator */
    if(separatorString->length >= string->length)
    {
        return NULL;
    }

    /* now we gotta calculate the amount of split items */
    EFIndex componentCount = 0;
    for(EFIndex i = 0; i < string->length; i++)
    {
        if(strncmp(&string->buffer[i], separatorString->buffer, (size_t)separatorString->length) == 0)
        {
            componentCount++;
        }
    }

    /* gotta need a array */
    EFAllocatorRef allocatorRef = EFGetAllocator(stringRef);
    EFMutableArrayRef componentsArrayRef = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, componentCount);
    if(componentsArrayRef == NULL)
    {
        return NULL;
    }

    /* now creating the sub components */
    EFIndex lastLengthMatch = 0;
    for(EFIndex i = 0; i < string->length; i++)
    {
        if(strncmp(&string->buffer[i], separatorString->buffer, separatorString->length) == 0)
        {
            EFIndex length = i - lastLengthMatch;
            EFStringRef componentRef = EFStringCreateWithCBuffer(allocatorRef, (const UInt8 *)&string->buffer[lastLengthMatch], length, string->encoding);
            if(componentRef == NULL)
            {
                EFRelease(componentsArrayRef);
                return NULL;
            }

            Boolean success = EFArrayAppendValue(componentsArrayRef, componentRef);
            EFRelease(componentRef);
            if(!success)
            {
                EFRelease(componentsArrayRef);
                return NULL;
            }

            lastLengthMatch = i + separatorString->length;
            i += separatorString->length - 1;
        }
    }

    /* creating ref for remaining length */
    EFIndex length = string->length - lastLengthMatch;
    if(length <= 0)
    {
        return componentsArrayRef;
    }

    EFStringRef componentRef = EFStringCreateWithCBuffer(allocatorRef, (const UInt8 *)&string->buffer[lastLengthMatch], length, string->encoding);
    Boolean success = EFArrayAppendValue(componentsArrayRef, componentRef);
    EFRelease(componentRef);
    if(!success)
    {
        EFRelease(componentsArrayRef);
        return NULL;
    }

    return componentsArrayRef;
}

static Boolean __EFIsWhitespace(char c)
{
    return c == ' '  || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

Boolean EFStringTrimWhitespace(EFMutableStringRef mutableStringRef)
{
    EFString mutableString = (EFString)mutableStringRef;
    if(mutableString == NULL || !mutableString->is_mutable)
    {
        return false;
    }

    EFIndex start = 0;
    while(start < mutableString->length && __EFIsWhitespace(mutableString->buffer[start]))
    {
        start++;
    }

    if(start == mutableString->length)
    {
        mutableString->buffer[0] = '\0';
        mutableString->length = 0;
        return true;
    }

    EFIndex end = mutableString->length - 1;
    while(end > start && __EFIsWhitespace(mutableString->buffer[end]))
    {
        end--;
    }

    EFIndex new_len = end - start + 1;

    if(start != 0)
    {
        memmove(mutableString->buffer, mutableString->buffer + start, (size_t)new_len);
    }
    mutableString->buffer[new_len] = '\0';
    mutableString->length = new_len;

    return true;
}

Boolean EFStringAppendString(EFMutableStringRef mutableStringRef,
                             EFStringRef stringRef)
{
    EFString mutableString = (EFString)mutableStringRef;
    EFString appendString = (EFString)stringRef;
    if(mutableString == NULL || !mutableString->is_mutable || appendString == NULL) /* string must be mutable to be compatible with operations like appending */
    {
        return false;
    }

    /* append string must comply to the encoding of the mutable string */
    if(!__EFStringValidateEncoding(mutableString->encoding, appendString->buffer, appendString->length))
    {
        return false;
    }

    EFIndex totalNewLength = mutableString->length + appendString->length + 1;
    char *newp = realloc(mutableString->buffer, totalNewLength);
    if(newp == NULL)
    {
        return false;
    }
    mutableString->buffer = newp;

    memcpy(mutableString->buffer + mutableString->length, appendString->buffer, appendString->length);
    mutableString->buffer[totalNewLength - 1] = '\0';
    mutableString->length = totalNewLength - 1;

    return true;
}
