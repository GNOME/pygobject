/* -*- Mode: C; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygobject-private.h"
#include "pythread.h"
#include <structmember.h> /* for PyMemberDef */


typedef struct {
    PyObject_HEAD
    GIOChannel *channel;
    int softspace;         /* to make print >> chan, "foo" ... work */
} PyGIOChannel;

static PyObject*
py_io_channel_next(PyGIOChannel *self)
{
    PyObject* ret_obj = NULL;
    gsize length = 0, terminator_pos;
    gchar *str_return = NULL;
    GError *error = NULL;
    GIOStatus status;

    status = g_io_channel_read_line(self->channel, &str_return, &length,
                                    &terminator_pos, &error);
    if (pyg_error_check(&error))
        return NULL;

    if (status == G_IO_STATUS_EOF) {
        PyErr_SetString(PyExc_StopIteration, "EOF");
        return NULL;
    }

    ret_obj = PyString_FromStringAndSize(str_return, length);
    g_free(str_return);
    return ret_obj;

}

static int
py_io_channel_compare(PyGIOChannel *self, PyGIOChannel *v)
{
    if (self->channel == v->channel) return 0;
    if (self->channel > v->channel) return -1;
    return 1;
}

static PyObject*
py_io_channel_get_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

static long
py_io_channel_hash(PyGIOChannel *self)
{
    return (long) self->channel;
}

static void
py_io_channel_dealloc(PyGIOChannel *self)
{
    if (self->channel)
        g_io_channel_unref(self->channel);
    PyObject_DEL(self);
}

static PyObject*
py_io_channel_shutdown(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "flush", NULL };
    GIOStatus ret;
    PyObject* flush = Py_True;
    GError* error = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:gobject.IOChannel.shutdown", kwlist, &flush))
        return NULL;
	
    ret = g_io_channel_shutdown(self->channel, PyObject_IsTrue(flush), &error);
    if (pyg_error_check(&error))
	return NULL;
	
    return PyInt_FromLong(ret);
}

/* character encoding conversion involved functions.
 */
 
static PyObject*
py_io_channel_set_buffer_size(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "size", NULL };
    int size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:gobject.IOChannel.set_buffer_size", kwlist, &size))
        return NULL;
	
    g_io_channel_set_buffer_size(self->channel, size);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
py_io_channel_get_buffer_size(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };
    int size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:gobject.IOChannel.get_buffer_size", kwlist))
        return NULL;
	
    size = g_io_channel_get_buffer_size(self->channel);
    
    return PyInt_FromLong(size);
}

static PyObject*
py_io_channel_set_buffered(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "buffered", NULL };
    int buffered;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:gobject.IOChannel.set_buffered", kwlist, &buffered))
        return NULL;
	
    g_io_channel_set_buffered(self->channel, buffered);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
py_io_channel_get_buffered(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };
    int buffered;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, ":gobject.IOChannel.get_buffered", kwlist))
        return NULL;
	
    buffered = g_io_channel_get_buffered(self->channel);
    
    return PyInt_FromLong(buffered);
}

static PyObject*
py_io_channel_set_encoding(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "encoding", NULL };
    const char* encoding;
    GError* error = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "z:gobject.IOChannel.set_encoding", kwlist, &encoding))
        return NULL;
    
    g_io_channel_set_encoding(self->channel, encoding, &error);
    if (pyg_error_check(&error))
	return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
py_io_channel_get_encoding(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };
    const char* encoding;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, ":gobject.IOChannel.get_encoding", kwlist))
        return NULL;
	
    encoding = g_io_channel_get_encoding(self->channel);
    
    if (encoding == NULL) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    return PyString_FromString(encoding);
}

#define CHUNK_SIZE (8 * 1024)

static PyObject*
py_io_channel_read_chars(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "max_count", NULL };
    int max_count = -1;
    PyObject* ret_obj = NULL;
    gsize total_read = 0;
    GError* error = NULL;
    GIOStatus status = G_IO_STATUS_NORMAL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:gobject.IOChannel.read", kwlist, &max_count))
        return NULL;
	
    if (max_count == 0)
	return PyString_FromString("");
    
    while (status == G_IO_STATUS_NORMAL
	   && (max_count == -1 || total_read < max_count)) {
	gsize single_read;
	char* buf;
	gsize buf_size;
	
	if (max_count == -1) 
	    buf_size = CHUNK_SIZE;
	else {
	    buf_size = max_count - total_read;
	    if (buf_size > CHUNK_SIZE)
		buf_size = CHUNK_SIZE;
        }
	
	if ( ret_obj == NULL ) {
	    ret_obj = PyString_FromStringAndSize((char *)NULL, buf_size);
	    if (ret_obj == NULL)
		goto failure;
	}
	else if (buf_size + total_read > PyString_GET_SIZE(ret_obj)) {
	    if (_PyString_Resize(&ret_obj, buf_size + total_read) == -1)
		goto failure;
	}
       
        buf = PyString_AS_STRING(ret_obj) + total_read;

        pyg_unblock_threads();
        status = g_io_channel_read_chars(self->channel, buf, buf_size, 
                                         &single_read, &error);
        pyg_block_threads();
	if (pyg_error_check(&error))
	    goto failure;
	
	total_read += single_read;
    }
	
    if ( total_read != PyString_GET_SIZE(ret_obj) ) {
	if (_PyString_Resize(&ret_obj, total_read) == -1)
	    goto failure;
    }
    return ret_obj;

  failure:
    Py_XDECREF(ret_obj);
    return NULL;
}

static PyObject*
py_io_channel_write_chars(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "buf", NULL };
    const char* buf;
    Py_ssize_t buf_len;
    gsize count;
    GError* error = NULL;
    GIOStatus status;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#:gobject.IOChannel.write",
                                     kwlist, &buf, &buf_len))
        return NULL;
	
    pyg_unblock_threads();
    status = g_io_channel_write_chars(self->channel, buf, buf_len, &count, &error);
    pyg_block_threads();
    if (pyg_error_check(&error))
	return NULL;
	
    return PyInt_FromLong(count);
}

static PyObject*
py_io_channel_write_lines(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "lines", NULL };
    char *buf;
    Py_ssize_t buf_len;
    gsize count;
    GError* error = NULL;
    GIOStatus status;
    PyObject *iter, *value, *pylines;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:gobject.IOChannel.writelines",
                                     kwlist, &pylines))
        return NULL;

    iter = PyObject_GetIter(pylines);
    
    while (1) {
        value = PyIter_Next(iter);
        if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
            PyErr_Clear();
            goto normal_exit;
        }
        if (!PyString_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "gobject.IOChannel.writelines must"
                            " be sequence/iterator of strings");
            Py_DECREF(iter);
            return NULL;
        }
        PyString_AsStringAndSize(value, &buf, &buf_len);
        pyg_unblock_threads();
        status = g_io_channel_write_chars(self->channel, buf, buf_len, &count, &error);
        pyg_unblock_threads();
        Py_DECREF(value);
        if (pyg_error_check(&error)) {
            Py_DECREF(iter);
            return NULL;
        }
    }
normal_exit:
    Py_DECREF(iter);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
py_io_channel_flush(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };
    GError* error = NULL;
    GIOStatus status;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, ":gobject.IOChannel.flush",
                                     kwlist))
        return NULL;
	
    pyg_unblock_threads();
    status = g_io_channel_flush(self->channel, &error);
    pyg_block_threads();
    if (pyg_error_check(&error))
	return NULL;
	
    return PyInt_FromLong(status);
}

static PyObject*
py_io_channel_set_flags(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "flags", NULL };
    GIOFlags flags;
    GIOStatus status;
    GError* error = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:gobject.IOChannel.set_flags",
                                     kwlist, &flags))
        return NULL;
	
    status = g_io_channel_set_flags(self->channel, flags, &error);
    if (pyg_error_check(&error))
	return NULL;
	
    return PyInt_FromLong(status);
}

static PyObject*
py_io_channel_get_flags(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };
    GIOFlags flags;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, ":gobject.IOChannel.get_flags",
                                     kwlist))
        return NULL;
	
    flags = g_io_channel_get_flags(self->channel);
    return PyInt_FromLong(flags);
}

static PyObject*
py_io_channel_get_buffer_condition(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };
    GIOCondition cond;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, ":gobject.IOChannel.get_buffer_condition",
                                     kwlist))
        return NULL;
	
    cond = g_io_channel_get_buffer_condition(self->channel);
    return PyInt_FromLong(cond);
}

static PyObject*
py_io_channel_set_close_on_unref(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "do_close", NULL };
    PyObject *do_close;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:gobject.IOChannel.set_close_on_unref",
                                     kwlist, &do_close))
        return NULL;
	
    g_io_channel_set_close_on_unref(self->channel, PyObject_IsTrue(do_close));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
py_io_channel_get_close_on_unref(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    if (g_io_channel_get_close_on_unref(self->channel)) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

typedef struct {
    PyObject *callback;
    PyObject *iochannel;
    PyObject *user_data;
} PyGIOWatchData;

static void
pyg_iowatch_data_free(PyGIOWatchData *data)
{
    Py_DECREF(data->callback);
    Py_XDECREF(data->user_data);
    Py_DECREF(data->iochannel);
    g_free(data);
}

static gboolean
pyg_iowatch_marshal(GIOChannel *source,
                    GIOCondition condition,
                    gpointer user_data)
{
    PyObject *ret;
    gboolean res;
    PyGIOWatchData *data = (PyGIOWatchData *) user_data;
    PyGILState_STATE state;

    g_return_val_if_fail(user_data != NULL, FALSE);
    g_return_val_if_fail(((PyGIOChannel *) data->iochannel)->channel == source,
                         FALSE);

    state = pyg_gil_state_ensure();

    if (data->user_data)
        ret = PyObject_CallFunction(data->callback, "OiO", data->iochannel,
                                    condition, data->user_data);
    else
        ret = PyObject_CallFunction(data->callback, "Oi", data->iochannel,
                                    condition);

    if (!ret) {
	PyErr_Print();
	res = FALSE;
    } else {
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }
    pyg_gil_state_release(state);

    return res;
}



static PyObject *
py_io_channel_add_watch(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "condition", "callback", "user_data", "priority", NULL };
    PyObject *callback, *user_data = NULL;
    int priority = G_PRIORITY_DEFAULT, condition;
    GIOChannel *iochannel = NULL;
    guint handler_id;
    PyGIOWatchData *data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "iO|Oi:gobject.IOChannel.add_watch",
                                     kwlist, &condition, &callback,
                                     &user_data, &priority))
        return NULL;

    iochannel = ((PyGIOChannel *) self)->channel;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "second must be callable");
        return NULL;
    }

    data = g_new(PyGIOWatchData, 1);
    data->callback = callback; Py_INCREF(callback);
    data->user_data = user_data; Py_XINCREF(user_data);
    data->iochannel = self; Py_INCREF(self);

    handler_id = g_io_add_watch_full(((PyGIOChannel *) self)->channel,
                                     priority, condition,
				     pyg_iowatch_marshal, data,
				     (GDestroyNotify) pyg_iowatch_data_free);
    return PyLong_FromUnsignedLong(handler_id);
}


#ifdef G_OS_WIN32

static PyObject *
py_io_channel_win32_poll(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "fds", "timeout", NULL };
    GPollFD *pollfd;
    PyObject *pyfds, *pyfd;
    guint len, i;
    gint timeout = -1;
    gint result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O!|i:gobject.IOChannel.win32_poll",
                                     kwlist, &PyList_Type, &pyfds, &timeout))
        return NULL;

    len = PyList_Size(pyfds);
    pollfd = g_newa(GPollFD, len);
    for (i = 0; i < len; ++i) {
        pyfd = PyList_GET_ITEM(pyfds, i);
        if (!PyObject_TypeCheck(pyfd, &PyGPollFD_Type)) {
            PyErr_SetString(PyExc_TypeError, "'fds' must be a list of gobject.PollFD objects");
            return NULL;
        }
        pollfd[i] = ((PyGPollFD *) pyfd)->pollfd;
    }

    result = g_io_channel_win32_poll(pollfd, len, timeout);
    for (i = 0; i < len; ++i) {
        pyfd = PyList_GET_ITEM(pyfds, i);
        ((PyGPollFD *) pyfd)->pollfd = pollfd[i];
    }
    return PyInt_FromLong(result);
}

static PyObject *
py_io_channel_win32_make_pollfd(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "condition", NULL };
    int condition;
    GPollFD pollfd;
    PyGPollFD *pypollfd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "i:gobject.IOChannel.win32_make_pollfd",
                                     kwlist, &condition))
        return NULL;

    g_io_channel_win32_make_pollfd(((PyGIOChannel *) self)->channel,
                                   condition, &pollfd);
    pypollfd = PyObject_NEW(PyGPollFD, &PyGPollFD_Type);
    pypollfd->pollfd = pollfd;
    return (PyObject *) pypollfd;
}
#endif /* def G_OS_WIN32 */


static PyObject*
py_io_channel_read_line(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "size", NULL };
    PyObject* ret_obj = NULL;
    gsize length = 0, terminator_pos;
    gchar *str_return = NULL;
    GError *error = NULL;
    gint size_hint = -1;
    GIOStatus status;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:gobject.IOChannel.readline", kwlist,
                                     &size_hint))
        return NULL;

    status = g_io_channel_read_line(self->channel, &str_return, &length,
                                    &terminator_pos, &error);
    if (pyg_error_check(&error))
        return NULL;
    ret_obj = PyString_FromStringAndSize(str_return, length);
    g_free(str_return);
    return ret_obj;
}

static PyObject*
py_io_channel_read_lines(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "size", NULL };
    PyObject *line = NULL;
    gsize length = 0, terminator_pos;
    gchar *str_return = NULL;
    GError *error = NULL;
    gint size_hint = -1;
    GIOStatus status = G_IO_STATUS_NORMAL;
    PyObject *list;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i:gobject.IOChannel.readlines", kwlist,
                                     &size_hint))
        return NULL;

    list = PyList_New(0);
    while (status == G_IO_STATUS_NORMAL) {
        status = g_io_channel_read_line(self->channel, &str_return, &length,
                                        &terminator_pos, &error);
        if (pyg_error_check(&error)) {
            Py_DECREF(line);
            return NULL;
        }
        line = PyString_FromStringAndSize(str_return, length);
        g_free(str_return);
        if (PyList_Append(list, line)) {
            Py_DECREF(line);
            Py_DECREF(list);
            return NULL;
        }
    }
    return list;
}


static PyObject*
py_io_channel_seek(PyGIOChannel* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "offset", "whence", NULL };
    gint64 offset;
    int whence = 0;
    GIOStatus status;
    GSeekType seek_type;
    GError* error = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "L|i:gobject.IOChannel.seek",
                                     kwlist, &offset, &whence))
        return NULL;

    switch (whence)
    {
    case 0: seek_type = G_SEEK_SET; break;
    case 1: seek_type = G_SEEK_CUR; break;
    case 2: seek_type = G_SEEK_END; break;
    default:
        PyErr_SetString(PyExc_ValueError, "invalid 'whence' value");
        return NULL;
    }
	
    status = g_io_channel_seek_position(self->channel, offset,
                                        seek_type, &error);
    if (pyg_error_check(&error))
	return NULL;
	
    return PyInt_FromLong(status);
}

#if 0 // Not wrapped
void                  g_io_channel_set_line_term        (GIOChannel   *channel,
							 const gchar  *line_term,
							 gint          length);

G_CONST_RETURN gchar* g_io_channel_get_line_term        (GIOChannel   *channel,
							 gint         *length);



GIOStatus   g_io_channel_read_line_string (GIOChannel   *channel,
					   GString      *buffer,
					   gsize        *terminator_pos,
					   GError      **error);
GIOStatus   g_io_channel_read_to_end      (GIOChannel   *channel,
					   gchar       **str_return,
					   gsize        *length,
					   GError      **error);
GIOStatus   g_io_channel_read_unichar     (GIOChannel   *channel,
					   gunichar     *thechar,
					   GError      **error);
GIOStatus   g_io_channel_write_unichar    (GIOChannel   *channel,
					   gunichar      thechar,
					   GError      **error);
#endif // Not wrapped

static PyMemberDef py_io_channel_members[] = {
    { "softspace", T_INT, offsetof(PyGIOChannel, softspace), 0, NULL },
    { NULL, 0, 0, 0, NULL }
};

static PyMethodDef py_io_channel_methods[] = {
    { "close", (PyCFunction)py_io_channel_shutdown, METH_KEYWORDS },
    { "flush", (PyCFunction)py_io_channel_flush, METH_KEYWORDS },
    { "set_encoding", (PyCFunction)py_io_channel_set_encoding, METH_KEYWORDS },
    { "get_encoding", (PyCFunction)py_io_channel_get_encoding, METH_KEYWORDS },
    { "set_buffered", (PyCFunction)py_io_channel_set_buffered, METH_KEYWORDS },
    { "get_buffered", (PyCFunction)py_io_channel_get_buffered, METH_KEYWORDS },
    { "set_buffer_size", (PyCFunction)py_io_channel_set_buffer_size, METH_KEYWORDS },
    { "get_buffer_size", (PyCFunction)py_io_channel_get_buffer_size, METH_KEYWORDS },
    { "read", (PyCFunction)py_io_channel_read_chars, METH_KEYWORDS },
    { "readline", (PyCFunction)py_io_channel_read_line, METH_KEYWORDS },
    { "readlines", (PyCFunction)py_io_channel_read_lines, METH_KEYWORDS },
    { "write", (PyCFunction)py_io_channel_write_chars, METH_KEYWORDS },
    { "writelines", (PyCFunction)py_io_channel_write_lines, METH_KEYWORDS },
    { "set_flags", (PyCFunction)py_io_channel_set_flags, METH_KEYWORDS },
    { "get_flags", (PyCFunction)py_io_channel_get_flags, METH_KEYWORDS },
    { "get_buffer_condition", (PyCFunction)py_io_channel_get_buffer_condition, METH_KEYWORDS },
    { "set_close_on_unref", (PyCFunction)py_io_channel_set_close_on_unref, METH_NOARGS },
    { "get_close_on_unref", (PyCFunction)py_io_channel_get_close_on_unref, METH_KEYWORDS },
    { "add_watch", (PyCFunction)py_io_channel_add_watch, METH_KEYWORDS },
    { "seek", (PyCFunction)py_io_channel_seek, METH_KEYWORDS },
#ifdef G_OS_WIN32
    { "win32_make_pollfd", (PyCFunction)py_io_channel_win32_make_pollfd, METH_KEYWORDS },
    { "win32_poll", (PyCFunction)py_io_channel_win32_poll, METH_KEYWORDS|METH_STATIC },
#endif
    { NULL, NULL, 0 }
};


static int
py_io_channel_init(PyGIOChannel *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "filedes", "filename", "mode",
#ifdef G_OS_WIN32
                              "hwnd",
#endif
                              NULL };
    int fd = -1;
    char *mode = "r", *filename = NULL;
    GError *error = NULL;
#ifdef G_OS_WIN32
    guint hwnd = 0;
#endif

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iss"
#ifdef G_OS_WIN32
                                     "I"
#endif
                                     ":gobject.IOChannel.__init__",
                                     kwlist, &fd, &filename, &mode
#ifdef G_OS_WIN32
                                     , &hwnd
#endif
            ))
        return -1;

    if (fd != -1)
        self->channel = g_io_channel_unix_new(fd);
    else if (filename != NULL) {
        self->channel = g_io_channel_new_file(filename, mode, &error);
        if (pyg_error_check(&error))
            return -1;
    }
#ifdef G_OS_WIN32
    else if (hwnd != 0) {
        self->channel = g_io_channel_win32_new_messages(hwnd);
    }
#endif
    else {
#ifdef G_OS_WIN32
        PyErr_SetString(PyExc_TypeError, "either a valid file descriptor, "
                        "file name, or window handle must be supplied");
#else
        PyErr_SetString(PyExc_TypeError, "either a valid file descriptor "
                        "or file name must be supplied");
#endif
        return -1;
    }
    return 0;
}


PyTypeObject PyGIOChannel_Type = {
    PyVarObject_HEAD_INIT(0, 0)
    "gobject.IOChannel",		/* tp_name */
    sizeof(PyGIOChannel),		/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)py_io_channel_dealloc,	/* tp_dealloc */
    (printfunc)0,			/* tp_print */
    (getattrfunc)NULL,             	/* tp_getattr */
    (setattrfunc)0,			/* tp_setattr */
    (cmpfunc)py_io_channel_compare,	/* tp_compare */
    (reprfunc)0,			/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    (hashfunc)py_io_channel_hash,	/* tp_hash */
    (ternaryfunc)0,			/* tp_call */
    (reprfunc)0,			/* tp_str */
    (getattrofunc)0,			/* tp_getattro */
    (setattrofunc)0,			/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,			/* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,				/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    (getiterfunc)py_io_channel_get_iter,/* tp_iter */
    (iternextfunc)py_io_channel_next,   /* tp_iternext */
    py_io_channel_methods,		/* tp_methods */
    py_io_channel_members,		/* tp_members */
    NULL,				/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)py_io_channel_init,	/* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
    0,					/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

