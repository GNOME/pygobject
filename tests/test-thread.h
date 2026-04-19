#include <glib-object.h>

#define TEST_TYPE_THREAD (test_thread_get_type ())
G_DECLARE_DERIVABLE_TYPE (TestThread, test_thread, TEST, THREAD, GObject);

struct _TestThreadClass {
    GObjectClass parent_class;
    void (*emit_signal) (TestThread *sink);
    void (*from_thread) (TestThread *sink);
};
