#include "test-unknown.h"

G_DEFINE_TYPE_WITH_CODE (TestUnknown, test_unknown, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (TEST_TYPE_INTERFACE, NULL));

static void test_unknown_init (TestUnknown *self) {}
static void test_unknown_class_init (TestUnknownClass *klass) {}

GType
test_interface_get_type (void)
{
  static GType gtype = 0;

  if (!gtype)
    {
      static const GTypeInfo info =
      {
        sizeof (TestInterface), /* class_size */
	NULL,   /* base_init */
        NULL,           /* base_finalize */
        NULL,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        0,
        0,              /* n_preallocs */
        NULL
      };

      gtype =
        g_type_register_static (G_TYPE_INTERFACE, "TestInterface",
                                &info, 0);

      g_type_interface_add_prerequisite (gtype, G_TYPE_OBJECT);
    }

  return gtype;
}
