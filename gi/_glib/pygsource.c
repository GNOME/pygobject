/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2005       Oracle
 *
 * Author: Manish Singh <manish.singh@oracle.com>
 *
 *   pygsource.c: GSource wrapper
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Python.h>
#include <structmember.h> /* for PyMemberDef */
#include "pyglib.h"
#include "pyglib-private.h"
#include "pygsource.h"

/* glib.PollFD */

PYGLIB_DEFINE_TYPE("gi._glib.PollFD", PyGPollFD_Type, PyGPollFD)

static PyMemberDef pyg_poll_fd_members[] = {
    { "fd",      T_INT,    offsetof(PyGPollFD, pollfd.fd),      READONLY },
    { "events",  T_USHORT, offsetof(PyGPollFD, pollfd.events),  READONLY },
    { "revents", T_USHORT, offsetof(PyGPollFD, pollfd.revents), READONLY },
    { NULL, 0, 0, 0 }
};

static void
pyg_poll_fd_dealloc(PyGPollFD *self)
{
    Py_XDECREF(self->fd_obj);
    PyObject_DEL(self);
}

static PyObject *
pyg_poll_fd_repr(PyGPollFD *self)
{
    return PYGLIB_PyUnicode_FromFormat("<GPollFD %d (%d) at 0x%lx>",
				 self->pollfd.fd, self->pollfd.events,
				 (long)self);
}

static int
pyg_poll_fd_init(PyGPollFD *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "fd", "events", NULL };
    PyObject *o;
    gint fd;
    gushort events;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "OH:gi._glib.PollFD.__init__", kwlist,
				     &o, &events))
	return -1;

    fd = PyObject_AsFileDescriptor(o);
    if (fd == -1)
	return -1;

    self->pollfd.fd = fd;
    self->pollfd.events = events;
    self->pollfd.revents = 0;

    Py_INCREF(o);
    self->fd_obj = o;

    return 0;
}

void
pyglib_source_register_types(PyObject *d)
{
    PyGPollFD_Type.tp_dealloc = (destructor)pyg_poll_fd_dealloc;
    PyGPollFD_Type.tp_repr = (reprfunc)pyg_poll_fd_repr;
    PyGPollFD_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPollFD_Type.tp_members = pyg_poll_fd_members;
    PyGPollFD_Type.tp_init = (initproc)pyg_poll_fd_init;
    PYGLIB_REGISTER_TYPE(d, PyGPollFD_Type, "PollFD");
}
