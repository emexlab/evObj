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

#include <evObj/event.h>
#include <evObj/reference.h>
#include <evObj/lock.h>
#include <stdlib.h>
#include <assert.h>

int evobject_event_register(evobject_strong_t *evo,
                            evobject_event_type_t mask,
                            evobject_event_handler_t handler,
                            void *context,
                            evobject_event_t **event)
{
    assert(evo != NULL && handler != NULL && evo->base_type != evObjBaseTypeObjectSnapshot);
    
    PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(evo->event_rwlock));
    
    /* find last event */
    if(evo->event_count >= EVOBJECT_EVENT_MAX)
    {
        PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(evo->event_rwlock));
        return 1;
    }
    
    /* allocating new event */
    evobject_event_t *e_event = malloc(sizeof(evobject_event_t));
    
    if(e_event == NULL)
    {
        PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(evo->event_rwlock));
        return 1;
    }
    
    /* setting mutex */
    if(pthread_mutex_init(&(e_event->in_use), NULL) != 0)
    {
        PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(evo->event_rwlock));
        free(e_event);
        return 1;
    }
    
    /* setting properties */
    e_event->previous = NULL;
    e_event->next = evo->event;
    e_event->owner = evo;
    e_event->handler = handler;
    e_event->ctx = context;
    e_event->mask = mask;
    
    /* now insert new event as the first event (faster) */
    if(evo->event != NULL)
    {
        evo->event->previous = e_event;
    }
    evo->event = e_event;
    evo->event_count++;
    
    PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(evo->event_rwlock));
    
    /* if event back pointer is givven, set it */
    if(event != NULL)
    {
        *event = e_event;
    }
    
    return 0;
}

void evobject_event_trigger(evobject_strong_t *evo,
                            evobject_event_type_t type,
                            uint64_t value)
{
    assert(evo != NULL && type != evObjEventCopy && type != evObjEventUnregister);
    
    /* sanity checking object type */
    if(evo->base_type != evObjBaseTypeObject)
    {
        return;
    }
    
    /* the main event handler shall always be called */
    evo->main_handler(&evo, type);
    
    PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(evo->event_rwlock));
    
    /*
     * execute all events in the chain and remove
     * events that wanna be removed(return true).
     */
    evobject_event_t *last_event = evo->event;
    while(last_event != NULL)
    {
        /* pointer to current event */
        evobject_event_t *current = last_event;
        last_event = current->next;
        
        if(type != evObjEventDeinit && (current->mask & type) != type)
        {
            continue;
        }
        
        if(pthread_mutex_trylock(&(current->in_use)) != 0)
        {
            continue;
        }
        
        /* unlocking again to allow recurse */
        PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(evo->event_rwlock));
        bool will_remove = current->handler(type, value, current);
        PTHREAD_RWLOCK_DEBUG_IMP_WRLOCK(&(evo->event_rwlock));
        
        pthread_mutex_unlock(&(current->in_use));
        
        /* remove if applicable */
        if(will_remove || type == evObjEventDeinit)
        {
            /* triggering unregistration event */
            current->handler(evObjEventUnregister, 0, current);
            
            /* relinking previous and next */
            if(current->next != NULL)
            {
                current->next->previous = current->previous;
            }
            
            if(current->previous != NULL)
            {
                current->previous->next = current->next;
            }
            else
            {
                current->owner->event = current->next;
            }
            
            /* removing event */
            evo->event_count--;
            pthread_mutex_destroy(&(current->in_use));
            free(current);
        }
    }
    
    PTHREAD_RWLOCK_DEBUG_IMP_UNLOCK(&(evo->event_rwlock));
}
