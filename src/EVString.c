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

static void __EVStringInit(EVStringRef stringRef)
{
    EVString string = (EVString)stringRef;

    /* we first automatically expect it to be at the inline */
    string->is_inlined = true;
    string->buf = (char*)((const char*)string + sizeof(struct __EVString));
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

static EVClass EVStringClass = {
    .name = "EVString",
    .typeID = kEVNotATypeID,
    .init = __EVStringInit,
    .deinit = NULL,
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

EVStringRef EVStringCreateWithCString(EVAllocatorRef allocatorRef,
                                      const char *str,
                                      kEVStringEncoding encoding)
{
    if(str == NULL)
    {
        return NULL;
    }

    size_t len = strlen(str);
    if(len <= 0 || !__EVStringValidateEncoding(encoding, str, len))
    {
        return NULL;
    }

    /* inline buffer and nullterminator counts */
    EVString string = EVObjectAlloc(allocatorRef, EVStringGetTypeID(), sizeof(struct __EVString) + len + 1);
    if(string == NULL)
    {
        return NULL;
    }

    /* copy buffer into object */
    memcpy(string->buf, str, len);
    string->buf[len] = '\0';
    string->len = len;
    string->encoding = encoding;

    return (EVStringRef)string;
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
    if(!__EVStringValidateEncoding(encoding, str, len))
    {
        return NULL;
    }

    EVString string = EVObjectAlloc(allocatorRef, EVStringGetTypeID(), sizeof(struct __EVString));
    if(string == NULL)
    {
        return NULL;
    }

    string->is_inlined = false; /* string is not inlined lol */
    string->buf = (char*)str;
    string->len = len;
    string->encoding = encoding;

    return (EVStringRef)string;
}

EVStringRef EVStringCreateWithCBuffer(EVAllocatorRef allocatorRef,
                                      const uint8_t *buf,
                                      size_t len,
                                      kEVStringEncoding encoding)
{
    if(buf == NULL || len == 0)
    {
        return NULL;
    }

    if(!__EVStringValidateEncoding(encoding, (const char*)buf, len))
    {
        return NULL;
    }

    EVString string = EVObjectAlloc(allocatorRef, EVStringGetTypeID(), sizeof(struct __EVString) + len + 1);
    if(string == NULL)
    {
        return NULL;
    }

    memcpy(string->buf, buf, len);
    string->buf[len] = '\0';
    string->len = len;
    string->encoding = encoding;

    return (EVStringRef)string;
}

EVStringRef EVStringCreateCopy(EVAllocatorRef allocatorRef,
                               EVStringRef stringRef)
{
    if(stringRef == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        allocatorRef = EVGetAllocator(stringRef);
    }

    /* copy gets harder due to the string inline feature now, inline strings have to stay inline */
    EVString string = (EVString)stringRef;

    if(string->is_inlined)
    {
        return EVStringCreateWithCString(allocatorRef, string->buf, string->encoding);
    }
    else
    {
        return EVStringCreateWithCStringNoCopy(allocatorRef, string->buf, string->encoding);
    }
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

size_t EVStringGetLength(EVStringRef stringRef)
{
    if(stringRef == NULL)
    {
        return 0;
    }

    EVString string = (EVString)stringRef;
    return string->len;
}

bool EVStringGetCString(EVStringRef stringRef,
                        char *str,
                        size_t str_len,
                        kEVStringEncoding encoding)
{
    const char *str_ptr = EVStringGetCStringPtr(stringRef, encoding);
    if(str_ptr == NULL)
    {
        return false;
    }

    size_t len = EVStringGetLength(stringRef);
    if((len + 1) > str_len)
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
    size_t component_cnt = 0;
    for(size_t i = 0; i < string->len; i++)
    {
        if(strncmp(&string->buf[i], separatorString->buf, separatorString->len) == 0)
        {
            component_cnt++;
        }
    }

    /* gotta need a array */
    EVAllocatorRef allocatorRef = EVGetAllocator(stringRef);
    EVMutableArrayRef componentsArrayRef = EVArrayCreateMutable(allocatorRef, kEVArrayCallbacksObjectCallbacks, component_cnt);
    if(componentsArrayRef == NULL)
    {
        return NULL;
    }

    /* now creating the sub components */
    size_t last_len_match = 0;
    for(size_t i = 0; i < string->len; i++)
    {
        if(strncmp(&string->buf[i], separatorString->buf, separatorString->len) == 0)
        {
            size_t len = i - last_len_match;
            EVStringRef componentRef = EVStringCreateWithCBuffer(allocatorRef, (const uint8_t *)&string->buf[last_len_match], len, string->encoding);
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

            last_len_match = i + separatorString->len;
            i += separatorString->len - 1;
        }
    }

    /* creating ref for remaining lenght */
    size_t len = string->len - last_len_match;
    if(len <= 0)
    {
        return componentsArrayRef;
    }

    EVStringRef componentRef = EVStringCreateWithCBuffer(allocatorRef, (const uint8_t *)&string->buf[last_len_match], len, string->encoding);
    bool success = EVArrayAppendValue(componentsArrayRef, componentRef);
    EVRelease(componentRef);
    if(!success)
    {
        EVRelease(componentsArrayRef);
        return NULL;
    }

    return componentsArrayRef;
}
