#include <glib-object.h>

/* TestUnknown */

#define TEST_TYPE_UNKNOWN (test_unknown_get_type ())
G_DECLARE_DERIVABLE_TYPE (TestUnknown, test_unknown, TEST, UNKNOWN, GObject);

struct _TestUnknownClass {
    GObjectClass parent_class;
};


/* TestInterface */
#define TEST_TYPE_INTERFACE (test_interface_get_type ())
G_DECLARE_INTERFACE (TestInterface, test_interface, TEST, INTERFACE, GObject);

struct _TestInterfaceInterface {
    GTypeInterface g_iface;
    /* VTable */
    void (*iface_method) (TestInterface *iface);
};

void test_interface_iface_method (TestInterface *iface);
