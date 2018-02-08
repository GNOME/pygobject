#include "regress.h"
#include "regressextra.h"

#include <glib-object.h>

struct _RegressTestBoxedCWrapper
{
  RegressTestBoxedC * cptr;
};

RegressTestBoxedCWrapper *
regress_test_boxed_c_wrapper_new (void)
{
  RegressTestBoxedCWrapper *boxed;
  boxed = g_slice_new (RegressTestBoxedCWrapper);
  boxed->cptr = regress_test_boxed_c_new ();
  return boxed;
}

RegressTestBoxedCWrapper *
regress_test_boxed_c_wrapper_copy (RegressTestBoxedCWrapper *self)
{
  RegressTestBoxedCWrapper *ret_boxed;
  ret_boxed = g_slice_new (RegressTestBoxedCWrapper);
  ret_boxed->cptr = g_boxed_copy (regress_test_boxed_c_get_type(), self->cptr);
  return ret_boxed;
}

static void
regress_test_boxed_c_wrapper_free (RegressTestBoxedCWrapper *boxed)
{
  g_boxed_free (regress_test_boxed_c_get_type(), boxed->cptr);
  g_slice_free (RegressTestBoxedCWrapper, boxed);
}

G_DEFINE_BOXED_TYPE(RegressTestBoxedCWrapper,
                    regress_test_boxed_c_wrapper,
                    regress_test_boxed_c_wrapper_copy,
                    regress_test_boxed_c_wrapper_free);

/**
 * regress_test_boxed_c_wrapper_get
 * @self: a #RegressTestBoxedCWrapper objects
 *
 * Returns: (transfer none): associated #RegressTestBoxedC
**/
RegressTestBoxedC *
regress_test_boxed_c_wrapper_get (RegressTestBoxedCWrapper *self)
{
  return self->cptr;
}
