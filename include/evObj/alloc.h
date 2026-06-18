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

#ifndef EVOBJECT_ALLOC_H
#define EVOBJECT_ALLOC_H

#include <evObj/defs.h>

#define evo_alloc(main) (void*)evobject_alloc(main)
#define evo_alloc_fastpath(name) (void*)evobject_alloc(GET_EVOBJECT_MAIN_EVENT_HANDLER(name))
#define evo_copy(evo) (void*)evobject_copy((evobject_t*)evo)
#define evo_snapshot(evo, option) (void*)evobject_snapshot((evobject_t*)evo, option)

evobject_strong_t *evobject_alloc(evobject_main_event_handler_t handler);
evobject_strong_t *evobject_copy(evobject_t *evo);
evobject_snapshot_t *evobject_snapshot(evobject_t *evo, evobject_snapshot_options_t option);

#endif /* EVOBJECT_ALLOC_H */
