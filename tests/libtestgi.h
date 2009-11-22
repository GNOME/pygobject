/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 */

#include <glib-object.h>

#ifndef __TEST_GI_H__
#define __TEST_GI_H__


/* Constants */

#define TESTGI_CONSTANT_NUMBER 42
#define TESTGI_CONSTANT_UTF8   "const \xe2\x99\xa5 utf8"


/* Booleans */

gboolean test_gi_boolean_return_true (void);
gboolean test_gi_boolean_return_false (void);

gboolean *test_gi_boolean_return_ptr_true (void);
gboolean *test_gi_boolean_return_ptr_false (void);

void test_gi_boolean_in_true (gboolean bool_);
void test_gi_boolean_in_false (gboolean bool_);

void test_gi_boolean_out_true (gboolean *bool_);
void test_gi_boolean_out_false (gboolean *bool_);

void test_gi_boolean_inout_true_false (gboolean *bool_);
void test_gi_boolean_inout_false_true (gboolean *bool_);

void test_gi_boolean_in_ptr_true (gboolean *bool_);
void test_gi_boolean_in_ptr_false (gboolean *bool_);


/* Integers */

gint8 test_gi_int8_return_max (void);
gint8 test_gi_int8_return_min (void);

gint8 *test_gi_int8_return_ptr_max (void);
gint8 *test_gi_int8_return_ptr_min (void);

void test_gi_int8_in_max (gint8 int8);
void test_gi_int8_in_min (gint8 int8);

void test_gi_int8_in_ptr_max (gint8 *int8);
void test_gi_int8_in_ptr_min (gint8 *int8);

void test_gi_int8_out_max (gint8 *int8);
void test_gi_int8_out_min (gint8 *int8);

void test_gi_int8_inout_max_min (gint8 *int8);
void test_gi_int8_inout_min_max (gint8 *int8);


guint8 test_gi_uint8_return (void);
guint8 *test_gi_uint8_return_ptr (void);

void test_gi_uint8_in (guint8 uint8);
void test_gi_uint8_in_ptr (guint8 *uint8);

void test_gi_uint8_out (guint8 *uint8);
void test_gi_uint8_inout (guint8 *uint8);


gint16 test_gi_int16_return_max (void);
gint16 test_gi_int16_return_min (void);

gint16 *test_gi_int16_return_ptr_max (void);
gint16 *test_gi_int16_return_ptr_min (void);

void test_gi_int16_in_max (gint16 int16);
void test_gi_int16_in_min (gint16 int16);

void test_gi_int16_in_ptr_max (gint16 *int16);
void test_gi_int16_in_ptr_min (gint16 *int16);

void test_gi_int16_out_max (gint16 *int16);
void test_gi_int16_out_min (gint16 *int16);

void test_gi_int16_inout_max_min (gint16 *int16);
void test_gi_int16_inout_min_max (gint16 *int16);


guint16 test_gi_uint16_return (void);
guint16 *test_gi_uint16_return_ptr (void);

void test_gi_uint16_in (guint16 uint16);
void test_gi_uint16_in_ptr (guint16 *uint16);

void test_gi_uint16_out (guint16 *uint16);
void test_gi_uint16_inout (guint16 *uint16);


gint32 test_gi_int32_return_max (void);
gint32 test_gi_int32_return_min (void);

gint32 *test_gi_int32_return_ptr_max (void);
gint32 *test_gi_int32_return_ptr_min (void);

void test_gi_int32_in_max (gint32 int32);
void test_gi_int32_in_min (gint32 int32);

void test_gi_int32_in_ptr_max (gint32 *int32);
void test_gi_int32_in_ptr_min (gint32 *int32);

void test_gi_int32_out_max (gint32 *int32);
void test_gi_int32_out_min (gint32 *int32);

void test_gi_int32_inout_max_min (gint32 *int32);
void test_gi_int32_inout_min_max (gint32 *int32);


guint32 test_gi_uint32_return (void);
guint32 *test_gi_uint32_return_ptr (void);

void test_gi_uint32_in (guint32 uint32);
void test_gi_uint32_in_ptr (guint32 *uint32);

void test_gi_uint32_out (guint32 *uint32);
void test_gi_uint32_inout (guint32 *uint32);


gint64 test_gi_int64_return_max (void);
gint64 test_gi_int64_return_min (void);

gint64 *test_gi_int64_return_ptr_max (void);
gint64 *test_gi_int64_return_ptr_min (void);

void test_gi_int64_in_max (gint64 int64);
void test_gi_int64_in_min (gint64 int64);

void test_gi_int64_in_ptr_max (gint64 *int64);
void test_gi_int64_in_ptr_min (gint64 *int64);

void test_gi_int64_out_max (gint64 *int64);
void test_gi_int64_out_min (gint64 *int64);

void test_gi_int64_inout_max_min (gint64 *int64);
void test_gi_int64_inout_min_max (gint64 *int64);


guint64 test_gi_uint64_return (void);
guint64 *test_gi_uint64_return_ptr (void);

void test_gi_uint64_in (guint64 uint64);
void test_gi_uint64_in_ptr (guint64 *uint64);

void test_gi_uint64_out (guint64 *uint64);
void test_gi_uint64_inout (guint64 *uint64);


gshort test_gi_short_return_max (void);
gshort test_gi_short_return_min (void);

gshort *test_gi_short_return_ptr_max (void);
gshort *test_gi_short_return_ptr_min (void);

void test_gi_short_in_max (gshort short_);
void test_gi_short_in_min (gshort short_);

void test_gi_short_in_ptr_max (gshort *short_);
void test_gi_short_in_ptr_min (gshort *short_);

void test_gi_short_out_max (gshort *short_);
void test_gi_short_out_min (gshort *short_);

void test_gi_short_inout_max_min (gshort *short_);
void test_gi_short_inout_min_max (gshort *short_);


gushort test_gi_ushort_return (void);
gushort *test_gi_ushort_return_ptr (void);

void test_gi_ushort_in (gushort ushort);
void test_gi_ushort_in_ptr (gushort *ushort);

void test_gi_ushort_out (gushort *ushort);
void test_gi_ushort_inout (gushort *ushort);


gint test_gi_int_return_max (void);
gint test_gi_int_return_min (void);

gint *test_gi_int_return_ptr_max (void);
gint *test_gi_int_return_ptr_min (void);

void test_gi_int_in_max (gint int_);
void test_gi_int_in_min (gint int_);

void test_gi_int_in_ptr_max (gint *int_);
void test_gi_int_in_ptr_min (gint *int_);

void test_gi_int_out_max (gint *int_);
void test_gi_int_out_min (gint *int_);

void test_gi_int_inout_max_min (gint *int_);
void test_gi_int_inout_min_max (gint *int_);


guint test_gi_uint_return (void);
guint *test_gi_uint_return_ptr (void);

void test_gi_uint_in (guint uint);
void test_gi_uint_in_ptr (guint *uint);

void test_gi_uint_out (guint *uint);
void test_gi_uint_inout (guint *uint);


glong test_gi_long_return_max (void);
glong test_gi_long_return_min (void);

glong *test_gi_long_return_ptr_max (void);
glong *test_gi_long_return_ptr_min (void);

void test_gi_long_in_max (glong long_);
void test_gi_long_in_min (glong long_);

void test_gi_long_in_ptr_max (glong *long_);
void test_gi_long_in_ptr_min (glong *long_);

void test_gi_long_out_max (glong *long_);
void test_gi_long_out_min (glong *long_);

void test_gi_long_inout_max_min (glong *long_);
void test_gi_long_inout_min_max (glong *long_);


gulong test_gi_ulong_return (void);
gulong *test_gi_ulong_return_ptr (void);

void test_gi_ulong_in (gulong ulong);
void test_gi_ulong_in_ptr (gulong *ulong);

void test_gi_ulong_out (gulong *ulong);
void test_gi_ulong_inout (gulong *ulong);


gssize test_gi_ssize_return_max (void);
gssize test_gi_ssize_return_min (void);

gssize *test_gi_ssize_return_ptr_max (void);
gssize *test_gi_ssize_return_ptr_min (void);

void test_gi_ssize_in_max (gssize ssize);
void test_gi_ssize_in_min (gssize ssize);

void test_gi_ssize_in_ptr_max (gssize *ssize);
void test_gi_ssize_in_ptr_min (gssize *ssize);

void test_gi_ssize_out_max (gssize *ssize);
void test_gi_ssize_out_min (gssize *ssize);

void test_gi_ssize_inout_max_min (gssize *ssize);
void test_gi_ssize_inout_min_max (gssize *ssize);


gsize test_gi_size_return (void);
gsize *test_gi_size_return_ptr (void);

void test_gi_size_in (gsize size);
void test_gi_size_in_ptr (gsize *size);

void test_gi_size_out (gsize *size);
void test_gi_size_inout (gsize *size);


/* Floating-point */

gfloat test_gi_float_return (void);
gfloat *test_gi_float_return_ptr (void);

void test_gi_float_in (gfloat float_);
void test_gi_float_in_ptr (gfloat *float_);

void test_gi_float_out (gfloat *float_);

void test_gi_float_inout (gfloat *float_);


gdouble test_gi_double_return (void);
gdouble *test_gi_double_return_ptr (void);

void test_gi_double_in (gdouble double_);
void test_gi_double_in_ptr (gdouble *double_);

void test_gi_double_out (gdouble *double_);

void test_gi_double_inout (gdouble *double_);


/* Timestamps */

time_t test_gi_time_t_return (void);
time_t *test_gi_time_t_return_ptr (void);

void test_gi_time_t_in (time_t time_t_);
void test_gi_time_t_in_ptr (time_t *time_t_);

void test_gi_time_t_out (time_t *time_t_);

void test_gi_time_t_inout (time_t *time_t_);


/* GType */

GType test_gi_gtype_return (void);
GType *test_gi_gtype_return_ptr (void);

void test_gi_gtype_in (GType gtype);
void test_gi_gtype_in_ptr (GType *gtype);

void test_gi_gtype_out (GType *gtype);

void test_gi_gtype_inout (GType *gtype);


/* UTF-8 */

const gchar *test_gi_utf8_none_return (void);
gchar *test_gi_utf8_full_return (void);

void test_gi_utf8_none_in (const gchar *utf8);
void test_gi_utf8_full_in (gchar *utf8);

void test_gi_utf8_none_out (gchar **utf8);
void test_gi_utf8_full_out (gchar **utf8);

void test_gi_utf8_none_inout (gchar **utf8);
void test_gi_utf8_full_inout (gchar **utf8);


/* Arrays */

/* Fixed-size */
const gint *test_gi_array_fixed_int_return (void);
const gshort *test_gi_array_fixed_short_return (void);

void test_gi_array_fixed_int_in (const gint *ints);
void test_gi_array_fixed_short_in (const gshort *shorts);

void test_gi_array_fixed_out (gint **ints);

void test_gi_array_fixed_inout (gint **ints);

/* Variable-size */

const gint *test_gi_array_return (gint *length);

void test_gi_array_in (const gint *ints, gint length);

void test_gi_array_out (gint **ints, gint *length);

void test_gi_array_inout (gint **ints, gint *length);

/* Zero-terminated */

gchar **test_gi_array_zero_terminated_return (void);

void test_gi_array_zero_terminated_in (gchar **utf8s);

void test_gi_array_zero_terminated_out (gchar ***utf8s);

void test_gi_array_zero_terminated_inout (gchar ***utf8s);


/* GList */

GList *test_gi_glist_int_none_return (void);
GList *test_gi_glist_utf8_none_return (void);
GList *test_gi_glist_utf8_container_return (void);
GList *test_gi_glist_utf8_full_return (void);

void test_gi_glist_int_none_in (GList *list);
void test_gi_glist_utf8_none_in (GList *list);
void test_gi_glist_utf8_container_in (GList *list);
void test_gi_glist_utf8_full_in (GList *list);

void test_gi_glist_utf8_none_out (GList **list);
void test_gi_glist_utf8_container_out (GList **list);
void test_gi_glist_utf8_full_out (GList **list);

void test_gi_glist_utf8_none_inout (GList **list);
void test_gi_glist_utf8_container_inout (GList **list);
void test_gi_glist_utf8_full_inout (GList **list);


/* GSList */

GSList *test_gi_gslist_int_none_return (void);
GSList *test_gi_gslist_utf8_none_return (void);
GSList *test_gi_gslist_utf8_container_return (void);
GSList *test_gi_gslist_utf8_full_return (void);

void test_gi_gslist_int_none_in (GSList *list);
void test_gi_gslist_utf8_none_in (GSList *list);
void test_gi_gslist_utf8_container_in (GSList *list);
void test_gi_gslist_utf8_full_in (GSList *list);

void test_gi_gslist_utf8_none_out (GSList **list);
void test_gi_gslist_utf8_container_out (GSList **list);
void test_gi_gslist_utf8_full_out (GSList **list);

void test_gi_gslist_utf8_none_inout (GSList **list);
void test_gi_gslist_utf8_container_inout (GSList **list);
void test_gi_gslist_utf8_full_inout (GSList **list);


/* GHashTable */

GHashTable *test_gi_ghashtable_int_none_return (void);
GHashTable *test_gi_ghashtable_utf8_none_return (void);
GHashTable *test_gi_ghashtable_utf8_container_return (void);
GHashTable *test_gi_ghashtable_utf8_full_return (void);

void test_gi_ghashtable_int_none_in (GHashTable *hash_table);
void test_gi_ghashtable_utf8_none_in (GHashTable *hash_table);
void test_gi_ghashtable_utf8_container_in (GHashTable *hash_table);
void test_gi_ghashtable_utf8_full_in (GHashTable *hash_table);

void test_gi_ghashtable_utf8_none_out (GHashTable **hash_table);
void test_gi_ghashtable_utf8_container_out (GHashTable **hash_table);
void test_gi_ghashtable_utf8_full_out (GHashTable **hash_table);

void test_gi_ghashtable_utf8_none_inout (GHashTable **hash_table);
void test_gi_ghashtable_utf8_container_inout (GHashTable **hash_table);
void test_gi_ghashtable_utf8_full_inout (GHashTable **hash_table);


/* GValue */

GValue *test_gi_gvalue_return (void);

void test_gi_gvalue_in (GValue *value);

void test_gi_gvalue_out (GValue **value);

void test_gi_gvalue_inout (GValue **value);


/* GClosure */

void test_gi_gclosure_in (GClosure *closure);


/* Pointer */

gpointer test_gi_pointer_in_return (gpointer pointer);


/* GEnum */

typedef enum
{
  TESTGI_ENUM_VALUE1,
  TESTGI_ENUM_VALUE2,
  TESTGI_ENUM_VALUE3 = 42
} TestGIEnum;

GType test_gi_enum_get_type (void) G_GNUC_CONST;
#define TESTGI_TYPE_ENUM (test_gi_enum_get_type ())

TestGIEnum test_gi_enum_return (void);

void test_gi_enum_in (TestGIEnum enum_);
void test_gi_enum_in_ptr (TestGIEnum *enum_);

void test_gi_enum_out (TestGIEnum *enum_);

void test_gi_enum_inout (TestGIEnum *enum_);


/* GFlags */

typedef enum
{
  TESTGI_FLAGS_VALUE1 = 1 << 0,
  TESTGI_FLAGS_VALUE2 = 1 << 1,
  TESTGI_FLAGS_VALUE3 = 1 << 2
} TestGIFlags;

GType test_gi_flags_get_type (void) G_GNUC_CONST;
#define TESTGI_TYPE_FLAGS (test_gi_flags_get_type ())

TestGIFlags test_gi_flags_return (void);

void test_gi_flags_in (TestGIFlags flags_);
void test_gi_flags_in_ptr (TestGIFlags *flags_);

void test_gi_flags_out (TestGIFlags *flags_);

void test_gi_flags_inout (TestGIFlags *flags_);


/* Structure */

typedef struct {
    glong long_;
    gint8 int8;
} TestGISimpleStruct;

typedef struct {
    TestGISimpleStruct simple_struct;
} TestGINestedStruct;

typedef struct {
    gpointer pointer;
} TestGINotSimpleStruct;


TestGISimpleStruct *test_gi__simple_struct_return (void);

void test_gi__simple_struct_in (TestGISimpleStruct *struct_);

void test_gi__simple_struct_out (TestGISimpleStruct **struct_);

void test_gi__simple_struct_inout (TestGISimpleStruct **struct_);

void test_gi_simple_struct_method (TestGISimpleStruct *struct_);


typedef struct {
    glong long_;
} TestGIPointerStruct;

GType test_gi_pointer_struct_get_type (void) G_GNUC_CONST;

TestGIPointerStruct *test_gi__pointer_struct_return (void);

void test_gi__pointer_struct_in (TestGIPointerStruct *struct_);

void test_gi__pointer_struct_out (TestGIPointerStruct **struct_);

void test_gi__pointer_struct_inout (TestGIPointerStruct **struct_);


typedef struct {
    glong long_;
} TestGIBoxedStruct;

GType test_gi_boxed_struct_get_type (void) G_GNUC_CONST;


typedef struct {
    glong long_;
} TestGIBoxedInstantiableStruct;

GType test_gi_boxed_instantiable_struct_get_type (void) G_GNUC_CONST;

TestGIBoxedInstantiableStruct *test_gi_boxed_instantiable_struct_new (void);

TestGIBoxedInstantiableStruct *test_gi__boxed_instantiable_struct_return (void);

void test_gi__boxed_instantiable_struct_in (TestGIBoxedInstantiableStruct *struct_);

void test_gi__boxed_instantiable_struct_out (TestGIBoxedInstantiableStruct **struct_);

void test_gi__boxed_instantiable_struct_inout (TestGIBoxedInstantiableStruct **struct_);


/* Object */

#define TESTGI_TYPE_OBJECT             (test_gi_object_get_type ())
#define TESTGI_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TESTGI_TYPE_OBJECT, TestGIObject))
#define TESTGI_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TESTGI_TYPE_OBJECT, TestGIObjectClass))
#define TESTGI_IS_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TESTGI_TYPE_OBJECT))
#define TESTGI_IS_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TESTGI_TYPE_OBJECT))
#define TESTGI_OBJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TESTGI_TYPE_OBJECT, TestGIObjectClass))

typedef struct _TestGIObjectClass TestGIObjectClass;
typedef struct _TestGIObject TestGIObject;

struct _TestGIObjectClass
{
	GObjectClass parent_class;
};

struct _TestGIObject
{
	GObject parent_instance;

    gint int_;
};

GType test_gi_object_get_type (void) G_GNUC_CONST;
void test_gi_object_static_method (void);
void test_gi_object_method (TestGIObject *object);
void test_gi_object_overridden_method (TestGIObject *object);
TestGIObject *test_gi_object_new (gint int_);


TestGIObject *test_gi__object_none_return (void);
TestGIObject *test_gi__object_full_return (void);

void test_gi__object_none_in (TestGIObject *object);
void test_gi__object_full_in (TestGIObject *object);

void test_gi__object_none_out (TestGIObject **object);
void test_gi__object_full_out (TestGIObject **object);

void test_gi__object_none_inout (TestGIObject **object);
void test_gi__object_full_inout (TestGIObject **object);
void test_gi__object_inout_same (TestGIObject **object);


#define TESTGI_TYPE_SUB_OBJECT             (test_gi_sub_object_get_type ())
#define TESTGI_SUB_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TESTGI_TYPE_SUB_OBJECT, TestGISubObject))
#define TESTGI_SUB_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TESTGI_TYPE_SUB_OBJECT, TestGISubObjectClass))
#define TESTGI_IS_SUB_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TESTGI_TYPE_SUB_OBJECT))
#define TESTGI_IS_SUB_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TESTGI_TYPE_SUB_OBJECT))
#define TESTGI_SUB_OBJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TESTGI_TYPE_SUB_OBJECT, TestGISubObjectClass))

typedef struct _TestGISubObjectClass TestGISubObjectClass;
typedef struct _TestGISubObject TestGISubObject;

struct _TestGISubObjectClass
{
	TestGIObjectClass parent_class;
};

struct _TestGISubObject
{
	TestGIObject parent_instance;
};

GType test_gi_sub_object_get_type (void) G_GNUC_CONST;

void test_gi_sub_object_sub_method (TestGISubObject *object);
void test_gi_sub_object_overwritten_method (TestGISubObject *object);


/* Multiple output arguments */

void test_gi_int_out_out (gint *int0, gint *int1);
gint test_gi_int_return_out (gint *int_);


/* Nullable arguments */

gint *test_gi_int_return_ptr_null (void);
void test_gi_int_in_ptr_null (gint *int_);


#endif /* __TEST_GI_H__ */
