#include "pygobject.h"
#include <gobject/gmarshal.h>

typedef struct {
  GObject parent;
  GThread *thread;
} TestThread;

typedef struct {
  GObjectClass parent_class;
  void (*emit_signal) (TestThread *sink);
  void (*from_thread)	(TestThread *sink);
} TestThreadClass;

static void  test_thread_init       (TestThread *self);
static void  test_thread_class_init (TestThreadClass *class);
GType        test_thread_get_type   (void);

#define TEST_TYPE_THREAD            (test_thread_get_type())
#define TEST_THREAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_THREAD, TestTHREAD))
#define TEST_THREAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_THREAD, TestTHREADClass))
#define TEST_IS_THREAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_THREAD))
#define TEST_IS_THREAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TEST_TYPE_THREAD))
#define TEST_THREAD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), TEST_TYPE_THREAD, TestTHREADClass))

enum
{
  /* methods */
  SIGNAL_EMIT_SIGNAL,
  SIGNAL_FROM_THREAD,
  LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;

static guint test_thread_signals[LAST_SIGNAL] = { 0 };

typedef enum {
  TEST_THREAD_A,
  TEST_THREAD_B
} ThreadEnumType;

static GType
test_thread_enum_get_type (void)
{
  static GType enum_type = 0;
  static GEnumValue enum_values[] = {
    {TEST_THREAD_A, "TEST_THREAD_A", "a as in apple"},
    {0, NULL, NULL},
  };

  if (!enum_type) {
    enum_type =
        g_enum_register_static ("TestThreadEnum", enum_values);
  }
  return enum_type;
}

GType
test_thread_get_type (void)
{
    static GType thread_type = 0;

    if (!thread_type) {
	static const GTypeInfo thread_info = {
	    sizeof(TestThreadClass),
	    (GBaseInitFunc) NULL,
	    (GBaseFinalizeFunc) NULL,
	    (GClassInitFunc) test_thread_class_init,
	    (GClassFinalizeFunc) NULL,
	    NULL,

	    sizeof (TestThread),
	    0, /* n_preallocs */
	    (GInstanceInitFunc) test_thread_init,
	};

	thread_type = g_type_register_static(G_TYPE_OBJECT, "TestThread",
					     &thread_info, 0);
    }
    
    return thread_type;

}

static void
other_thread_cb (TestThread *self)
{
  g_signal_emit_by_name (self, "from-thread", 0, NULL);
  g_thread_exit (0);
}

static void
test_thread_emit_signal (TestThread *self)
{
  self->thread = g_thread_create ((GThreadFunc)other_thread_cb,
				  self, TRUE, NULL);
}

static void test_thread_init (TestThread *self) {}
static void test_thread_class_init (TestThreadClass *klass)
{
  parent_class = g_type_class_ref (G_TYPE_OBJECT);

  test_thread_signals[SIGNAL_EMIT_SIGNAL] =
    g_signal_new ("emit-signal", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (TestThreadClass, emit_signal),
		  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  test_thread_signals[SIGNAL_FROM_THREAD] =
    g_signal_new ("from-thread", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (TestThreadClass, from_thread),
		  NULL, NULL, g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1,
		  test_thread_enum_get_type ());

  klass->emit_signal = test_thread_emit_signal;
}

static PyObject *
_wrap_get_tp_basicsize (PyObject * self, PyObject * args)
{
  PyObject *item = PyTuple_GetItem(args, 0);
  return PyInt_FromLong(((PyTypeObject*)item)->tp_basicsize);
}

static PyObject *
_wrap_get_test_thread (PyObject * self)
{
  GObject *obj;

  test_thread_get_type();
  g_assert (g_type_is_a (TEST_TYPE_THREAD, G_TYPE_OBJECT));
  obj = g_object_new (TEST_TYPE_THREAD, NULL);
  g_assert (obj != NULL);
  
  return pygobject_new(obj);
}

static PyMethodDef testhelper_methods[] = {
    { "get_tp_basicsize", _wrap_get_tp_basicsize, METH_VARARGS },
    { "get_test_thread", (PyCFunction)_wrap_get_test_thread, METH_NOARGS },
    { NULL, NULL }
};

void 
inittesthelper ()
{
  init_pygobject();
  g_thread_init(NULL);
  Py_InitModule ("testhelper", testhelper_methods);
}

