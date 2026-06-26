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

#include <evObj/reference.h>
#include <evObj/register.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

EVObjectRef EVRetain(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    assert(object != NULL);

    /* performing retain if valid */
    while(1)
    {
        /* getting current reference count */
        int current = atomic_load(&object->refcount);

        /* checking if object can be retained */
        if(current <= 0 || (atomic_load(&object->state) == kEVObjectStateInvalid))
        {
            return NULL;
        }

        /* retaining object */
        if(atomic_compare_exchange_weak(&object->refcount, &current, current + 1))
        {
            /* performing another check */
            if(atomic_load(&object->state) == kEVObjectStateInvalid)
            {
                /* rollback using release logic */
                EVRelease(ref);
                return NULL;
            }

            return ref;
        }
    }
}

void EVRelease(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    assert(object != NULL);

    /* releasing and trying to get the old reference count */
    int old = atomic_fetch_sub(&object->refcount, 1);
    if(old == 1)
    {
        /* trigger handler */
        EVClass *class = EVClassGetByID(object->typeID);
        assert(class != NULL);
        if(class->deinit != NULL)
        {
            class->deinit(ref);
        }
        pthread_mutex_destroy(&object->mutex);
        free(object);
    }
    else if(old <= 0)
    {
        fprintf(stderr, "EVRelease: fatal error occured, reference underflow\n");
        exit(1);
    }
}

void EVInvalidate(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    assert(object != NULL);
    atomic_store(&(object->state), kEVObjectStateInvalid);
}

int EVGetRetainCount(EVObjectRef ref)
{
    EVObject *object = (EVObject*)ref;
    return atomic_load(&object->refcount);
}
