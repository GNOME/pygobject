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
 * regress_test_array_of_non_utf8_strings
 * Returns: (transfer full) (allow-none) (array zero-terminated=1): Array of strings
 */
gchar**
regress_test_array_of_non_utf8_strings (void)
{
    char **ret = g_new (char *, 2);
    ret[0] = g_strdup ("Andr\351 Lur\347at");
    ret[1] = NULL;
    return ret;
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
 * regress_test_gvalue_out_boxed:
 * @value: (out) (transfer full): the output gvalue
 * @init: (in): the initialisation value
**/
void
regress_test_gvalue_out_boxed (GValue *value, int init)
{
  RegressTestBoxed rtb;
  GValue v = G_VALUE_INIT;

  memset(&rtb, 0, sizeof (rtb));
  rtb.some_int8 = init;
  g_value_init (&v, REGRESS_TEST_TYPE_BOXED);
  g_value_set_boxed (&v, &rtb);
  *value = v;
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

/**
 * regress_test_array_of_fundamental_objects_in
 * @list: (array length=len) (element-type RegressTestFundamentalObject): An array of #RegressTestFundamentalObject
 * @len: length of the list
 **/
gboolean
regress_test_array_of_fundamental_objects_in (RegressTestFundamentalObject **list, gsize len)
{
    gsize i;
  
    for (i = 0; i < len; i++) {
        if (!REGRESS_TEST_IS_FUNDAMENTAL_OBJECT (list[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * regress_test_array_of_fundamental_objects_out
 * @len: (out): length of the list
 * Returns: (array length=len) (transfer full): An array of #RegressTestFundamentalObject
 **/
RegressTestFundamentalObject **
regress_test_array_of_fundamental_objects_out (gsize *len)
{
    RegressTestFundamentalObject **objs;
    int i;

    objs = g_new (RegressTestFundamentalObject *, 2);
    
    for (i = 0; i < 2; i++) {
        objs[i] = (RegressTestFundamentalObject *) regress_test_fundamental_sub_object_new("foo");
    }
    *len = 2;
    return objs;
}

/**
 * regress_test_fundamental_argument_in
 * @obj: (transfer full): A #RegressTestFundamentalObject
 **/
gboolean
regress_test_fundamental_argument_in (RegressTestFundamentalObject *obj)
{
    return REGRESS_TEST_IS_FUNDAMENTAL_OBJECT (obj);
}

/**
 * regress_test_fundamental_argument_out
 * @obj: (transfer none): A #RegressTestFundamentalObject
 * Returns: (transfer none): Same #RegressTestFundamentalObject
 **/
RegressTestFundamentalObject*
regress_test_fundamental_argument_out (RegressTestFundamentalObject *obj)
{
    return obj;
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
 * regress_test_cairo_pattern_full_in:
 * @pattern: (transfer full):
 */
void
regress_test_cairo_pattern_full_in (cairo_pattern_t *pattern)
{
    cairo_pattern_destroy (pattern);
}

/**
 * regress_test_cairo_pattern_none_in:
 * @pattern: (transfer none):
 */
void
regress_test_cairo_pattern_none_in (cairo_pattern_t *pattern)
{
    cairo_t *cr = regress_test_cairo_context_full_return ();
    cairo_set_source (cr, pattern);
    g_assert (cairo_status (cr) == CAIRO_STATUS_SUCCESS);
    cairo_destroy (cr);
}

/**
 * regress_test_cairo_pattern_none_return:
 *
 * Returns: (transfer none):
 */
cairo_pattern_t*
regress_test_cairo_pattern_none_return (void)
{
    static cairo_pattern_t *pattern;

    if (pattern == NULL) {
        pattern = cairo_pattern_create_rgb(0.1, 0.2, 0.3);
    }

    return pattern;
}

/**
 * regress_test_cairo_pattern_full_return:
 *
 * Returns: (transfer full):
 */
cairo_pattern_t *
regress_test_cairo_pattern_full_return (void)
{
    cairo_pattern_t *pattern = cairo_pattern_create_rgb(0.5, 0.6, 0.7);
    return pattern;
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

G_DEFINE_TYPE (RegressTestAction, regress_test_action, G_TYPE_INITIALLY_UNOWNED)

enum
{
    SIGNAL_0,
    ACTION_SIGNAL,
    ACTION2_SIGNAL,
    LAST_SIGNAL
};

static guint regress_test_action_signals[LAST_SIGNAL] = { 0 };

static RegressTestAction *
regress_test_action_do_action (RegressTestAction *self)
{
    RegressTestAction *ret = g_object_new (regress_test_action_get_type (), NULL);

    return ret;
}

static RegressTestAction *
regress_test_action_do_action2 (RegressTestAction *self)
{
    return NULL;
}

static void
regress_test_action_init (RegressTestAction *self)
{
}

static void regress_test_action_class_init (RegressTestActionClass *klass)
{
    /**
     * RegressTestAction::action:
     *
     * An action signal.
     *
     * Returns: (transfer full): another #RegressTestAction
     */
    regress_test_action_signals[ACTION_SIGNAL] =
        g_signal_new_class_handler ("action",
        G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
        G_CALLBACK (regress_test_action_do_action), NULL, NULL,
        NULL, regress_test_action_get_type (), 0);

    /**
     * RegressTestAction::action2:
     *
     * Another action signal.
     *
     * Returns: (transfer full): another #RegressTestAction
     */
    regress_test_action_signals[ACTION2_SIGNAL] =
        g_signal_new_class_handler ("action2",
        G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
        G_CALLBACK (regress_test_action_do_action2), NULL, NULL,
        NULL, regress_test_action_get_type (), 0);
}
