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
#include <girepository.h>

typedef gboolean (*PyGIArgOverrideToGArgumentFunc) (PyObject       *value,
                                                    GITypeInfo     *type_info,
                                                    GITransfer      transfer,
                                                    GArgument      *arg);

typedef PyObject * (*PyGIArgOverrideFromGArgumentFunc) (GITypeInfo *type_info,
                                                        GArgument  *arg);
typedef gboolean (*PyGIArgOverrideReleaseGArgumentFunc) (GITransfer  transfer,
                                                         GITypeInfo *type_info,
                                                         GArgument  *arg);

gboolean pygi_struct_foreign_convert_to_g_argument (PyObject           *value,
                                                    GITypeInfo         *type_info,
                                                    GITransfer          transfer,
                                                    GArgument          *arg);
PyObject *pygi_struct_foreign_convert_from_g_argument (GITypeInfo *type_info,
                                                       GArgument  *arg);
gboolean pygi_struct_foreign_release_g_argument (GITransfer          transfer,
                                                 GITypeInfo         *type_info,
                                                 GArgument          *arg);

#endif /* __PYGI_FOREIGN_H__ */
