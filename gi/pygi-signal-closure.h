/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2011  Laszlo Pandy <lpandy@src.gnome.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __PYGI_SIGNAL_CLOSURE_H__
#define __PYGI_SIGNAL_CLOSURE_H__

#include <girepository/girepository.h>
#include <pythoncapi_compat.h>

#include "pygobject-internal.h"

G_BEGIN_DECLS

/* Private */
typedef struct _PyGISignalClosure
{
    PyGClosure pyg_closure;
    GISignalInfo *signal_info;
} PyGISignalClosure;

GClosure *
pygi_signal_closure_new (PyGObject *instance,
                         GType g_type,
                         const gchar *sig_name,
                         PyObject *callback,
                         PyObject *extra_args,
                         PyObject *swap_data);

G_END_DECLS

#endif /* __PYGI_SIGNAL_CLOSURE_H__ */
