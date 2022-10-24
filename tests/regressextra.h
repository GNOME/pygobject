#ifndef REGRESS_EXTRA_H
#define REGRESS_EXTRA_H

#include <glib-object.h>

typedef struct _RegressTestBoxedC RegressTestBoxedC;
typedef struct _RegressTestBoxedCWrapper RegressTestBoxedCWrapper;

_GI_TEST_EXTERN
GType regress_test_boxed_c_wrapper_get_type (void);

_GI_TEST_EXTERN
RegressTestBoxedCWrapper *regress_test_boxed_c_wrapper_new (void);
_GI_TEST_EXTERN
RegressTestBoxedCWrapper * regress_test_boxed_c_wrapper_copy (RegressTestBoxedCWrapper *self);
_GI_TEST_EXTERN
RegressTestBoxedC *regress_test_boxed_c_wrapper_get (RegressTestBoxedCWrapper *self);

_GI_TEST_EXTERN
void regress_test_array_fixed_boxed_none_out (RegressTestBoxedC ***objs);
_GI_TEST_EXTERN
void regress_test_gvalue_out_boxed (GValue *value, int init);
_GI_TEST_EXTERN
GList *regress_test_glist_boxed_none_return (guint count);
_GI_TEST_EXTERN
GList *regress_test_glist_boxed_full_return (guint count);

#ifndef _GI_DISABLE_CAIRO

_GI_TEST_EXTERN
cairo_t *regress_test_cairo_context_none_return (void);
_GI_TEST_EXTERN
void regress_test_cairo_context_full_in (cairo_t *context);
_GI_TEST_EXTERN
cairo_path_t *regress_test_cairo_path_full_return (void);
_GI_TEST_EXTERN
void regress_test_cairo_path_none_in (cairo_path_t *path);
_GI_TEST_EXTERN
cairo_path_t * regress_test_cairo_path_full_in_full_return (cairo_path_t *path);
_GI_TEST_EXTERN
void regress_test_cairo_pattern_full_in (cairo_pattern_t *pattern);
_GI_TEST_EXTERN
void regress_test_cairo_pattern_none_in (cairo_pattern_t *pattern);
_GI_TEST_EXTERN
cairo_pattern_t* regress_test_cairo_pattern_none_return (void);
_GI_TEST_EXTERN
cairo_pattern_t * regress_test_cairo_pattern_full_return (void);
_GI_TEST_EXTERN
cairo_font_options_t *regress_test_cairo_font_options_full_return (void);
_GI_TEST_EXTERN
cairo_font_options_t *regress_test_cairo_font_options_none_return (void);
_GI_TEST_EXTERN
void regress_test_cairo_font_options_full_in (cairo_font_options_t *options);
_GI_TEST_EXTERN
void regress_test_cairo_font_options_none_in (cairo_font_options_t *options);
_GI_TEST_EXTERN
void regress_test_cairo_region_full_in (cairo_region_t *region);
_GI_TEST_EXTERN
void regress_test_cairo_surface_full_in (cairo_surface_t *surface);
_GI_TEST_EXTERN
void regress_test_cairo_matrix_none_in (const cairo_matrix_t *matrix);
_GI_TEST_EXTERN
cairo_matrix_t *regress_test_cairo_matrix_none_return (void);
_GI_TEST_EXTERN
void regress_test_cairo_matrix_out_caller_allocates (cairo_matrix_t *matrix);

#endif

/* RegressTestAction */

typedef struct {
  GInitiallyUnowned parent;
} RegressTestAction;

typedef struct {
  GInitiallyUnownedClass parent_class;
} RegressTestActionClass;

_GI_TEST_EXTERN
GType regress_test_action_get_type (void);

#endif /* REGRESS_EXTRA_H */
