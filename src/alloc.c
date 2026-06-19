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
#include <evObj/reference.h>
#include <evObj/lock.h>
#include <evObj/event.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

evobject_strong_t *evobject_alloc(evobject_main_event_handler_t handler)
{
    assert(handler != NULL);
    
    /* get object size first */
    size_t size = (size_t)handler(NULL, evObjEventInit);
    
    /*
     * first we gotta check if the size
     * is atleast the size of an evobject
     */
    assert(size >= sizeof(evobject_t));
    
    /* allocating brand new evobject */
    evobject_t *evo = calloc(1, size);
    
    /* checking if allocation suceeded */
    if(evo == NULL)
    {
        return NULL;
    }
    
    /* setting up evobject for usage */
    evo->refcount = 1;                          /* starting as retained for the caller, cuz the caller gets one reference */
    evo->base_type = evObjBaseTypeObject;
    evo->state = evObjStateNormal;
    evo->orig = NULL;
    
    /* setting handlers and running init straight */
    evo->main_handler = handler;
    
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
    fprintf(stderr, "\033[1m[evObj]\033[0m allocated object @ \033[1m%p\033[0m (rfcnt=\033[1m%d\033[0m)\n", (void*)evo, evo->refcount);
    #endif /* DEBUG */
    
    /* returning da object */
    return evo;
}

evobject_strong_t *evobject_copy(evobject_t *evo)
{
    assert(evo != NULL);
    
    /* sanity check */
    if(!evo_retain(evo))
    {
        return NULL;
    }
    
    evo_rdlock(evo);
    
    assert(evo->base_type == evObjBaseTypeObject && evo->main_handler != NULL);
    
    /* getting object size */
    size_t size = (size_t)evo->main_handler(NULL, evObjEventInit);
    
    /* creating new object */
    evobject_t *evo_dup = calloc(1, size);
    
    /* checking if allocation was successful */
    if(evo_dup == NULL)
    {
        goto out_unlock;
    }
    
    /* setup object initially */
    evo_dup->refcount = 1;                                  /* starting as retained for the caller, cuz the caller gets one reference */
    evo_dup->base_type = evObjBaseTypeObject;
    evo_dup->state = evObjStateNormal;
    evo_dup->orig = NULL;
    
    /* setting handlers and running copyit straight */
    evo_dup->main_handler = evo->main_handler;
    
    /* preparing stack array */
    evobject_t *evoarr[2] = { evo_dup, evo };
    
    /* safely initilizing both locks */
    if(pthread_rwlock_init(&(evo_dup->rwlock), NULL) != 0)
    {
        free(evo_dup);
        goto out_unlock;
    }
    
    if(pthread_rwlock_init(&(evo_dup->event_rwlock), NULL) != 0)
    {
        pthread_rwlock_destroy(&(evo_dup->rwlock));
        free(evo_dup);
        evo_dup = NULL;
        goto out_unlock;
    }
    
    /* checking init handler and executing if nonnull */
    if(evo_dup->main_handler(evoarr, evObjEventCopy) != 0)
    {
        pthread_rwlock_destroy(&(evo_dup->rwlock));
        pthread_rwlock_destroy(&(evo_dup->event_rwlock));
        free(evo_dup);
        evo_dup = NULL;
    }

    #ifdef DEBUG
    fprintf(stderr, "\033[1m[evObj]\033[0m copied object @ \033[1m%p\033[0m (rfcnt=\033[1m%d\033[0m) to object @ \033[1m%p\033[0m (rfcnt=\033[1m%d\033[0m)\n", (void*)evo, evo->refcount, (void*)evo_dup, evo_dup->refcount);
    #endif /* DEBUG */
    
out_unlock:
    evo_unlock(evo);
    evo_release(evo);
    return evo_dup;
}

evobject_snapshot_t *evobject_snapshot(evobject_t *evo,
                                       evobject_snapshot_options_t option)
{
    assert(evo != NULL);
    
    if(!evo_retain(evo))
    {
        return NULL;
    }
    
    evo_rdlock(evo);
    
    assert(evo->base_type == evObjBaseTypeObject && evo->main_handler != NULL);
    
    /* getting object size */
    size_t size = (size_t)evo->main_handler(NULL, evObjEventInit);
    
    /* creating new snapshot object */
    evobject_t *evo_snap = calloc(1, size);
    
    /* checking if allocation was successful */
    if(evo_snap == NULL)
    {
        goto out_unlock;
    }
    
    /* setup object initially */
    evo_snap->refcount = 1;                                 /* starting as retained for the caller, cuz the caller gets one reference */
    evo_snap->base_type = evObjBaseTypeObjectSnapshot;
    evo_snap->state = evObjStateNormal;
    
    /* set orig pointer if applicable */
    if(option == evObjSnapReferenced ||
       option == evObjSnapConsumeReference)
    {
        evo_snap->orig = evo;
    }
    
    /* setting handlers and running copyit straight */
    evo_snap->main_handler = evo->main_handler;
    
    /* preparing stack array */
    evobject_t *evoarr[2] = { evo_snap, evo };
    
    /* checking init handler and executing if nonnull */
    if(evo_snap->main_handler(evoarr, evObjEventSnapshot) != 0)
    {
        free(evo_snap);
        evo_snap = NULL;
    }
    
out_unlock:
    evo_unlock(evo);
    
    /* release object if applicable */
    if(option == evObjSnapStatic ||
       option == evObjSnapConsumeReference)
    {
        evo_release(evo);
    }

    #ifdef DEBUG
    fprintf(stderr, "\033[1m[evObj]\033[0m copied object @ \033[1m%p\033[0m (rfcnt=\033[1m%d\033[0m) to object @ \033[1m%p\033[0m (rfcnt=\033[1m%d\033[0m)\n", (void*)evo, evo->refcount, (void*)evo_snap, evo_snap->refcount);
    #endif /* DEBUG */
    
    return evo_snap;
}
