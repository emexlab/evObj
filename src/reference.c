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
#include <evObj/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>

bool evobject_retain(evobject_t *evo)
{
    assert(evo != NULL);

    /* performing retain if valid */
    while(1)
    {
        /* getting current reference count */
        int current = atomic_load(&evo->refcount);

        /* checking if object can be retained */
        if(current <= 0 || (atomic_load(&evo->state) == evObjStateInvalid))
        {
            return false;
        }

        /* retaining object */
        if(atomic_compare_exchange_weak(&evo->refcount, &current, current + 1))
        {
            /* performing another check */
            if(atomic_load(&evo->state) == evObjStateInvalid)
            {
                /* rollback using release logic */
                evo_release(evo);
                return false;
            }

            return true;
        }
    }
}

void evobject_invalidate(evobject_strong_t *evo)
{
    assert(evo != NULL);
    evo_event_trigger(evo, evObjEventInvalidate, 0);
    atomic_store(&(evo->state), evObjStateInvalid);
}

void evobject_release(evobject_strong_t *evo)
{
    assert(evo != NULL);

    /* releasing and trying to get the old reference count */
    int old = atomic_fetch_sub(&evo->refcount, 1);
    if(old == 1)
    {
        evobject_event_trigger(evo, evObjEventDeinit, 0);

        /* only a normal object has these locks */
        switch(evo->base_type)
        {
            case evObjBaseTypeObject:
                pthread_rwlock_destroy(&(evo->rwlock));
                pthread_rwlock_destroy(&(evo->event_rwlock));
                break;
            case evObjBaseTypeObjectSnapshot:
                if(evo->orig != NULL)
                {
                    evo_release(evo->orig);
                }
                break;
            default:
                fprintf(stderr, "unknown object %d type got deinitilized", evo->base_type);
                exit(1);
        }

        free(evo);
    }
    else if(old <= 0)
    {
        fprintf(stderr, "reference underflow on evobject @ %p", (void*)evo);
        exit(1);
    }
}
