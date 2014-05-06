/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2010  litl, LLC
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

#ifndef __PYGI_FOREIGN_H__
#define __PYGI_FOREIGN_H__

#include <Python.h>
#include "pygi-foreign-api.h"

PyObject *pygi_struct_foreign_convert_to_g_argument (PyObject           *value,
                                                     GIInterfaceInfo    *interface_info,
                                                     GITransfer          transfer,
                                                     GIArgument         *arg);
PyObject *pygi_struct_foreign_convert_from_g_argument (GIInterfaceInfo *interface_info,
                                                       GITransfer       transfer,
                                                       GIArgument      *arg);
PyObject *pygi_struct_foreign_release (GITypeInfo *type_info,
                                       gpointer struct_);

void pygi_register_foreign_struct (const char* namespace_,
                                   const char* name,
                                   PyGIArgOverrideToGIArgumentFunc to_func,
                                   PyGIArgOverrideFromGIArgumentFunc from_func,
                                   PyGIArgOverrideReleaseFunc release_func);

PyObject *pygi_require_foreign    (PyObject *self,
                                   PyObject *args,
                                   PyObject *kwargs);

void pygi_foreign_init (void);

#endif /* __PYGI_FOREIGN_H__ */
