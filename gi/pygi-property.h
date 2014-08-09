/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2010  Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef __PYGI_PROPERTY_H__
#define __PYGI_PROPERTY_H__

#include <Python.h>
#include <girepository.h>

#include "pygobject.h"

PyObject *
pygi_get_property_value (PyGObject *instance,
                         GParamSpec *pspec);

PyObject *
pygi_get_property_value_by_name (PyGObject *self,
                                 gchar *param_name);
PyObject *
pygi_call_do_get_property       (PyObject *instance,
                                 GParamSpec *pspec);

gint
pygi_set_property_value (PyGObject *instance,
                         GParamSpec *pspec,
                         PyObject *py_value);

#endif /* __PYGI_PROPERTY_H__ */
