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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <evObj/runtime/EVBase.h>
#include <evObj/runtime/EVAllocator.h>
#include <evObj/EVString.h>

typedef struct EVString {
    EVObject header;
    kEVStringEncoding encoding;
    bool owns_buf;
    const char *buf;
    size_t len;
} *EVString;

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

static void __EVStringDealloc(EVStringRef stringRef)
{
    EVString string = (EVString)stringRef;
    if(string->owns_buf)
    {
        free((char*)string->buf);
    }
}

static bool __EVStringEqual(EVStringRef stringRef1,
                            EVStringRef stringRef2)
{
    EVString string1 = (EVString)stringRef1;
    EVString string2 = (EVString)stringRef2;

    /* encoding must match */
    if(string1->encoding != string2->encoding)
    {
        return false;
    }

    /* size must match */
    if(string1->len != string2->len)
    {
        return false;
    }

    return (strncmp(string1->buf, string2->buf, string1->len) == 0);
}

static EVClass EVStringClass = {
    .name = "EVString",
    .typeID = kEVNotATypeID,
    .size = sizeof(struct EVString),
    .init = NULL,
    .deinit = __EVStringDealloc,
    .copy = NULL,
    .equal = __EVStringEqual,
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

EVStringRef EVStringCreateWithCString(EVAllocator *allocator,
                                      const char *str,
                                      kEVStringEncoding encoding)
{
    if(str == NULL)
    {
        return NULL;
    }

    size_t len = strlen(str);
    if(!__EVStringValidateEncoding(encoding, str, len))
    {
        return NULL;
    }

    EVString string = EVObjectAlloc(allocator, EVStringGetTypeID());
    if(string == NULL)
    {
        return NULL;
    }

    string->owns_buf = true;
    string->buf = strndup(str, len + 1);
    string->len = len;
    string->encoding = encoding;

    return (EVStringRef)string;
}

EVStringRef EVStringCreateWithCStringNoCopy(EVAllocator *allocator,
                                            const char *str,
                                            kEVStringEncoding encoding)
{
    if(str == NULL)
    {
        return NULL;
    }

    size_t len = strlen(str);
    if(!__EVStringValidateEncoding(encoding, str, len))
    {
        return NULL;
    }

    EVString string = EVObjectAlloc(allocator, EVStringGetTypeID());
    if(string == NULL)
    {
        return NULL;
    }

    string->owns_buf = false;
    string->buf = str;
    string->len = len;
    string->encoding = encoding;

    return (EVStringRef)string;
}

EVStringRef EVStringCreateCopy(EVAllocator *allocator,
                               EVStringRef stringRef)
{
    if(stringRef == NULL)
    {
        return NULL;
    }

    EVString string = (EVString)stringRef;
    return EVStringCreateWithCString(EVGetAllocator(stringRef), string->buf, string->encoding);
}
