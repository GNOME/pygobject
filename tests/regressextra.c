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

/**
 * regress_test_array_fixed_boxed_none_out
 * @objs: (out) (array fixed-size=2) (transfer none): An array of #RegressTestBoxedC
**/
void
regress_test_array_fixed_boxed_none_out (RegressTestBoxedC ***objs)
{
  static RegressTestBoxedC **arr;

  if (arr == NULL) {
    arr = g_new0 (RegressTestBoxedC *, 3);
    arr[0] = regress_test_boxed_c_new ();
    arr[1] = regress_test_boxed_c_new ();
  }

  *objs = arr;
}

/**
 * regress_test_glist_boxed_none_return
 * Return value: (element-type RegressTestBoxedC) (transfer none):
**/
GList *
regress_test_glist_boxed_none_return (guint count)
{
    static GList *list = NULL;
    if (!list) {
        while (count > 0) {
            list = g_list_prepend (list, regress_test_boxed_c_new ());
            count--;
        }
    }

    return list;
}

/**
 * regress_test_glist_boxed_full_return
 * Return value: (element-type RegressTestBoxedC) (transfer full):
**/
GList *
regress_test_glist_boxed_full_return (guint count)
{
    GList *list = NULL;
    while (count > 0) {
        list = g_list_prepend (list, regress_test_boxed_c_new ());
        count--;
    }
    return list;
}


#ifndef _GI_DISABLE_CAIRO

/**
 * regress_test_cairo_context_none_return:
 *
 * Returns: (transfer none):
 */
cairo_t *
regress_test_cairo_context_none_return (void)
{
    static cairo_t *cr;

    if (cr == NULL) {
        cairo_surface_t *surface;
        surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
        cr = cairo_create (surface);
        cairo_surface_destroy (surface);
    }

    return cr;
}

/**
 * regress_test_cairo_context_full_in:
 * @context: (transfer full):
 */
void
regress_test_cairo_context_full_in (cairo_t *context)
{
    cairo_destroy (context);
}


/**
 * regress_test_cairo_path_full_return:
 *
 * Returns: (transfer full):
 */
cairo_path_t *
regress_test_cairo_path_full_return (void)
{
    cairo_t *cr = regress_test_cairo_context_none_return ();

    return cairo_copy_path (cr);
}

/**
 * regress_test_cairo_path_none_in:
 * @path: (transfer none):
 */
void
regress_test_cairo_path_none_in (cairo_path_t *path)
{
    cairo_t *cr = regress_test_cairo_context_full_return ();
    cairo_append_path (cr, path);
    g_assert (cairo_status (cr) == CAIRO_STATUS_SUCCESS);
    cairo_destroy (cr);
}

/**
 * regress_test_cairo_path_full_in_full_return:
 * @path: (transfer full):
 *
 * Returns: (transfer full):
 */
cairo_path_t *
regress_test_cairo_path_full_in_full_return (cairo_path_t *path)
{
    return path;
}

/**
 * regress_test_cairo_region_full_in:
 * @region: (transfer full):
 */
void
regress_test_cairo_region_full_in (cairo_region_t *region)
{
    cairo_region_destroy (region);
}

/**
 * regress_test_cairo_surface_full_in:
 * @surface: (transfer full):
 */
void
regress_test_cairo_surface_full_in (cairo_surface_t *surface)
{
    g_assert (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32);
    g_assert (cairo_image_surface_get_width (surface) == 10);
    g_assert (cairo_image_surface_get_height (surface) == 10);
    cairo_surface_destroy (surface);
}

/**
 * regress_test_cairo_font_options_full_return:
 *
 * Returns: (transfer full):
 */
cairo_font_options_t *
regress_test_cairo_font_options_full_return (void)
{
    return cairo_font_options_create ();
}

/**
 * regress_test_cairo_font_options_none_return:
 *
 * Returns: (transfer none):
 */
cairo_font_options_t *
regress_test_cairo_font_options_none_return (void)
{
    static cairo_font_options_t *options;

    if (options == NULL)
        options = cairo_font_options_create ();

    return options;
}

/**
 * regress_test_cairo_font_options_full_in:
 * @options: (transfer full):
 */
void
regress_test_cairo_font_options_full_in (cairo_font_options_t *options)
{
    cairo_font_options_destroy (options);
}

/**
 * regress_test_cairo_font_options_none_in:
 * @options: (transfer none):
 */
void
regress_test_cairo_font_options_none_in (cairo_font_options_t *options)
{
}


/**
 * regress_test_cairo_matrix_none_in:
 * @matrix: (transfer none):
 */
void
regress_test_cairo_matrix_none_in (const cairo_matrix_t *matrix)
{
    cairo_matrix_t m = *matrix;
    g_assert (m.x0 == 0);
    g_assert (m.y0 == 0);
    g_assert (m.xx == 1);
    g_assert (m.xy == 0);
    g_assert (m.yy == 1);
    g_assert (m.yx == 0);
}

/**
 * regress_test_cairo_matrix_none_return:
 * Returns: (transfer none):
 */
cairo_matrix_t *
regress_test_cairo_matrix_none_return (void)
{
    static cairo_matrix_t matrix;
    cairo_matrix_init_identity (&matrix);
    return &matrix;
}

/**
 * regress_test_cairo_matrix_out_caller_allocates:
 * @matrix: (out):
 */
void
regress_test_cairo_matrix_out_caller_allocates (cairo_matrix_t *matrix)
{
    cairo_matrix_t m;
    cairo_matrix_init_identity (&m);
    *matrix = m;
}

#endif
