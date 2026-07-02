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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>
#include <evObj/EVString.h>

typedef __EVString EVString;

static bool __EVStringValidateEncoding(kEVStringEncoding encoding,
                                       const char *buf,
                                       size_t len)
{
    switch(encoding)
    {
        case kEVStringEncodingUTF7:
            for(size_t i = 0; i < len; i++)
            {
                if((unsigned char)buf[i] > 0x7F)
                {
                    return false;
                }
            }
            return true;
        case kEVStringEncodingUTF8: /* thanks to: https://www.rfc-editor.org/rfc/rfc3629.html */
        {
            size_t i = 0;
            while(i < len)
            {
                unsigned char c = (unsigned char)buf[i];
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

                if(i + (size_t)seqlen > len)
                {
                    return false;
                }
                for(int k = 1; k < seqlen; ++k)
                {
                    unsigned char cc = (unsigned char)buf[i+k];
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

static void __EVStringDeinit(EVStringRef stringRef)
{
    EVString string = (EVString)stringRef;
    if(string->is_mutable)
    {
        free(string->buf);
    }
}

static bool __EVStringEqual(EVStringRef stringRef1,
                            EVStringRef stringRef2)
{
    EVString string1 = (EVString)stringRef1;
    EVString string2 = (EVString)stringRef2;

    /* size must match */
    if(string1->len != string2->len)
    {
        return false;
    }

    /* they share the same lenght */
    size_t len = string1->len;

    /* strings must comply to their encodings */
    if(string1->encoding != string2->encoding)
    {
        bool string1_complies = __EVStringValidateEncoding(string2->encoding, string1->buf, len);
        bool string2_complies = __EVStringValidateEncoding(string1->encoding, string2->buf, len);
        if(!string1_complies || !string2_complies)
        {
            return false;
        }
    }

    return (memcmp(string1->buf, string2->buf, len) == 0);
}

static EVStringRef __EVStringCopyDescription(EVStringRef stringRef)
{
    return EVRetain(stringRef); /* just return our selves */
}

static EVClass EVStringClass = {
    .name = "EVString",
    .typeID = kEVNotATypeID,
    .init = NULL,
    .deinit = __EVStringDeinit,
    .equal = __EVStringEqual,
    .copyDescription = __EVStringCopyDescription,
};

static void EVStringRegisterClass(void)
{
    EVClassRegister(&EVStringClass);
}

EVTypeID EVStringGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, EVStringRegisterClass);
    return EVStringClass.typeID;
}

static inline EVStringRef __EVStringCreate(EVAllocatorRef allocatorRef,
                                           const uint8_t *buf,
                                           size_t len,
                                           kEVStringEncoding encoding,
                                           Boolean is_inlined,
                                           Boolean is_mutable)
{
    if(buf == NULL)
    {
        return NULL;
    }

    len = strnlen((const char*)buf, len);
    if(!__EVStringValidateEncoding(encoding, (const char*)buf, len))
    {
        return NULL;
    }

    EVString string = EVObjectAlloc(allocatorRef, EVStringGetTypeID(), sizeof(struct __EVString) + (is_inlined ? len + 1 : 0));
    if(string == NULL)
    {
        return NULL;
    }

    if(is_mutable)
    {
        string->buf = malloc(len + 1);
        is_inlined = false; /* must be false */
        goto needs_copy;
    }
    else if(is_inlined)
    {
        string->buf = (char*)((const char*)string + sizeof(struct __EVString));
    needs_copy:
        memcpy(string->buf, buf, len);
        string->buf[len] = '\0';
    }
    else
    {
        string->is_inlined = false; /* string is not inlined lol */
        string->buf = (char*)buf;
    }

    /* always assigned with the same values */
    string->len = len;
    string->encoding = encoding;
    string->is_inlined = !is_mutable && is_inlined; /* is_inlined is only possible when is_mutable is not enabled */
    string->is_mutable = is_mutable;

    return (EVStringRef)string;
}

EVStringRef EVStringCreateWithCBuffer(EVAllocatorRef allocatorRef,
                                      const uint8_t *buf,
                                      size_t len,
                                      kEVStringEncoding encoding)
{
    return __EVStringCreate(allocatorRef, buf, len, encoding, true, false);
}

EVStringRef EVStringCreateWithCBufferNoCopy(EVAllocatorRef allocatorRef,
                                            const uint8_t *buf,
                                            size_t len,
                                            kEVStringEncoding encoding)
{
    return __EVStringCreate(allocatorRef, buf, len, encoding, false, false);
}

EVStringRef EVStringCreateWithCString(EVAllocatorRef allocatorRef,
                                      const char *str,
                                      kEVStringEncoding encoding)
{
    if(str == NULL)
    {
        return NULL;
    }

    size_t len = strlen(str);
    return __EVStringCreate(allocatorRef, (const uint8_t*)str, len, encoding, true, false);
}

EVStringRef EVStringCreateWithCStringNoCopy(EVAllocatorRef allocatorRef,
                                            const char *str,
                                            kEVStringEncoding encoding)
{
    if(str == NULL)
    {
        return NULL;
    }

    size_t len = strlen(str);
    return __EVStringCreate(allocatorRef, (const uint8_t*)str, len, encoding, false, false);
}

typedef struct {
    char *data;
    size_t len;
    size_t cap;
    bool failed;
} __EVFmtBuf;

static void __evfb_ensure(__EVFmtBuf *b,
                          size_t extra)
{
    if(b->failed)
    {
        return;
    }
    if(b->len + extra + 1 > b->cap)
    {
        size_t nc = b->cap ? b->cap : 64;
        while(nc < b->len + extra + 1)
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
static void __evfb_bytes(__EVFmtBuf *b, const char *s, size_t n)
{
    __evfb_ensure(b, n); if (b->failed) return;
    memcpy(b->data + b->len, s, n); b->len += n;
}

static void __evfb_char(__EVFmtBuf *b, char c)
{
    __evfb_ensure(b, 1); if (b->failed) return; b->data[b->len++] = c;
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

#define __EV_EMIT(TYPE) do { \
    TYPE _v = va_arg(*ap, TYPE); \
    int _n = snprintf(NULL, 0, spec, _v); \
    if(_n < 0) \
    { \
        b->failed = true; \
        return; \
    } \
    __evfb_ensure(b, (size_t)_n); if (b->failed) return; \
    snprintf(b->data + b->len, (size_t)_n + 1, spec, _v); \
    b->len += (size_t)_n; \
} while (0)

static void __EVEmitValue(__EVFmtBuf *b,
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
                case LEN_l: __EV_EMIT(long); break;
                case LEN_ll: __EV_EMIT(long long); break;
                case LEN_j: __EV_EMIT(intmax_t); break;
                case LEN_z: __EV_EMIT(size_t); break;
                case LEN_t: __EV_EMIT(ptrdiff_t); break;
                default: __EV_EMIT(int); break;
        } break;
    case 'o':
    case 'u':
    case 'x':
    case 'X':
        switch(lenmod)
        {
            case LEN_l: __EV_EMIT(unsigned long); break;
            case LEN_ll: __EV_EMIT(unsigned long long); break;
            case LEN_j: __EV_EMIT(uintmax_t); break;
            case LEN_z: __EV_EMIT(size_t); break;
            case LEN_t: __EV_EMIT(ptrdiff_t); break;
            default: __EV_EMIT(unsigned int); break;
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
            __EV_EMIT(long double);
        }
        else
        {
            __EV_EMIT(double);
        }
        break;
    case 'c': __EV_EMIT(int); break;
    case 's': __EV_EMIT(const char *); break;
    case 'p': __EV_EMIT(void *); break;
    default: __evfb_bytes(b, spec, strlen(spec)); break;
    }
}

EVStringRef EVStringCreateWithFormatAndArguments(EVAllocatorRef allocatorRef,
                                                 EVStringRef formatStringRef,
                                                 va_list arguments)
{
    if(formatStringRef == NULL)
    {
        return NULL;
    }

    const char *fmt = EVStringGetCStringPtr(formatStringRef, kEVStringEncodingUTF8);
    if(fmt == NULL)
    {
        return NULL;
    }

    __EVFmtBuf b = {0};
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
            EVObjectRef obj = va_arg(ap, EVObjectRef);
            EVStringRef descriptionRef = EVCopyDescription(obj);
            const char *d = EVStringGetCStringPtr(descriptionRef, kEVStringEncodingUTF8);
            __evfb_bytes(&b, d, strlen(d));
            EVRelease(descriptionRef);
            continue;
        }

        spec[si++] = conv; spec[si] = '\0'; p++;
        __EVEmitValue(&b, spec, conv, lenmod, &ap);
    }

    va_end(ap);

    if(b.failed)
    {
        free(b.data);
        return NULL;
    }

    EVStringRef resultRef = EVStringCreateWithCBuffer(allocatorRef, (const uint8_t *)(b.data ? b.data : ""), b.len, kEVStringEncodingUTF8);
    free(b.data);
    return resultRef;
}

EVStringRef EVStringCreateWithFormat(EVAllocatorRef allocatorRef,
                                     EVStringRef formatStringRef,
                                     ...)
{
    va_list arguments;
    va_start(arguments, formatStringRef);
    EVStringRef resultRef = EVStringCreateWithFormatAndArguments(allocatorRef, formatStringRef, arguments);
    va_end(arguments);
    return resultRef;
}

EVStringRef EVStringCreateCopy(EVAllocatorRef allocatorRef,
                               EVStringRef stringRef)
{
    EVString string = (EVString)stringRef;
    if(string == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        /* falling back to the same allocator used to allocate the source x3 */
        allocatorRef = EVGetAllocator(stringRef);
    }

    return __EVStringCreate(allocatorRef, (const uint8_t*)string->buf, string->len, string->encoding, string->is_inlined, false);
}

EVMutableStringRef EVStringCreateMutableCopy(EVAllocatorRef allocatorRef,
                                             EVStringRef stringRef)
{
    EVString string = (EVString)stringRef;
    if(string == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        /* falling back to the same allocator used to allocate the source x3 */
        allocatorRef = EVGetAllocator(stringRef);
    }

    return __EVStringCreate(allocatorRef, (const uint8_t*)string->buf, string->len, string->encoding, string->is_inlined, true);
}

const char *EVStringGetCStringPtr(EVStringRef stringRef,
                                  kEVStringEncoding encoding)
{
    if(stringRef == NULL)
    {
        return NULL;
    }

    EVString string = (EVString)stringRef;
    if(!__EVStringValidateEncoding(encoding, string->buf, string->len))
    {
        return NULL;
    }

    return string->buf;
}

EVIndex EVStringGetLength(EVStringRef stringRef)
{
    if(stringRef == NULL)
    {
        return 0;
    }

    EVString string = (EVString)stringRef;
    return string->len;
}

Boolean EVStringGetCString(EVStringRef stringRef,
                           char *str,
                           size_t str_len,
                           kEVStringEncoding encoding)
{
    const char *str_ptr = EVStringGetCStringPtr(stringRef, encoding);
    if(str_ptr == NULL)
    {
        return false;
    }

    EVIndex len = EVStringGetLength(stringRef);
    if((len + 1) > (EVIndex)str_len)
    {
        /* buffer is too small */
        return false;
    }

    memcpy(str, str_ptr, len);
    str[len] = '\0';
    return true;
}

EVArrayRef EVStringComponentsSplitBySeparator(EVStringRef stringRef,
                                              EVStringRef separatorStringRef)
{
    EVString string = (EVString)stringRef;
    EVString separatorString = (EVString)separatorStringRef;

    if(string == NULL || separatorString == NULL)
    {
        return NULL;
    }

    /* the separator has to comply to the encoding of the string */
    if(string->encoding != separatorString->encoding &&
        !__EVStringValidateEncoding(string->encoding, separatorString->buf, separatorString->len))
    {
        return NULL;
    }

    /* the string must be bigger than the separator */
    if(separatorString->len >= string->len)
    {
        return NULL;
    }

    /* now we gotta calculate the amount of split items */
    EVIndex componentCount = 0;
    for(EVIndex i = 0; i < string->len; i++)
    {
        if(strncmp(&string->buf[i], separatorString->buf, (size_t)separatorString->len) == 0)
        {
            componentCount++;
        }
    }

    /* gotta need a array */
    EVAllocatorRef allocatorRef = EVGetAllocator(stringRef);
    EVMutableArrayRef componentsArrayRef = EVArrayCreateMutable(allocatorRef, kEVArrayCallbacksObjectCallbacks, componentCount);
    if(componentsArrayRef == NULL)
    {
        return NULL;
    }

    /* now creating the sub components */
    EVIndex lastLengthMatch = 0;
    for(EVIndex i = 0; i < string->len; i++)
    {
        if(strncmp(&string->buf[i], separatorString->buf, separatorString->len) == 0)
        {
            EVIndex length = i - lastLengthMatch;
            EVStringRef componentRef = EVStringCreateWithCBuffer(allocatorRef, (const uint8_t *)&string->buf[lastLengthMatch], length, string->encoding);
            if(componentRef == NULL)
            {
                EVRelease(componentsArrayRef);
                return NULL;
            }

            bool success = EVArrayAppendValue(componentsArrayRef, componentRef);
            EVRelease(componentRef);
            if(!success)
            {
                EVRelease(componentsArrayRef);
                return NULL;
            }

            lastLengthMatch = i + separatorString->len;
            i += separatorString->len - 1;
        }
    }

    /* creating ref for remaining lenght */
    EVIndex len = string->len - lastLengthMatch;
    if(len <= 0)
    {
        return componentsArrayRef;
    }

    EVStringRef componentRef = EVStringCreateWithCBuffer(allocatorRef, (const uint8_t *)&string->buf[lastLengthMatch], len, string->encoding);
    bool success = EVArrayAppendValue(componentsArrayRef, componentRef);
    EVRelease(componentRef);
    if(!success)
    {
        EVRelease(componentsArrayRef);
        return NULL;
    }

    return componentsArrayRef;
}

static Boolean __EVIsWhitespace(char c)
{
    return c == ' '  || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}


Boolean EVStringTrimWhitespace(EVMutableStringRef mutableStringRef)
{
    EVString mutableString = (EVString)mutableStringRef;
    if(mutableString == NULL || !mutableString->is_mutable)
    {
        return false;
    }

    EVIndex start = 0;
    while(start < mutableString->len && __EVIsWhitespace(mutableString->buf[start]))
    {
        start++;
    }

    if(start == mutableString->len)
    {
        mutableString->buf[0] = '\0';
        mutableString->len = 0;
        return true;
    }

    EVIndex end = mutableString->len - 1;
    while(end > start && __EVIsWhitespace(mutableString->buf[end]))
    {
        end--;
    }

    EVIndex new_len = end - start + 1;

    if(start != 0)
    {
        memmove(mutableString->buf, mutableString->buf + start, (size_t)new_len);
    }
    mutableString->buf[new_len] = '\0';
    mutableString->len = new_len;

    return true;
}

Boolean EVStringAppendString(EVMutableStringRef mutableStringRef,
                             EVStringRef stringRef)
{
    EVString mutableString = (EVString)mutableStringRef;
    EVString appendString = (EVString)stringRef;
    if(mutableString == NULL || !mutableString->is_mutable ||
       appendString == NULL)
    {
        return false;
    }

    /* append string must comply to the encoding of the other string */
    if(!__EVStringValidateEncoding(mutableString->encoding, appendString->buf, appendString->len))
    {
        return false;
    }

    EVIndex totalNewLength = mutableString->len + appendString->len + 1;
    char *newp = realloc(mutableString->buf, totalNewLength);
    if(newp == NULL)
    {
        return false;
    }
    mutableString->buf = newp;

    memcpy(mutableString->buf + mutableString->len, appendString->buf, appendString->len);
    mutableString->buf[totalNewLength - 1] = '\0';
    mutableString->len = totalNewLength - 1;

    return true;
}
