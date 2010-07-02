/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2010 Johan Dahlin <johan@gnome.org>
 *
 *   pygi-llvm-compiler.cpp: LLVM PyGI integration
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include "llvm-compiler.h"

extern "C" {

PyObject *
_wrap_g_function_info_llvm_compile(PyGIBaseInfo *self, PyObject *args)
{
  PyCFunction func;
  PyObject *pyfunc;
  GIFunctionInfo *info;
  PyMethodDef *def;

  g_print("Compiling...%s\n", g_function_info_get_symbol(self->info));
  func = llvm_compile(self->info);
  g_assert(func != NULL);

  def = (PyMethodDef*)malloc(sizeof(PyMethodDef*));
  def->ml_name = g_base_info_get_name((GIBaseInfo*)self->info);
  def->ml_meth = func;
  int n_args = g_callable_info_get_n_args((GICallableInfo*)self->info);
  if (n_args == 0) {
    def->ml_flags = METH_NOARGS;
  } else if (n_args == 1) {
    def->ml_flags = METH_O;
  } else {
    def->ml_flags = METH_VARARGS;
  }

  pyfunc = PyCFunction_New(def, (PyObject*)self);

  Py_INCREF(pyfunc);
  return pyfunc;
}

}
