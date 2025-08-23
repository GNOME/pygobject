#include <string.h>

#include <glib-object.h>
#include <gobject/gmarshal.h>
#include <pythoncapi_compat.h>

#include "pygobject.h"
#include "test-floating.h"
#include "test-thread.h"
#include "test-unknown.h"

#define PYGI_DEFINE_TYPE(typename, symbol, csymbol)                           \
    PyTypeObject symbol = { PyVarObject_HEAD_INIT (NULL, 0) typename,         \
                            sizeof (csymbol) };

static PyObject *_wrap_TestInterface__do_iface_method (PyObject *cls,
                                                       PyObject *args,
                                                       PyObject *kwargs);

static GType
test_type_get_type (void)
{
    static GType gtype = 0;
    GType parent_type;

    if (gtype == 0) {
        GTypeInfo *type_info;
        GTypeQuery query;

        parent_type = g_type_from_name ("PyGObject");
        if (parent_type == 0)
            g_error ("could not get PyGObject from testmodule");

        type_info = (GTypeInfo *)g_new0 (GTypeInfo, 1);

        g_type_query (parent_type, &query);
        type_info->class_size = (guint16)query.class_size;
        type_info->instance_size = (guint16)query.instance_size;

        gtype = g_type_register_static (parent_type, "TestType", type_info, 0);
        if (!gtype) g_error ("Could not register TestType");
    }

    return gtype;
}

#define TYPE_TEST (test_type_get_type ())

static PyObject *
_wrap_get_test_thread (PyObject *self)
{
    GObject *obj;

    test_thread_get_type ();
    g_assert (g_type_is_a (TEST_TYPE_THREAD, G_TYPE_OBJECT));
    obj = g_object_new (TEST_TYPE_THREAD, NULL);
    g_assert (obj != NULL);

    return pygobject_new (obj);
}

static PyObject *
_wrap_get_unknown (PyObject *self)
{
    GObject *obj;
    obj = g_object_new (TEST_TYPE_UNKNOWN, NULL);
    return pygobject_new (obj);
}

static PyObject *
_wrap_create_test_type (PyObject *self)
{
    GObject *obj;
    PyObject *rv;
    obj = g_object_new (TYPE_TEST, NULL);
    rv = pygobject_new (obj);
    g_object_unref (obj);
    return rv;
}

static PyObject *
_wrap_test_g_object_new (PyObject *self)
{
    GObject *obj;
    PyObject *rv;

    obj = g_object_new (g_type_from_name ("PyGObject"), NULL);
    rv = PyLong_FromLong (obj->ref_count); /* should be == 2 at this point */
    g_object_unref (obj);
    return rv;
}

/* TestUnknown */
static PyObject *
_wrap_test_interface_iface_method (PyGObject *self, PyObject *args,
                                   PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, ":", kwlist)) return NULL;

    test_interface_iface_method (TEST_INTERFACE (self->obj));

    Py_RETURN_NONE;
}

static const PyMethodDef _PyTestInterface_methods[] = {
    { "iface_method", (PyCFunction)_wrap_test_interface_iface_method,
      METH_VARARGS | METH_KEYWORDS, NULL },
    { "do_iface_method", (PyCFunction)_wrap_TestInterface__do_iface_method,
      METH_VARARGS | METH_KEYWORDS | METH_CLASS, NULL },
    { NULL, NULL, 0, NULL }
};

/* TestInterface */
PYGI_DEFINE_TYPE ("test.Interface", PyTestInterface_Type, PyObject);

static PyObject *
_wrap_TestInterface__do_iface_method (PyObject *cls, PyObject *args,
                                      PyObject *kwargs)
{
    TestInterfaceIface *iface;
    static char *kwlist[] = { "self", NULL };
    PyGObject *self;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs,
                                      "O!:TestInterface.iface_method", kwlist,
                                      &PyTestInterface_Type, &self))
        return NULL;

    iface = g_type_interface_peek (
        g_type_class_peek (pyg_type_from_object (cls)), TEST_TYPE_INTERFACE);
    if (iface->iface_method)
        iface->iface_method (TEST_INTERFACE (self->obj));
    else {
        PyErr_SetString (
            PyExc_NotImplementedError,
            "interface method TestInterface.iface_method not implemented");
        return NULL;
    }
    Py_RETURN_NONE;
}

PYGI_DEFINE_TYPE ("testhelper.Unknown", PyTestUnknown_Type, PyGObject);

static void
_wrap_TestInterface__proxy_do_iface_method (TestInterface *self)
{
    PyGILState_STATE __py_state;
    PyObject *py_self;
    PyObject *py_retval;
    PyObject *py_args;
    PyObject *py_method;

    __py_state = PyGILState_Ensure ();
    py_self = pygobject_new ((GObject *)self);
    if (!py_self) {
        if (PyErr_Occurred ()) PyErr_Print ();
        PyGILState_Release (__py_state);
        return;
    }
    py_args = PyTuple_New (0);
    py_method = PyObject_GetAttrString (py_self, "do_iface_method");
    if (!py_method) {
        if (PyErr_Occurred ()) PyErr_Print ();
        Py_DECREF (py_args);
        Py_DECREF (py_self);
        PyGILState_Release (__py_state);
        return;
    }
    py_retval = PyObject_CallObject (py_method, py_args);
    if (!py_retval) {
        if (PyErr_Occurred ()) PyErr_Print ();
        Py_DECREF (py_method);
        Py_DECREF (py_args);
        Py_DECREF (py_self);
        PyGILState_Release (__py_state);
        return;
    }
    if (!Py_IsNone (py_retval)) {
        if (PyErr_Occurred ()) PyErr_Print ();
        PyErr_SetString (PyExc_TypeError, "retval should be None");
        Py_DECREF (py_retval);
        Py_DECREF (py_method);
        Py_DECREF (py_args);
        Py_DECREF (py_self);
        PyGILState_Release (__py_state);
        return;
    }

    Py_DECREF (py_retval);
    Py_DECREF (py_method);
    Py_DECREF (py_args);
    Py_DECREF (py_self);
    PyGILState_Release (__py_state);
}

static void
__TestInterface__interface_init (TestInterfaceIface *iface,
                                 PyTypeObject *pytype)
{
    TestInterfaceIface *parent_iface = g_type_interface_peek_parent (iface);
    PyObject *py_method;

    py_method =
        pytype ? PyObject_GetAttrString ((PyObject *)pytype, "do_iface_method")
               : NULL;

    if (py_method && !PyObject_TypeCheck (py_method, &PyCFunction_Type)) {
        iface->iface_method = _wrap_TestInterface__proxy_do_iface_method;
    } else {
        PyErr_Clear ();
        if (parent_iface) {
            iface->iface_method = parent_iface->iface_method;
        }
        Py_XDECREF (py_method);
    }
}

static const GInterfaceInfo __TestInterface__iinfo = {
    (GInterfaceInitFunc)__TestInterface__interface_init, NULL, NULL
};

/* TestFloating */
PYGI_DEFINE_TYPE ("testhelper.Floating", PyTestFloating_Type, PyGObject);

/* TestOwnedByLibrary */
PYGI_DEFINE_TYPE ("testhelper.OwnedByLibrary", PyTestOwnedByLibrary_Type,
                  PyGObject);

static PyObject *
_wrap_test_owned_by_library_release (PyGObject *self)
{
    test_owned_by_library_release (TEST_OWNED_BY_LIBRARY (self->obj));
    return Py_None;
}

static const PyMethodDef _PyTestOwnedByLibrary_methods[] = {
    { "release", (PyCFunction)_wrap_test_owned_by_library_release, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

/* TestFloatingAndSunk */
PYGI_DEFINE_TYPE ("testhelper.FloatingAndSunk", PyTestFloatingAndSunk_Type,
                  PyGObject);

static PyObject *
_wrap_test_floating_and_sunk_release (PyGObject *self)
{
    test_floating_and_sunk_release (TEST_FLOATING_AND_SUNK (self->obj));
    return Py_None;
}

static const PyMethodDef _PyTestFloatingAndSunk_methods[] = {
    { "release", (PyCFunction)_wrap_test_floating_and_sunk_release,
      METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL }
};

static void
test1_callback (GObject *object, char *data)
{
    g_return_if_fail (G_IS_OBJECT (object));
    g_return_if_fail (!strcmp (data, "user-data"));
}

static void
test1_callback_swapped (char *data, GObject *object)
{
    g_return_if_fail (G_IS_OBJECT (object));
    g_return_if_fail (!strcmp (data, "user-data"));
}

static void
test2_callback (GObject *object, char *string)
{
    g_return_if_fail (G_IS_OBJECT (object));
    g_return_if_fail (!strcmp (string, "string"));
}

static int
test3_callback (GObject *object, double d)
{
    g_return_val_if_fail (G_IS_OBJECT (object), -1);
    g_return_val_if_fail (d == 42.0, -1);

    return 20;
}

static void
test4_callback (GObject *object, gboolean b, long l, float f, double d,
                guint uint, gulong ulong, gpointer user_data)
{
    g_return_if_fail (b == TRUE);
    g_return_if_fail (l == 10L);
    g_return_if_fail (f <= 3.14001 && f >= 3.13999);
    g_return_if_fail (d <= 1.78001 && d >= 1.77999);
    g_return_if_fail (uint == 20);
    g_return_if_fail (ulong == 30L);
}

static float
test_float_callback (GObject *object, float f)
{
    g_return_val_if_fail (G_IS_OBJECT (object), -1);
    g_return_val_if_fail (f <= 1.234001 && f >= 1.123999, -1);

    return f;
}

static double
test_double_callback (GObject *object, double d)
{
    g_return_val_if_fail (G_IS_OBJECT (object), -1);
    g_return_val_if_fail (d <= 1.234001 && d >= 1.123999, -1);

    return d;
}

static gint64
test_int64_callback (GObject *object, gint64 i)
{
    g_return_val_if_fail (G_IS_OBJECT (object), -1);

    if (i == G_MAXINT64) return i - 1;
    return i;
}

static char *
test_string_callback (GObject *object, char *s)
{
    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    g_return_val_if_fail (!strcmp (s, "str"), NULL);

    return g_strdup (s);
}

static GObject *
test_object_callback (GObject *object, GObject *o)
{
    g_return_val_if_fail (G_IS_OBJECT (object), NULL);

    return o;
}

static GParamSpec *
test_paramspec_callback (GObject *object)
{
    g_return_val_if_fail (G_IS_OBJECT (object), NULL);

    return g_param_spec_boolean ("test-param", "test", "test boolean", TRUE,
                                 G_PARAM_READABLE);
}

static GValue *
test_gvalue_callback (GObject *object, const GValue *v)
{
    GValue *ret;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    g_return_val_if_fail (G_IS_VALUE (v), NULL);

    ret = g_malloc0 (sizeof (GValue));
    g_value_init (ret, G_VALUE_TYPE (v));
    g_value_copy (v, ret);
    return ret;
}

static GValue *
test_gvalue_ret_callback (GObject *object, GType type)
{
    GValue *ret;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);

    ret = g_malloc0 (sizeof (GValue));
    g_value_init (ret, type);

    switch (type) {
    case G_TYPE_INT:
        g_value_set_int (ret, G_MAXINT);
        break;
    case G_TYPE_INT64:
        g_value_set_int64 (ret, G_MAXINT64);
        break;
    case G_TYPE_UINT:
        g_value_set_uint (ret, G_MAXUINT);
        break;
    case G_TYPE_UINT64:
        g_value_set_uint64 (ret, G_MAXUINT64);
        break;
    case G_TYPE_STRING:
        g_value_set_string (ret, "hello");
        break;
    default:
        g_critical ("test_gvalue_ret_callback() does not support type %s",
                    g_type_name (type));
    }

    return ret;
}

static GParamSpec *
test_paramspec_in_callback (GObject *object, GParamSpec *p)
{
    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    g_return_val_if_fail (G_IS_PARAM_SPEC (p), NULL);

    return p;
}

static void
connectcallbacks (GObject *object)
{
    gchar *data = "user-data";

    g_signal_connect (G_OBJECT (object), "test1", G_CALLBACK (test1_callback),
                      data);
    g_signal_connect_swapped (G_OBJECT (object), "test1",
                              G_CALLBACK (test1_callback_swapped), data);
    g_signal_connect (G_OBJECT (object), "test2", G_CALLBACK (test2_callback),
                      NULL);
    g_signal_connect (G_OBJECT (object), "test3", G_CALLBACK (test3_callback),
                      NULL);
    g_signal_connect (G_OBJECT (object), "test4", G_CALLBACK (test4_callback),
                      NULL);
    g_signal_connect (G_OBJECT (object), "test_float",
                      G_CALLBACK (test_float_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_double",
                      G_CALLBACK (test_double_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_int64",
                      G_CALLBACK (test_int64_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_string",
                      G_CALLBACK (test_string_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_object",
                      G_CALLBACK (test_object_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_paramspec",
                      G_CALLBACK (test_paramspec_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_gvalue",
                      G_CALLBACK (test_gvalue_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_gvalue_ret",
                      G_CALLBACK (test_gvalue_ret_callback), NULL);
    g_signal_connect (G_OBJECT (object), "test_paramspec_in",
                      G_CALLBACK (test_paramspec_in_callback), NULL);
}

static PyObject *
_wrap_connectcallbacks (PyObject *self, PyObject *args)
{
    PyGObject *obj;

    if (!PyArg_ParseTuple (args, "O", &obj)) return NULL;

    connectcallbacks (G_OBJECT (obj->obj));

    Py_RETURN_NONE;
}

static PyObject *
_wrap_test_value (PyObject *self, PyObject *args)
{
    GValue tvalue = {0,}, *value = &tvalue;
    PyObject *obj;

    if (!PyArg_ParseTuple (args, "O", &obj)) return NULL;

    g_value_init (value, G_TYPE_VALUE);
    if (pyg_value_from_pyobject (value, obj)) {
        PyErr_SetString (PyExc_TypeError, "Could not convert to GValue");
        return NULL;
    }

    return pyg_value_as_pyobject (value, FALSE);
}

static PyObject *
_wrap_test_state_ensure_release (PyObject *self, PyObject *args)
{
    int state = pyg_gil_state_ensure ();
    pyg_gil_state_release (state);

    Py_RETURN_NONE;
}

#define PYGI_TYPE_VALUE_ARRAY (g_value_array_get_type ())

static PyObject *
_wrap_test_value_array (PyObject *self, PyObject *args)
{
    GValue tvalue = {0,}, *value = &tvalue;
    PyObject *obj;

    if (!PyArg_ParseTuple (args, "O", &obj)) return NULL;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    g_value_init (value, PYGI_TYPE_VALUE_ARRAY);
    G_GNUC_END_IGNORE_DEPRECATIONS

    if (pyg_value_from_pyobject (value, obj)) {
        PyErr_SetString (PyExc_TypeError, "Could not convert to GValueArray");
        return NULL;
    }

    return pyg_value_as_pyobject (value, FALSE);
}


static PyObject *
_wrap_value_array_get_nth_type (PyObject *self, PyObject *args)
{
    guint n;
    GType type;
    GValue *nth;
    GValueArray *arr;
    PyObject *obj;

    if (!PyArg_ParseTuple (args, "OI", &obj, &n)) return NULL;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS

    if (pyg_boxed_check (obj, G_TYPE_VALUE)
        && G_VALUE_HOLDS (pyg_boxed_get (obj, GValue),
                          PYGI_TYPE_VALUE_ARRAY)) {
        arr = g_value_get_boxed (pyg_boxed_get (obj, GValue));
    } else if (pyg_boxed_check (obj, PYGI_TYPE_VALUE_ARRAY)) {
        arr = pyg_boxed_get (obj, GValueArray);
    } else {
        PyErr_SetString (PyExc_TypeError, "First argument is not GValueArray");
        return NULL;
    }

    if (n >= arr->n_values) {
        PyErr_SetString (PyExc_TypeError, "Index is out of bounds");
        return NULL;
    }
    nth = g_value_array_get_nth (arr, n);
    type = G_VALUE_TYPE (nth);

    G_GNUC_END_IGNORE_DEPRECATIONS

    return pyg_type_wrapper_new (type);
}

static PyObject *
_wrap_constant_strip_prefix (PyObject *self, PyObject *args)
{
    const char *name, *strip_prefix;
    const gchar *result;

    if (!PyArg_ParseTuple (args, "ss", &name, &strip_prefix)) return NULL;

    result = pyg_constant_strip_prefix (name, strip_prefix);
    return PyUnicode_FromString (result);
}

static PyObject *
_wrap_test_gerror_exception (PyObject *self, PyObject *args)
{
    PyObject *py_method;
    PyObject *py_args;
    PyObject *py_ret;
    GError *err = NULL;

    if (!PyArg_ParseTuple (args, "O", &py_method)) return NULL;

    py_args = PyTuple_New (0);
    py_ret = PyObject_CallObject (py_method, py_args);
    if (pyg_gerror_exception_check (&err)) {
        pyg_error_check (&err);
        return NULL;
    }

    Py_DECREF (py_args);
    Py_DECREF (py_ret);

    Py_RETURN_NONE;
}

static PyObject *
_wrap_test_owned_by_library_get_instance_list (PyObject *self)
{
    PyObject *py_list, *py_obj;
    GSList *list, *tmp;

    list = test_owned_by_library_get_instance_list ();

    if ((py_list = PyList_New (0)) == NULL) {
        return NULL;
    }
    for (tmp = list; tmp != NULL; tmp = tmp->next) {
        py_obj = pygobject_new (G_OBJECT (tmp->data));
        if (py_obj == NULL) {
            Py_DECREF (py_list);
            return NULL;
        }
        PyList_Append (py_list, py_obj);
        Py_DECREF (py_obj);
    }
    return py_list;
}

static PyObject *
_wrap_test_floating_and_sunk_get_instance_list (PyObject *self)
{
    PyObject *py_list, *py_obj;
    GSList *list, *tmp;

    list = test_floating_and_sunk_get_instance_list ();

    if ((py_list = PyList_New (0)) == NULL) {
        return NULL;
    }
    for (tmp = list; tmp != NULL; tmp = tmp->next) {
        py_obj = pygobject_new (G_OBJECT (tmp->data));
        if (py_obj == NULL) {
            Py_DECREF (py_list);
            return NULL;
        }
        PyList_Append (py_list, py_obj);
        Py_DECREF (py_obj);
    }
    return py_list;
}


static PyObject *
_wrap_force_g_object_ref (PyObject *self, PyObject *args)
{
    PyGObject *obj;

    if (!PyArg_ParseTuple (args, "O", &obj)) return NULL;

    g_assert (G_IS_OBJECT (obj->obj));
    g_object_ref (obj->obj);

    Py_RETURN_NONE;
}

static PyObject *
_wrap_force_g_object_unref (PyObject *self, PyObject *args)
{
    PyGObject *obj;

    if (!PyArg_ParseTuple (args, "O", &obj)) return NULL;

    g_assert (G_IS_OBJECT (obj->obj));
    g_object_unref (obj->obj);

    Py_RETURN_NONE;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static PyObject *
_wrap_test_parse_constructor_args (PyObject *self, PyObject *args)
{
    char *arg_names[] = { "label", NULL };
    char *prop_names[] = { "label", NULL };
    GParameter params[1] = { { 0 } };
    PyObject *parsed_args[1];
    guint nparams = 0;

    if (!PyArg_ParseTuple (args, "O", &(parsed_args[0]))) return NULL;

    if (!pyg_parse_constructor_args (TYPE_TEST, arg_names, prop_names, params,
                                     &nparams, parsed_args)) {
        return NULL;
    }

    return PyLong_FromLong (nparams);
}

G_GNUC_END_IGNORE_DEPRECATIONS

static PyObject *
_wrap_test_to_unichar_conv (PyObject *self, PyObject *args)
{
    PyObject *obj;
    gunichar result;

    if (!PyArg_ParseTuple (args, "O", &obj)) return NULL;

    if (!pyg_pyobj_to_unichar_conv (obj, &result)) return NULL;

    return PyLong_FromLong (result);
}

static PyMethodDef testhelper_functions[] = {
    { "force_g_object_ref", (PyCFunction)_wrap_force_g_object_ref,
      METH_VARARGS },
    { "force_g_object_unref", (PyCFunction)_wrap_force_g_object_unref,
      METH_VARARGS },
    { "test_parse_constructor_args",
      (PyCFunction)_wrap_test_parse_constructor_args, METH_VARARGS },
    { "get_test_thread", (PyCFunction)_wrap_get_test_thread, METH_NOARGS },
    { "test_to_unichar_conv", (PyCFunction)_wrap_test_to_unichar_conv,
      METH_VARARGS },
    { "get_unknown", (PyCFunction)_wrap_get_unknown, METH_NOARGS },
    { "create_test_type", (PyCFunction)_wrap_create_test_type, METH_NOARGS },
    { "test_state_ensure_release",
      (PyCFunction)_wrap_test_state_ensure_release, METH_NOARGS },
    { "test_g_object_new", (PyCFunction)_wrap_test_g_object_new, METH_NOARGS },
    { "connectcallbacks", (PyCFunction)_wrap_connectcallbacks, METH_VARARGS },
    { "test_value", (PyCFunction)_wrap_test_value, METH_VARARGS },
    { "test_value_array", (PyCFunction)_wrap_test_value_array, METH_VARARGS },
    { "value_array_get_nth_type", (PyCFunction)_wrap_value_array_get_nth_type,
      METH_VARARGS },
    { "constant_strip_prefix", (PyCFunction)_wrap_constant_strip_prefix,
      METH_VARARGS },
    { "test_gerror_exception", (PyCFunction)_wrap_test_gerror_exception,
      METH_VARARGS },
    { "owned_by_library_get_instance_list",
      (PyCFunction)_wrap_test_owned_by_library_get_instance_list,
      METH_NOARGS },
    { "floating_and_sunk_get_instance_list",
      (PyCFunction)_wrap_test_floating_and_sunk_get_instance_list,
      METH_NOARGS },
    { NULL, NULL }
};

static struct PyModuleDef _testhelpermodule = { PyModuleDef_HEAD_INIT,
                                                "testhelper",
                                                NULL,
                                                -1,
                                                testhelper_functions,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL };

#ifdef __GNUC__
#define PYGI_MODINIT_FUNC                                                     \
    __attribute__ ((visibility ("default"))) PyMODINIT_FUNC
#else
#define PYGI_MODINIT_FUNC PyMODINIT_FUNC
#endif

PYGI_MODINIT_FUNC PyInit_testhelper (void);

PYGI_MODINIT_FUNC
PyInit_testhelper (void)
{
    PyObject *module;
    PyObject *gobject_module;
    PyObject *m, *d;

    module = PyModule_Create (&_testhelpermodule);

    if ((gobject_module = pygobject_init (-1, -1, -1)) == NULL) return NULL;
    Py_DECREF (gobject_module);

    d = PyModule_GetDict (module);

    if ((m = PyImport_ImportModule ("gi.repository.GObject")) == NULL) {
        PyErr_SetString (PyExc_ImportError, "could not import gobject");
        return NULL;
    }

    /* TestInterface */
    PyTestInterface_Type.tp_methods =
        (struct PyMethodDef *)_PyTestInterface_methods;
    PyTestInterface_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    pyg_register_interface (d, "Interface", TEST_TYPE_INTERFACE,
                            &PyTestInterface_Type);
    pyg_register_interface_info (TEST_TYPE_INTERFACE, &__TestInterface__iinfo);


    /* TestUnknown */
    PyTestUnknown_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyTestUnknown_Type.tp_weaklistoffset = offsetof (PyGObject, weakreflist);
    PyTestUnknown_Type.tp_dictoffset = offsetof (PyGObject, inst_dict);
    pygobject_register_class (
        d, "Unknown", TEST_TYPE_UNKNOWN, &PyTestUnknown_Type,
        Py_BuildValue ("(O)", &PyGObject_Type, &PyTestInterface_Type));

    /* TestFloating */
    PyTestFloating_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyTestFloating_Type.tp_weaklistoffset = offsetof (PyGObject, weakreflist);
    PyTestFloating_Type.tp_dictoffset = offsetof (PyGObject, inst_dict);
    pygobject_register_class (d, "Floating", TEST_TYPE_FLOATING,
                              &PyTestFloating_Type,
                              Py_BuildValue ("(O)", &PyGObject_Type));

    /* TestOwnedByLibrary */
    PyTestOwnedByLibrary_Type.tp_flags =
        (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyTestOwnedByLibrary_Type.tp_methods =
        (struct PyMethodDef *)_PyTestOwnedByLibrary_methods;
    PyTestOwnedByLibrary_Type.tp_weaklistoffset =
        offsetof (PyGObject, weakreflist);
    PyTestOwnedByLibrary_Type.tp_dictoffset = offsetof (PyGObject, inst_dict);
    pygobject_register_class (d, "OwnedByLibrary", TEST_TYPE_OWNED_BY_LIBRARY,
                              &PyTestOwnedByLibrary_Type,
                              Py_BuildValue ("(O)", &PyGObject_Type));

    /* TestFloatingAndSunk */
    PyTestFloatingAndSunk_Type.tp_flags =
        (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyTestFloatingAndSunk_Type.tp_methods =
        (struct PyMethodDef *)_PyTestFloatingAndSunk_methods;
    PyTestFloatingAndSunk_Type.tp_weaklistoffset =
        offsetof (PyGObject, weakreflist);
    PyTestFloatingAndSunk_Type.tp_dictoffset = offsetof (PyGObject, inst_dict);
    pygobject_register_class (
        d, "FloatingAndSunk", TEST_TYPE_FLOATING_AND_SUNK,
        &PyTestFloatingAndSunk_Type, Py_BuildValue ("(O)", &PyGObject_Type));

    return module;
}
