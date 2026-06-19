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

#include <evObj/alloc.h>
#include <evObj/heap.h>
#include <evObj/reference.h>
#include <evObj/lock.h>
#include <evObj/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define evobject_header_from_heap_ptr(ptr) (evobject_t*)((uint8_t*)(ptr) - sizeof(evobject_t))

DEFINE_EVOBJECT_MAIN_EVENT_HANDLER(heap)
{
    if(evarr == NULL)
    {
        fprintf(stderr, "evObj heap objects may only be allocated using evmalloc/evcalloc\n");
        exit(1);
    }

    switch(type)
    {
        case evObjEventCopy:
        case evObjEventSnapshot:
            fprintf(stderr, "evObj heap objects don't support being copied or snapshotted yet\n");
            exit(1);
        default:
            return 0;
    }
}

void *ev_heap_alloc(size_t len)
{
    evobject_t *evo = calloc(1, sizeof(evobject_t) + len);
    if(evo == NULL)
    {
        return NULL;
    }
    
    /* setting up evobject for usage */
    evo->refcount = 1;                          /* starting as retained for the caller, cuz the caller gets one reference */
    evo->base_type = evObjBaseTypeObject;
    evo->state = evObjStateNormal;
    evo->orig = NULL;
    evo->main_handler = GET_EVOBJECT_MAIN_EVENT_HANDLER(heap);
    
    /* safely initilizing both locks */
    if(pthread_rwlock_init(&(evo->rwlock), NULL) != 0)
    {
        free(evo);
        return NULL;
    }
    
    if(pthread_rwlock_init(&(evo->event_rwlock), NULL) != 0)
    {
        pthread_rwlock_destroy(&(evo->rwlock));
        free(evo);
        return NULL;
    }
    
    /* checking init handler and executing if nonnull */
    if(evo->main_handler(&evo, evObjEventInit) != 0)
    {
        pthread_rwlock_destroy(&(evo->rwlock));
        pthread_rwlock_destroy(&(evo->event_rwlock));
        free(evo);
        return NULL;
    }

    #ifdef DEBUG
    fprintf(stderr, "\033[1m[evObj]\033[0m heap allocated object @ \033[1m%p\033[0m (rfcnt=\033[1m%d\033[0m)\n", (void*)evo, evo->refcount);
    #endif /* DEBUG */
    
    /* returning da object */
    return (void*)((uint8_t*)evo + sizeof(evobject_t));
}

void *ev_heap_calloc(size_t count, size_t len)
{
    size_t total = count * len;
    return ev_heap_alloc(total);    /* uses calloc internally, so no need to zero it out */
}

bool ev_heap_retain(void *p)
{
    return evo_retain(evobject_header_from_heap_ptr(p));
}

void ev_heap_release(void *p)
{
    evo_release(evobject_header_from_heap_ptr(p));
}

evobject_t *ev_heap_get_header(void *p)
{
    return evobject_header_from_heap_ptr(p);
}
