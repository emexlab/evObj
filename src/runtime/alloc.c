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

#include <evObj/runtime/alloc.h>
#include <evObj/runtime/reference.h>
#include <evObj/runtime/register.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

EVObjectRef EVAlloc(EVTypeID typeID)
{
    EVClass *class = EVClassGetByID(typeID);

    /* validating class passed size */
    assert(class != NULL && class->size >= sizeof(EVObject));

    EVObject *object = calloc(1, class->size);
    if(object == NULL)
    {
        return NULL;
    }
    object->refcount = 1;
    object->typeID = class->typeID;

    /* initilizing when applicable */
    if(class->init != NULL)
    {
        class->init(object);
    }

    return (EVObjectRef)object;
}
