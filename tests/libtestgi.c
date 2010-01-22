/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 */

#include "libtestgi.h"

#include <string.h>


/* Booleans */

gboolean
test_gi_boolean_return_true (void)
{
    return TRUE;
}

gboolean
test_gi_boolean_return_false (void)
{
    return FALSE;
}

/**
 * test_gi_boolean_return_ptr_true:
 * Returns: (transfer none):
 */
gboolean *
test_gi_boolean_return_ptr_true (void)
{
    static gboolean bool_ = TRUE;
    return &bool_;
}

/**
 * test_gi_boolean_return_ptr_false:
 * Returns: (transfer none):
 */
gboolean *
test_gi_boolean_return_ptr_false (void)
{
    static gboolean bool_ = FALSE;
    return &bool_;
}

void
test_gi_boolean_in_true (gboolean bool_)
{
    g_assert (bool_ == TRUE);
}

void
test_gi_boolean_in_false (gboolean bool_)
{
    g_assert (bool_ == FALSE);
}

/**
 * test_gi_boolean_in_ptr_true:
 * bool_: (in):
 */
void
test_gi_boolean_in_ptr_true (gboolean *bool_)
{
    g_assert (*bool_ == TRUE);
}

/**
 * test_gi_boolean_in_ptr_false:
 * bool_: (in):
 */
void
test_gi_boolean_in_ptr_false (gboolean *bool_)
{
    g_assert (*bool_ == FALSE);
}

/**
 * test_gi_boolean_out_true:
 * bool_: (out):
 */
void
test_gi_boolean_out_true (gboolean *bool_)
{
    *bool_ = TRUE;
}

/**
 * test_gi_boolean_out_false:
 * bool_: (out):
 */
void
test_gi_boolean_out_false (gboolean *bool_)
{
    *bool_ = FALSE;
}

/**
 * test_gi_boolean_inout_true_false:
 * bool_: (inout):
 */
void
test_gi_boolean_inout_true_false (gboolean *bool_)
{
    g_assert (*bool_ == TRUE);
    *bool_ = FALSE;
}

/**
 * test_gi_boolean_inout_false_true:
 * bool_: (inout):
 */
void
test_gi_boolean_inout_false_true (gboolean *bool_)
{
    g_assert (*bool_ == FALSE);
    *bool_ = TRUE;
}


/* Integers */

gint8
test_gi_int8_return_max (void)
{
    return G_MAXINT8;
}

gint8
test_gi_int8_return_min (void)
{
    return G_MININT8;
}

/**
 * test_gi_int8_return_ptr_max:
 * Returns: (transfer none):
 */
gint8 *
test_gi_int8_return_ptr_max (void)
{
    static gint8 int8 = G_MAXINT8;
    return &int8;
}

/**
 * test_gi_int8_return_ptr_min:
 * Returns: (transfer none):
 */
gint8 *
test_gi_int8_return_ptr_min (void)
{
    static gint8 int8 = G_MININT8;
    return &int8;
}

void
test_gi_int8_in_max (gint8 int8)
{
    g_assert(int8 == G_MAXINT8);
}

void
test_gi_int8_in_min (gint8 int8)
{
    g_assert(int8 == G_MININT8);
}

/**
 * test_gi_int8_in_ptr_max:
 * int8: (in):
 */
void
test_gi_int8_in_ptr_max (gint8 *int8)
{
    g_assert(*int8 == G_MAXINT8);
}

/**
 * test_gi_int8_in_ptr_min:
 * int8: (in):
 */
void
test_gi_int8_in_ptr_min (gint8 *int8)
{
    g_assert(*int8 == G_MININT8);
}

/**
 * test_gi_int8_out_max:
 * int8: (out):
 */
void
test_gi_int8_out_max (gint8 *int8)
{
    *int8 = G_MAXINT8;
}

/**
 * test_gi_int8_out_min:
 * int8: (out):
 */
void
test_gi_int8_out_min (gint8 *int8)
{
    *int8 = G_MININT8;
}

/**
 * test_gi_int8_inout_max_min:
 * int8: (inout):
 */
void
test_gi_int8_inout_max_min (gint8 *int8)
{
    g_assert(*int8 == G_MAXINT8);
    *int8 = G_MININT8;
}

/**
 * test_gi_int8_inout_min_max:
 * int8: (inout):
 */
void
test_gi_int8_inout_min_max (gint8 *int8)
{
    g_assert(*int8 == G_MININT8);
    *int8 = G_MAXINT8;
}


guint8
test_gi_uint8_return (void)
{
    return G_MAXUINT8;
}

/**
 * test_gi_uint8_return_ptr:
 * Returns: (transfer none):
 */
guint8 *
test_gi_uint8_return_ptr (void)
{
    static guint8 uint8 = G_MAXUINT8;
    return &uint8;
}

void
test_gi_uint8_in (guint8 uint8)
{
    g_assert(uint8 == G_MAXUINT8);
}

/**
 * test_gi_uint8_in_ptr:
 * uint8: (in):
 */
void
test_gi_uint8_in_ptr (guint8 *uint8)
{
    g_assert(*uint8 == G_MAXUINT8);
}

/**
 * test_gi_uint8_out:
 * uint8: (out):
 */
void
test_gi_uint8_out (guint8 *uint8)
{
    *uint8 = G_MAXUINT8;
}

/**
 * test_gi_uint8_inout:
 * uint8: (inout):
 */
void
test_gi_uint8_inout (guint8 *uint8)
{
    g_assert(*uint8 == G_MAXUINT8);
    *uint8 = 0;
}


gint16
test_gi_int16_return_max (void)
{
    return G_MAXINT16;
}

gint16
test_gi_int16_return_min (void)
{
    return G_MININT16;
}

/**
 * test_gi_int16_return_ptr_max:
 * Returns: (transfer none):
 */
gint16 *
test_gi_int16_return_ptr_max (void)
{
    static gint16 int16 = G_MAXINT16;
    return &int16;
}

/**
 * test_gi_int16_return_ptr_min:
 * Returns: (transfer none):
 */
gint16 *
test_gi_int16_return_ptr_min (void)
{
    static gint16 int16 = G_MININT16;
    return &int16;
}

void
test_gi_int16_in_max (gint16 int16)
{
    g_assert(int16 == G_MAXINT16);
}

void
test_gi_int16_in_min (gint16 int16)
{
    g_assert(int16 == G_MININT16);
}

/**
 * test_gi_int16_in_ptr_max:
 * int16: (in):
 */
void
test_gi_int16_in_ptr_max (gint16 *int16)
{
    g_assert(*int16 == G_MAXINT16);
}

/**
 * test_gi_int16_in_ptr_min:
 * int16: (in):
 */
void
test_gi_int16_in_ptr_min (gint16 *int16)
{
    g_assert(*int16 == G_MININT16);
}

/**
 * test_gi_int16_out_max:
 * int16: (out):
 */
void
test_gi_int16_out_max (gint16 *int16)
{
    *int16 = G_MAXINT16;
}

/**
 * test_gi_int16_out_min:
 * int16: (out):
 */
void
test_gi_int16_out_min (gint16 *int16)
{
    *int16 = G_MININT16;
}

/**
 * test_gi_int16_inout_max_min:
 * int16: (inout):
 */
void
test_gi_int16_inout_max_min (gint16 *int16)
{
    g_assert(*int16 == G_MAXINT16);
    *int16 = G_MININT16;
}

/**
 * test_gi_int16_inout_min_max:
 * int16: (inout):
 */
void
test_gi_int16_inout_min_max (gint16 *int16)
{
    g_assert(*int16 == G_MININT16);
    *int16 = G_MAXINT16;
}


guint16
test_gi_uint16_return (void)
{
    return G_MAXUINT16;
}

/**
 * test_gi_uint16_return_ptr:
 * Returns: (transfer none):
 */
guint16 *
test_gi_uint16_return_ptr (void)
{
    static guint16 uint16 = G_MAXUINT16;
    return &uint16;
}

void
test_gi_uint16_in (guint16 uint16)
{
    g_assert(uint16 == G_MAXUINT16);
}

/**
 * test_gi_uint16_in_ptr:
 * uint16: (in):
 */
void
test_gi_uint16_in_ptr (guint16 *uint16)
{
    g_assert(*uint16 == G_MAXUINT16);
}

/**
 * test_gi_uint16_out:
 * uint16: (out):
 */
void
test_gi_uint16_out (guint16 *uint16)
{
    *uint16 = G_MAXUINT16;
}

/**
 * test_gi_uint16_inout:
 * uint16: (inout):
 */
void
test_gi_uint16_inout (guint16 *uint16)
{
    g_assert(*uint16 == G_MAXUINT16);
    *uint16 = 0;
}


gint32
test_gi_int32_return_max (void)
{
    return G_MAXINT32;
}

gint32
test_gi_int32_return_min (void)
{
    return G_MININT32;
}

/**
 * test_gi_int32_return_ptr_max:
 * Returns: (transfer none):
 */
gint32 *
test_gi_int32_return_ptr_max (void)
{
    static gint32 int32 = G_MAXINT32;
    return &int32;
}

/**
 * test_gi_int32_return_ptr_min:
 * Returns: (transfer none):
 */
gint32 *
test_gi_int32_return_ptr_min (void)
{
    static gint32 int32 = G_MININT32;
    return &int32;
}

void
test_gi_int32_in_max (gint32 int32)
{
    g_assert(int32 == G_MAXINT32);
}

void
test_gi_int32_in_min (gint32 int32)
{
    g_assert(int32 == G_MININT32);
}

/**
 * test_gi_int32_in_ptr_max:
 * int32: (in):
 */
void
test_gi_int32_in_ptr_max (gint32 *int32)
{
    g_assert(*int32 == G_MAXINT32);
}

/**
 * test_gi_int32_in_ptr_min:
 * int32: (in):
 */
void
test_gi_int32_in_ptr_min (gint32 *int32)
{
    g_assert(*int32 == G_MININT32);
}

/**
 * test_gi_int32_out_max:
 * int32: (out):
 */
void
test_gi_int32_out_max (gint32 *int32)
{
    *int32 = G_MAXINT32;
}

/**
 * test_gi_int32_out_min:
 * int32: (out):
 */
void
test_gi_int32_out_min (gint32 *int32)
{
    *int32 = G_MININT32;
}

/**
 * test_gi_int32_inout_max_min:
 * int32: (inout):
 */
void
test_gi_int32_inout_max_min (gint32 *int32)
{
    g_assert(*int32 == G_MAXINT32);
    *int32 = G_MININT32;
}

/**
 * test_gi_int32_inout_min_max:
 * int32: (inout):
 */
void
test_gi_int32_inout_min_max (gint32 *int32)
{
    g_assert(*int32 == G_MININT32);
    *int32 = G_MAXINT32;
}


guint32
test_gi_uint32_return (void)
{
    return G_MAXUINT32;
}

/**
 * test_gi_uint32_return_ptr:
 * Returns: (transfer none):
 */
guint32 *
test_gi_uint32_return_ptr (void)
{
    static guint32 uint32 = G_MAXUINT32;
    return &uint32;
}

void
test_gi_uint32_in (guint32 uint32)
{
    g_assert(uint32 == G_MAXUINT32);
}

/**
 * test_gi_uint32_in_ptr:
 * uint32: (in):
 */
void
test_gi_uint32_in_ptr (guint32 *uint32)
{
    g_assert(*uint32 == G_MAXUINT32);
}

/**
 * test_gi_uint32_out:
 * uint32: (out):
 */
void
test_gi_uint32_out (guint32 *uint32)
{
    *uint32 = G_MAXUINT32;
}

/**
 * test_gi_uint32_inout:
 * uint32: (inout):
 */
void
test_gi_uint32_inout (guint32 *uint32)
{
    g_assert(*uint32 == G_MAXUINT32);
    *uint32 = 0;
}


gint64
test_gi_int64_return_max (void)
{
    return G_MAXINT64;
}

gint64
test_gi_int64_return_min (void)
{
    return G_MININT64;
}

/**
 * test_gi_int64_return_ptr_max:
 * Returns: (transfer none):
 */
gint64 *
test_gi_int64_return_ptr_max (void)
{
    static gint64 int64 = G_MAXINT64;
    return &int64;
}

/**
 * test_gi_int64_return_ptr_min:
 * Returns: (transfer none):
 */
gint64 *
test_gi_int64_return_ptr_min (void)
{
    static gint64 int64 = G_MININT64;
    return &int64;
}

void
test_gi_int64_in_max (gint64 int64)
{
    g_assert(int64 == G_MAXINT64);
}

void
test_gi_int64_in_min (gint64 int64)
{
    g_assert(int64 == G_MININT64);
}

/**
 * test_gi_int64_in_ptr_max:
 * int64: (in):
 */
void
test_gi_int64_in_ptr_max (gint64 *int64)
{
    g_assert(*int64 == G_MAXINT64);
}

/**
 * test_gi_int64_in_ptr_min:
 * int64: (in):
 */
void
test_gi_int64_in_ptr_min (gint64 *int64)
{
    g_assert(*int64 == G_MININT64);
}

/**
 * test_gi_int64_out_max:
 * int64: (out):
 */
void
test_gi_int64_out_max (gint64 *int64)
{
    *int64 = G_MAXINT64;
}

/**
 * test_gi_int64_out_min:
 * int64: (out):
 */
void
test_gi_int64_out_min (gint64 *int64)
{
    *int64 = G_MININT64;
}

/**
 * test_gi_int64_inout_max_min:
 * int64: (inout):
 */
void
test_gi_int64_inout_max_min (gint64 *int64)
{
    g_assert(*int64 == G_MAXINT64);
    *int64 = G_MININT64;
}

/**
 * test_gi_int64_inout_min_max:
 * int64: (inout):
 */
void
test_gi_int64_inout_min_max (gint64 *int64)
{
    g_assert(*int64 == G_MININT64);
    *int64 = G_MAXINT64;
}


guint64
test_gi_uint64_return (void)
{
    return G_MAXUINT64;
}

/**
 * test_gi_uint64_return_ptr:
 * Returns: (transfer none):
 */
guint64 *
test_gi_uint64_return_ptr (void)
{
    static guint64 uint64 = G_MAXUINT64;
    return &uint64;
}

void
test_gi_uint64_in (guint64 uint64)
{
    g_assert(uint64 == G_MAXUINT64);
}

/**
 * test_gi_uint64_in_ptr:
 * uint64: (in):
 */
void
test_gi_uint64_in_ptr (guint64 *uint64)
{
    g_assert(*uint64 == G_MAXUINT64);
}

/**
 * test_gi_uint64_out:
 * uint64: (out):
 */
void
test_gi_uint64_out (guint64 *uint64)
{
    *uint64 = G_MAXUINT64;
}

/**
 * test_gi_uint64_inout:
 * uint64: (inout):
 */
void
test_gi_uint64_inout (guint64 *uint64)
{
    g_assert(*uint64 == G_MAXUINT64);
    *uint64 = 0;
}


gshort
test_gi_short_return_max (void)
{
    return G_MAXSHORT;
}

gshort
test_gi_short_return_min (void)
{
    return G_MINSHORT;
}

/**
 * test_gi_short_return_ptr_max:
 * Returns: (transfer none):
 */
gshort *
test_gi_short_return_ptr_max (void)
{
    static gshort short_ = G_MAXSHORT;
    return &short_;
}

/**
 * test_gi_short_return_ptr_min:
 * Returns: (transfer none):
 */
gshort *
test_gi_short_return_ptr_min (void)
{
    static gshort short_ = G_MINSHORT;
    return &short_;
}

void
test_gi_short_in_max (gshort short_)
{
    g_assert(short_ == G_MAXSHORT);
}

void
test_gi_short_in_min (gshort short_)
{
    g_assert(short_ == G_MINSHORT);
}

/**
 * test_gi_short_in_ptr_max:
 * short_: (in):
 */
void
test_gi_short_in_ptr_max (gshort *short_)
{
    g_assert(*short_ == G_MAXSHORT);
}

/**
 * test_gi_short_in_ptr_min:
 * short_: (in):
 */
void
test_gi_short_in_ptr_min (gshort *short_)
{
    g_assert(*short_ == G_MINSHORT);
}

/**
 * test_gi_short_out_max:
 * short_: (out):
 */
void
test_gi_short_out_max (gshort *short_)
{
    *short_ = G_MAXSHORT;
}

/**
 * test_gi_short_out_min:
 * short_: (out):
 */
void
test_gi_short_out_min (gshort *short_)
{
    *short_ = G_MINSHORT;
}

/**
 * test_gi_short_inout_max_min:
 * short_: (inout):
 */
void
test_gi_short_inout_max_min (gshort *short_)
{
    g_assert(*short_ == G_MAXSHORT);
    *short_ = G_MINSHORT;
}

/**
 * test_gi_short_inout_min_max:
 * short_: (inout):
 */
void
test_gi_short_inout_min_max (gshort *short_)
{
    g_assert(*short_ == G_MINSHORT);
    *short_ = G_MAXSHORT;
}


gushort
test_gi_ushort_return (void)
{
    return G_MAXUSHORT;
}

/**
 * test_gi_ushort_return_ptr:
 * Returns: (transfer none):
 */
gushort *
test_gi_ushort_return_ptr (void)
{
    static gushort ushort = G_MAXUSHORT;
    return &ushort;
}

void
test_gi_ushort_in (gushort ushort)
{
    g_assert(ushort == G_MAXUSHORT);
}

/**
 * test_gi_ushort_in_ptr:
 * ushort: (in):
 */
void
test_gi_ushort_in_ptr (gushort *ushort)
{
    g_assert(*ushort == G_MAXUSHORT);
}

/**
 * test_gi_ushort_out:
 * ushort: (out):
 */
void
test_gi_ushort_out (gushort *ushort)
{
    *ushort = G_MAXUSHORT;
}

/**
 * test_gi_ushort_inout:
 * ushort: (inout):
 */
void
test_gi_ushort_inout (gushort *ushort)
{
    g_assert(*ushort == G_MAXUSHORT);
    *ushort = 0;
}


gint
test_gi_int_return_max (void)
{
    return G_MAXINT;
}

gint
test_gi_int_return_min (void)
{
    return G_MININT;
}

/**
 * test_gi_int_return_ptr_max:
 * Returns: (transfer none):
 */
gint *
test_gi_int_return_ptr_max (void)
{
    static gint int_ = G_MAXINT;
    return &int_;
}

/**
 * test_gi_int_return_ptr_min:
 * Returns: (transfer none):
 */
gint *
test_gi_int_return_ptr_min (void)
{
    static gint int_ = G_MININT;
    return &int_;
}

void
test_gi_int_in_max (gint int_)
{
    g_assert(int_ == G_MAXINT);
}

void
test_gi_int_in_min (gint int_)
{
    g_assert(int_ == G_MININT);
}

/**
 * test_gi_int_in_ptr_max:
 * int_: (in):
 */
void
test_gi_int_in_ptr_max (gint *int_)
{
    g_assert(*int_ == G_MAXINT);
}

/**
 * test_gi_int_in_ptr_min:
 * int_: (in):
 */
void
test_gi_int_in_ptr_min (gint *int_)
{
    g_assert(*int_ == G_MININT);
}

/**
 * test_gi_int_out_max:
 * int_: (out):
 */
void
test_gi_int_out_max (gint *int_)
{
    *int_ = G_MAXINT;
}

/**
 * test_gi_int_out_min:
 * int_: (out):
 */
void
test_gi_int_out_min (gint *int_)
{
    *int_ = G_MININT;
}

/**
 * test_gi_int_inout_max_min:
 * int_: (inout):
 */
void
test_gi_int_inout_max_min (gint *int_)
{
    g_assert(*int_ == G_MAXINT);
    *int_ = G_MININT;
}

/**
 * test_gi_int_inout_min_max:
 * int_: (inout):
 */
void
test_gi_int_inout_min_max (gint *int_)
{
    g_assert(*int_ == G_MININT);
    *int_ = G_MAXINT;
}


guint
test_gi_uint_return (void)
{
    return G_MAXUINT;
}

/**
 * test_gi_uint_return_ptr:
 * Returns: (transfer none):
 */
guint *
test_gi_uint_return_ptr (void)
{
    static guint uint = G_MAXUINT;
    return &uint;
}

void
test_gi_uint_in (guint uint)
{
    g_assert(uint == G_MAXUINT);
}

/**
 * test_gi_uint_in_ptr:
 * uint: (in):
 */
void
test_gi_uint_in_ptr (guint *uint)
{
    g_assert(*uint == G_MAXUINT);
}

/**
 * test_gi_uint_out:
 * uint: (out):
 */
void
test_gi_uint_out (guint *uint)
{
    *uint = G_MAXUINT;
}

/**
 * test_gi_uint_inout:
 * uint: (inout):
 */
void
test_gi_uint_inout (guint *uint)
{
    g_assert(*uint == G_MAXUINT);
    *uint = 0;
}


glong
test_gi_long_return_max (void)
{
    return G_MAXLONG;
}

glong
test_gi_long_return_min (void)
{
    return G_MINLONG;
}

/**
 * test_gi_long_return_ptr_max:
 * Returns: (transfer none):
 */
glong *
test_gi_long_return_ptr_max (void)
{
    static glong long_ = G_MAXLONG;
    return &long_;
}

/**
 * test_gi_long_return_ptr_min:
 * Returns: (transfer none):
 */
glong *
test_gi_long_return_ptr_min (void)
{
    static glong long_ = G_MINLONG;
    return &long_;
}

void
test_gi_long_in_max (glong long_)
{
    g_assert(long_ == G_MAXLONG);
}

void
test_gi_long_in_min (glong long_)
{
    g_assert(long_ == G_MINLONG);
}

/**
 * test_gi_long_in_ptr_max:
 * long_: (in):
 */
void
test_gi_long_in_ptr_max (glong *long_)
{
    g_assert(*long_ == G_MAXLONG);
}

/**
 * test_gi_long_in_ptr_min:
 * long_: (in):
 */
void
test_gi_long_in_ptr_min (glong *long_)
{
    g_assert(*long_ == G_MINLONG);
}

/**
 * test_gi_long_out_max:
 * long_: (out):
 */
void
test_gi_long_out_max (glong *long_)
{
    *long_ = G_MAXLONG;
}

/**
 * test_gi_long_out_min:
 * long_: (out):
 */
void
test_gi_long_out_min (glong *long_)
{
    *long_ = G_MINLONG;
}

/**
 * test_gi_long_inout_max_min:
 * long_: (inout):
 */
void
test_gi_long_inout_max_min (glong *long_)
{
    g_assert(*long_ == G_MAXLONG);
    *long_ = G_MINLONG;
}

/**
 * test_gi_long_inout_min_max:
 * long_: (inout):
 */
void
test_gi_long_inout_min_max (glong *long_)
{
    g_assert(*long_ == G_MINLONG);
    *long_ = G_MAXLONG;
}


gulong
test_gi_ulong_return (void)
{
    return G_MAXULONG;
}

/**
 * test_gi_ulong_return_ptr:
 * Returns: (transfer none):
 */
gulong *
test_gi_ulong_return_ptr (void)
{
    static gulong ulong = G_MAXULONG;
    return &ulong;
}

void
test_gi_ulong_in (gulong ulong)
{
    g_assert(ulong == G_MAXULONG);
}

/**
 * test_gi_ulong_in_ptr:
 * ulong: (in):
 */
void
test_gi_ulong_in_ptr (gulong *ulong)
{
    g_assert(*ulong == G_MAXULONG);
}

/**
 * test_gi_ulong_out:
 * ulong: (out):
 */
void
test_gi_ulong_out (gulong *ulong)
{
    *ulong = G_MAXULONG;
}

/**
 * test_gi_ulong_inout:
 * ulong: (inout):
 */
void
test_gi_ulong_inout (gulong *ulong)
{
    g_assert(*ulong == G_MAXULONG);
    *ulong = 0;
}


gssize
test_gi_ssize_return_max (void)
{
    return G_MAXSSIZE;
}

gssize
test_gi_ssize_return_min (void)
{
    return G_MINSSIZE;
}

/**
 * test_gi_ssize_return_ptr_max:
 * Returns: (transfer none):
 */
gssize *
test_gi_ssize_return_ptr_max (void)
{
    static gssize ssize = G_MAXSSIZE;
    return &ssize;
}

/**
 * test_gi_ssize_return_ptr_min:
 * Returns: (transfer none):
 */
gssize *
test_gi_ssize_return_ptr_min (void)
{
    static gssize ssize = G_MINSSIZE;
    return &ssize;
}

void
test_gi_ssize_in_max (gssize ssize)
{
    g_assert(ssize == G_MAXSSIZE);
}

void
test_gi_ssize_in_min (gssize ssize)
{
    g_assert(ssize == G_MINSSIZE);
}

/**
 * test_gi_ssize_in_ptr_max:
 * ssize: (in):
 */
void
test_gi_ssize_in_ptr_max (gssize *ssize)
{
    g_assert(*ssize == G_MAXSSIZE);
}

/**
 * test_gi_ssize_in_ptr_min:
 * ssize: (in):
 */
void
test_gi_ssize_in_ptr_min (gssize *ssize)
{
    g_assert(*ssize == G_MINSSIZE);
}

/**
 * test_gi_ssize_out_max:
 * ssize: (out):
 */
void
test_gi_ssize_out_max (gssize *ssize)
{
    *ssize = G_MAXSSIZE;
}

/**
 * test_gi_ssize_out_min:
 * ssize: (out):
 */
void
test_gi_ssize_out_min (gssize *ssize)
{
    *ssize = G_MINSSIZE;
}

/**
 * test_gi_ssize_inout_max_min:
 * ssize: (inout):
 */
void
test_gi_ssize_inout_max_min (gssize *ssize)
{
    g_assert(*ssize == G_MAXSSIZE);
    *ssize = G_MINSSIZE;
}

/**
 * test_gi_ssize_inout_min_max:
 * ssize: (inout):
 */
void
test_gi_ssize_inout_min_max (gssize *ssize)
{
    g_assert(*ssize == G_MINSSIZE);
    *ssize = G_MAXSSIZE;
}


gsize
test_gi_size_return (void)
{
    return G_MAXSIZE;
}

/**
 * test_gi_size_return_ptr:
 * Returns: (transfer none):
 */
gsize *
test_gi_size_return_ptr (void)
{
    static gsize size = G_MAXSIZE;
    return &size;
}

void
test_gi_size_in (gsize size)
{
    g_assert(size == G_MAXSIZE);
}

/**
 * test_gi_size_in_ptr:
 * size: (in):
 */
void
test_gi_size_in_ptr (gsize *size)
{
    g_assert(*size == G_MAXSIZE);
}

/**
 * test_gi_size_out:
 * size: (out):
 */
void
test_gi_size_out (gsize *size)
{
    *size = G_MAXSIZE;
}

/**
 * test_gi_size_inout:
 * size: (inout):
 */
void
test_gi_size_inout (gsize *size)
{
    g_assert(*size == G_MAXSIZE);
    *size = 0;
}


gfloat
test_gi_float_return (void)
{
    return G_MAXFLOAT;
}

/**
 * test_gi_float_return_ptr:
 * Returns: (transfer none):
 */
gfloat *
test_gi_float_return_ptr (void)
{
    static gfloat float_ = G_MAXFLOAT;
    return &float_;
}

void
test_gi_float_in (gfloat float_)
{
    g_assert(float_ == G_MAXFLOAT);
}

/**
 * test_gi_float_in_ptr:
 * float_: (in):
 */
void
test_gi_float_in_ptr (gfloat *float_)
{
    g_assert(*float_ == G_MAXFLOAT);
}

/**
 * test_gi_float_out:
 * float_: (out):
 */
void
test_gi_float_out (gfloat *float_)
{
    *float_ = G_MAXFLOAT;
}

/**
 * test_gi_float_inout:
 * float_: (inout):
 */
void
test_gi_float_inout (gfloat *float_)
{
    g_assert(*float_ == G_MAXFLOAT);
    *float_ = G_MINFLOAT;
}


gdouble
test_gi_double_return (void)
{
    return G_MAXDOUBLE;
}

/**
 * test_gi_double_return_ptr:
 * Returns: (transfer none):
 */
gdouble *
test_gi_double_return_ptr (void)
{
    static gdouble double_ = G_MAXDOUBLE;
    return &double_;
}

void
test_gi_double_in (gdouble double_)
{
    g_assert(double_ == G_MAXDOUBLE);
}

/**
 * test_gi_double_in_ptr:
 * double_: (in):
 */
void
test_gi_double_in_ptr (gdouble *double_)
{
    g_assert(*double_ == G_MAXDOUBLE);
}

/**
 * test_gi_double_out:
 * double_: (out):
 */
void
test_gi_double_out (gdouble *double_)
{
    *double_ = G_MAXDOUBLE;
}

/**
 * test_gi_double_inout:
 * double_: (inout):
 */
void
test_gi_double_inout (gdouble *double_)
{
    g_assert(*double_ == G_MAXDOUBLE);
    *double_ = G_MINDOUBLE;
}


time_t
test_gi_time_t_return (void)
{
    return 1234567890;
}

/**
 * test_gi_time_t_return_ptr:
 * Returns: (transfer none):
 */
time_t *
test_gi_time_t_return_ptr (void)
{
    static time_t time_t_ = 1234567890;
    return &time_t_;
}

void
test_gi_time_t_in (time_t time_t_)
{
    g_assert(time_t_ == 1234567890);
}

/**
 * test_gi_time_t_in_ptr:
 * time_t_: (in):
 */
void
test_gi_time_t_in_ptr (time_t *time_t_)
{
    g_assert(*time_t_ == 1234567890);
}

/**
 * test_gi_time_t_out:
 * time_t_: (out):
 */
void
test_gi_time_t_out (time_t *time_t_)
{
    *time_t_ = 1234567890;
}

/**
 * test_gi_time_t_inout:
 * time_t_: (inout):
 */
void
test_gi_time_t_inout (time_t *time_t_)
{
    g_assert(*time_t_ == 1234567890);
    *time_t_ = 0;
}


GType
test_gi_gtype_return (void)
{
    return G_TYPE_NONE;
}

/**
 * test_gi_gtype_return_ptr:
 * Returns: (transfer none):
 */
GType *
test_gi_gtype_return_ptr (void)
{
    static GType gtype = G_TYPE_NONE;
    return &gtype;
}

void
test_gi_gtype_in (GType gtype)
{
    g_assert(gtype == G_TYPE_NONE);
}

/**
 * test_gi_gtype_in_ptr:
 * gtype: (in):
 */
void
test_gi_gtype_in_ptr (GType *gtype)
{
    g_assert(*gtype == G_TYPE_NONE);
}

/**
 * test_gi_gtype_out:
 * gtype: (out):
 */
void
test_gi_gtype_out (GType *gtype)
{
    *gtype = G_TYPE_NONE;
}

/**
 * test_gi_gtype_inout:
 * gtype: (inout):
 */
void
test_gi_gtype_inout (GType *gtype)
{
    g_assert(*gtype == G_TYPE_NONE);
    *gtype = G_TYPE_INT;
}


const gchar *
test_gi_utf8_none_return (void)
{
    return TESTGI_CONSTANT_UTF8;
}

gchar *
test_gi_utf8_full_return (void)
{
    return g_strdup(TESTGI_CONSTANT_UTF8);
}

void
test_gi_utf8_none_in (const gchar *utf8)
{
    g_assert(strcmp(TESTGI_CONSTANT_UTF8, utf8) == 0);
}

void
test_gi_utf8_full_in (gchar *utf8)
{
    g_assert(strcmp(TESTGI_CONSTANT_UTF8, utf8) == 0);
    g_free(utf8);
}

/**
 * test_gi_utf8_none_out:
 * utf8: (out) (transfer none):
 */
void
test_gi_utf8_none_out (gchar **utf8)
{
    *utf8 = TESTGI_CONSTANT_UTF8;
}

/**
 * test_gi_utf8_full_out:
 * utf8: (out) (transfer full):
 */
void
test_gi_utf8_full_out (gchar **utf8)
{
    *utf8 = g_strdup(TESTGI_CONSTANT_UTF8);
}

/**
 * test_gi_utf8_none_inout:
 * utf8: (inout) (transfer none):
 */
void
test_gi_utf8_none_inout (gchar **utf8)
{
    g_assert(strcmp(TESTGI_CONSTANT_UTF8, *utf8) == 0);
    *utf8 = "";
}

/**
 * test_gi_utf8_full_inout:
 * utf8: (inout) (transfer full):
 */
void
test_gi_utf8_full_inout (gchar **utf8)
{
    g_assert(strcmp(TESTGI_CONSTANT_UTF8, *utf8) == 0);
    g_free(*utf8);
    *utf8 = g_strdup("");
}


/**
 * test_gi_array_fixed_int_return:
 * Returns: (array fixed-size=4):
 */
const gint *
test_gi_array_fixed_int_return (void)
{
    static gint ints[] = {-1, 0, 1, 2};
    return ints;
}

/**
 * test_gi_array_fixed_short_return:
 * Returns: (array fixed-size=4):
 */
const gshort *
test_gi_array_fixed_short_return (void)
{
    static gshort shorts[] = {-1, 0, 1, 2};
    return shorts;
}

/**
 * test_gi_array_fixed_int_in:
 * @ints: (array fixed-size=4):
 */
void
test_gi_array_fixed_int_in (const gint *ints)
{
    g_assert(ints[0] == -1);
    g_assert(ints[1] == 0);
    g_assert(ints[2] == 1);
    g_assert(ints[3] == 2);
}

/**
 * test_gi_array_fixed_short_in:
 * @shorts: (array fixed-size=4):
 */
void
test_gi_array_fixed_short_in (const gshort *shorts)
{
    g_assert(shorts[0] == -1);
    g_assert(shorts[1] == 0);
    g_assert(shorts[2] == 1);
    g_assert(shorts[3] == 2);
}

/**
 * test_gi_array_fixed_out:
 * @ints: (out) (array fixed-size=4) (transfer none):
 */
void
test_gi_array_fixed_out (gint **ints)
{
    static gint values[] = {-1, 0, 1, 2};
    *ints = values;
}

/**
 * test_gi_array_fixed_out_struct:
 * @structs: (out) (array fixed-size=2) (transfer none):
 */
void
test_gi_array_fixed_out_struct (TestGISimpleStruct **structs)
{
    static TestGISimpleStruct *values;

    if (values == NULL) {
        values = g_new(TestGISimpleStruct, 2);

        values[0].long_ = 7;
        values[0].int8 = 6;

        values[1].long_ = 6;
        values[1].int8 = 7;
    }

    *structs = values;
}

/**
 * test_gi_array_fixed_inout:
 * @ints: (inout) (array fixed-size=4) (transfer none):
 */
void
test_gi_array_fixed_inout (gint **ints)
{
    static gint values[] = {2, 1, 0, -1};

    g_assert((*ints)[0] == -1);
    g_assert((*ints)[1] == 0);
    g_assert((*ints)[2] == 1);
    g_assert((*ints)[3] == 2);

    *ints = values;
}


/**
 * test_gi_array_return:
 * Returns: (array length=length):
 */
const gint *
test_gi_array_return (gint *length)
{
    static gint ints[] = {-1, 0, 1, 2};

    *length = 4;
    return ints;
}

/**
 * test_gi_array_in:
 * @ints: (array length=length):
 */
void
test_gi_array_in (const gint *ints, gint length)
{
    g_assert(length == 4);
    g_assert(ints[0] == -1);
    g_assert(ints[1] == 0);
    g_assert(ints[2] == 1);
    g_assert(ints[3] == 2);
}

/**
 * test_gi_array_out:
 * @ints: (out) (array length=length) (transfer none):
 */
void
test_gi_array_out (gint **ints, gint *length)
{
    static gint values[] = {-1, 0, 1, 2};

    *length = 4;
    *ints = values;
}

/**
 * test_gi_array_inout:
 * @ints: (inout) (array length=length) (transfer none):
 * @length: (inout):
 */
void
test_gi_array_inout (gint **ints, gint *length)
{
    static gint values[] = {-2, -1, 0, 1, 2};

    g_assert(*length == 4);
    g_assert((*ints)[0] == -1);
    g_assert((*ints)[1] == 0);
    g_assert((*ints)[2] == 1);
    g_assert((*ints)[3] == 2);

    *length = 5;
    *ints = values;
}

/**
 * test_gi_array_zero_terminated_return:
 * Returns: (array zero-terminated=1) (transfer none):
 */
gchar **
test_gi_array_zero_terminated_return (void)
{
    static gchar *values[] = {"0", "1", "2", NULL};
    return values;
}

/**
 * test_gi_array_zero_terminated_in:
 * @utf8s: (array zero-terminated=1) (transfer none):
 */
void
test_gi_array_zero_terminated_in (gchar **utf8s)
{
    g_assert(g_strv_length(utf8s));
    g_assert(strcmp(utf8s[0], "0") == 0);
    g_assert(strcmp(utf8s[1], "1") == 0);
    g_assert(strcmp(utf8s[2], "2") == 0);
}

/**
 * test_gi_array_zero_terminated_out:
 * @utf8s: (out) (array zero-terminated=1) (transfer none):
 */
void
test_gi_array_zero_terminated_out (gchar ***utf8s)
{
    static gchar *values[] = {"0", "1", "2", NULL};
    *utf8s = values;
}

/**
 * test_gi_array_zero_terminated_inout:
 * @utf8s: (inout) (array zero-terminated=1) (transfer none):
 */
void
test_gi_array_zero_terminated_inout (gchar ***utf8s)
{
    static gchar *values[] = {"-1", "0", "1", "2", NULL};

    g_assert(g_strv_length(*utf8s));
    g_assert(strcmp((*utf8s)[0], "0") == 0);
    g_assert(strcmp((*utf8s)[1], "1") == 0);
    g_assert(strcmp((*utf8s)[2], "2") == 0);

    *utf8s = values;
}


/**
 * test_gi_glist_int_none_return:
 * Returns: (element-type gint) (transfer none):
 */
GList *
test_gi_glist_int_none_return (void)
{
    static GList *list = NULL;

    if (list == NULL) {
        list = g_list_append(list, GINT_TO_POINTER(-1));
        list = g_list_append(list, GINT_TO_POINTER(0));
        list = g_list_append(list, GINT_TO_POINTER(1));
        list = g_list_append(list, GINT_TO_POINTER(2));
    }

    return list;
}

/**
 * test_gi_glist_utf8_none_return:
 * Returns: (element-type utf8) (transfer none):
 */
GList *
test_gi_glist_utf8_none_return (void)
{
    static GList *list = NULL;

    if (list == NULL) {
        list = g_list_append(list, "0");
        list = g_list_append(list, "1");
        list = g_list_append(list, "2");
    }

    return list;
}

/**
 * test_gi_glist_utf8_container_return:
 * Returns: (element-type utf8) (transfer container):
 */
GList *
test_gi_glist_utf8_container_return (void)
{
    GList *list = NULL;

    list = g_list_append(list, "0");
    list = g_list_append(list, "1");
    list = g_list_append(list, "2");

    return list;
}

/**
 * test_gi_glist_utf8_full_return:
 * Returns: (element-type utf8) (transfer full):
 */
GList *
test_gi_glist_utf8_full_return (void)
{
    GList *list = NULL;

    list = g_list_append(list, g_strdup("0"));
    list = g_list_append(list, g_strdup("1"));
    list = g_list_append(list, g_strdup("2"));

    return list;
}

/**
 * test_gi_glist_int_none_in:
 * @list: (element-type gint) (transfer none):
 */
void
test_gi_glist_int_none_in (GList *list)
{
    g_assert(g_list_length(list) == 4);
    g_assert(GPOINTER_TO_INT(g_list_nth_data(list, 0)) == -1);
    g_assert(GPOINTER_TO_INT(g_list_nth_data(list, 1)) == 0);
    g_assert(GPOINTER_TO_INT(g_list_nth_data(list, 2)) == 1);
    g_assert(GPOINTER_TO_INT(g_list_nth_data(list, 3)) == 2);
}

/**
 * test_gi_glist_utf8_none_in:
 * @list: (element-type utf8) (transfer none):
 */
void
test_gi_glist_utf8_none_in (GList *list)
{
    g_assert(g_list_length(list) == 3);
    g_assert(strcmp(g_list_nth_data(list, 0), "0") == 0);
    g_assert(strcmp(g_list_nth_data(list, 1), "1") == 0);
    g_assert(strcmp(g_list_nth_data(list, 2), "2") == 0);
}

/**
 * test_gi_glist_utf8_container_in:
 * @list: (element-type utf8) (transfer container):
 */
void
test_gi_glist_utf8_container_in (GList *list)
{
    g_assert(g_list_length(list) == 3);
    g_assert(strcmp(g_list_nth_data(list, 0), "0") == 0);
    g_assert(strcmp(g_list_nth_data(list, 1), "1") == 0);
    g_assert(strcmp(g_list_nth_data(list, 2), "2") == 0);
    g_list_free(list);
}

/**
 * test_gi_glist_utf8_full_in:
 * @list: (element-type utf8) (transfer full):
 */
void
test_gi_glist_utf8_full_in (GList *list)
{
    g_assert(g_list_length(list) == 3);
    g_assert(strcmp(g_list_nth_data(list, 0), "0") == 0);
    g_assert(strcmp(g_list_nth_data(list, 1), "1") == 0);
    g_assert(strcmp(g_list_nth_data(list, 2), "2") == 0);
    g_free(g_list_nth_data(list, 0));
    g_free(g_list_nth_data(list, 1));
    g_free(g_list_nth_data(list, 2));
    g_list_free(list);
}

/**
 * test_gi_glist_utf8_none_out:
 * @list: (out) (element-type utf8) (transfer none):
 */
void
test_gi_glist_utf8_none_out (GList **list)
{
    static GList *values = NULL;

    if (values == NULL) {
        values = g_list_append(values, "0");
        values = g_list_append(values, "1");
        values = g_list_append(values, "2");
    }

    *list = values;
}

/**
 * test_gi_glist_utf8_container_out:
 * @list: (out) (element-type utf8) (transfer container):
 */
void
test_gi_glist_utf8_container_out (GList **list)
{
    *list = NULL;

    *list = g_list_append(*list, "0");
    *list = g_list_append(*list, "1");
    *list = g_list_append(*list, "2");
}

/**
 * test_gi_glist_utf8_full_out:
 * @list: (out) (element-type utf8) (transfer full):
 */
void
test_gi_glist_utf8_full_out (GList **list)
{
    *list = NULL;

    *list = g_list_append(*list, g_strdup("0"));
    *list = g_list_append(*list, g_strdup("1"));
    *list = g_list_append(*list, g_strdup("2"));
}

/**
 * test_gi_glist_utf8_none_inout:
 * @list: (inout) (element-type utf8) (transfer none):
 */
void
test_gi_glist_utf8_none_inout (GList **list)
{
    static GList *values = NULL;

    g_assert(g_list_length(*list) == 3);
    g_assert(strcmp(g_list_nth_data(*list, 0), "0") == 0);
    g_assert(strcmp(g_list_nth_data(*list, 1), "1") == 0);
    g_assert(strcmp(g_list_nth_data(*list, 2), "2") == 0);

    if (values == NULL) {
        values = g_list_append(values, "-2");
        values = g_list_append(values, "-1");
        values = g_list_append(values, "0");
        values = g_list_append(values, "1");
    }

    *list = values;
}

/**
 * test_gi_glist_utf8_container_inout:
 * @list: (inout) (element-type utf8) (transfer container):
 */
void
test_gi_glist_utf8_container_inout (GList **list)
{
    g_assert(g_list_length(*list) == 3);
    g_assert(strcmp(g_list_nth_data(*list, 0), "0") == 0);
    g_assert(strcmp(g_list_nth_data(*list, 1), "1") == 0);
    g_assert(strcmp(g_list_nth_data(*list, 2), "2") == 0);

    *list = g_list_remove_link(*list, g_list_last(*list));

    *list = g_list_prepend(*list, "-1");
    *list = g_list_prepend(*list, "-2");
}

/**
 * test_gi_glist_utf8_full_inout:
 * @list: (inout) (element-type utf8) (transfer full):
 */
void
test_gi_glist_utf8_full_inout (GList **list)
{
    gpointer *data;

    g_assert(g_list_length(*list) == 3);
    g_assert(strcmp(g_list_nth_data(*list, 0), "0") == 0);
    g_assert(strcmp(g_list_nth_data(*list, 1), "1") == 0);
    g_assert(strcmp(g_list_nth_data(*list, 2), "2") == 0);

    data = g_list_last(*list)->data;
    *list = g_list_remove(*list, data);
    g_free(data);

    *list = g_list_prepend(*list, g_strdup("-1"));
    *list = g_list_prepend(*list, g_strdup("-2"));
}


/**
 * test_gi_gslist_int_none_return:
 * Returns: (element-type gint) (transfer none):
 */
GSList *
test_gi_gslist_int_none_return (void)
{
    static GSList *list = NULL;

    if (list == NULL) {
        list = g_slist_prepend(list, GINT_TO_POINTER(-1));
        list = g_slist_prepend(list, GINT_TO_POINTER(0));
        list = g_slist_prepend(list, GINT_TO_POINTER(1));
        list = g_slist_prepend(list, GINT_TO_POINTER(2));
        list = g_slist_reverse(list);
    }

    return list;
}

/**
 * test_gi_gslist_utf8_none_return:
 * Returns: (element-type utf8) (transfer none):
 */
GSList *
test_gi_gslist_utf8_none_return (void)
{
    static GSList *list = NULL;

    if (list == NULL) {
        list = g_slist_prepend(list, "0");
        list = g_slist_prepend(list, "1");
        list = g_slist_prepend(list, "2");
        list = g_slist_reverse(list);
    }

    return list;
}

/**
 * test_gi_gslist_utf8_container_return:
 * Returns: (element-type utf8) (transfer container):
 */
GSList *
test_gi_gslist_utf8_container_return (void)
{
    GSList *list = NULL;

    list = g_slist_prepend(list, "0");
    list = g_slist_prepend(list, "1");
    list = g_slist_prepend(list, "2");
    list = g_slist_reverse(list);

    return list;
}

/**
 * test_gi_gslist_utf8_full_return:
 * Returns: (element-type utf8) (transfer full):
 */
GSList *
test_gi_gslist_utf8_full_return (void)
{
    GSList *list = NULL;

    list = g_slist_prepend(list, g_strdup("0"));
    list = g_slist_prepend(list, g_strdup("1"));
    list = g_slist_prepend(list, g_strdup("2"));
    list = g_slist_reverse(list);

    return list;
}

/**
 * test_gi_gslist_int_none_in:
 * @list: (element-type gint) (transfer none):
 */
void
test_gi_gslist_int_none_in (GSList *list)
{
    g_assert(g_slist_length(list) == 4);
    g_assert(GPOINTER_TO_INT(g_slist_nth_data(list, 0)) == -1);
    g_assert(GPOINTER_TO_INT(g_slist_nth_data(list, 1)) == 0);
    g_assert(GPOINTER_TO_INT(g_slist_nth_data(list, 2)) == 1);
    g_assert(GPOINTER_TO_INT(g_slist_nth_data(list, 3)) == 2);
}

/**
 * test_gi_gslist_utf8_none_in:
 * @list: (element-type utf8) (transfer none):
 */
void
test_gi_gslist_utf8_none_in (GSList *list)
{
    g_assert(g_slist_length(list) == 3);
    g_assert(strcmp(g_slist_nth_data(list, 0), "0") == 0);
    g_assert(strcmp(g_slist_nth_data(list, 1), "1") == 0);
    g_assert(strcmp(g_slist_nth_data(list, 2), "2") == 0);
}

/**
 * test_gi_gslist_utf8_container_in:
 * @list: (element-type utf8) (transfer container):
 */
void
test_gi_gslist_utf8_container_in (GSList *list)
{
    g_assert(g_slist_length(list) == 3);
    g_assert(strcmp(g_slist_nth_data(list, 0), "0") == 0);
    g_assert(strcmp(g_slist_nth_data(list, 1), "1") == 0);
    g_assert(strcmp(g_slist_nth_data(list, 2), "2") == 0);
    g_slist_free(list);
}

/**
 * test_gi_gslist_utf8_full_in:
 * @list: (element-type utf8) (transfer full):
 */
void
test_gi_gslist_utf8_full_in (GSList *list)
{
    g_assert(g_slist_length(list) == 3);
    g_assert(strcmp(g_slist_nth_data(list, 0), "0") == 0);
    g_assert(strcmp(g_slist_nth_data(list, 1), "1") == 0);
    g_assert(strcmp(g_slist_nth_data(list, 2), "2") == 0);
    g_free(g_slist_nth_data(list, 0));
    g_free(g_slist_nth_data(list, 1));
    g_free(g_slist_nth_data(list, 2));
    g_slist_free(list);
}

/**
 * test_gi_gslist_utf8_none_out:
 * @list: (out) (element-type utf8) (transfer none):
 */
void
test_gi_gslist_utf8_none_out (GSList **list)
{
    static GSList *values = NULL;

    if (values == NULL) {
        values = g_slist_prepend(values, "0");
        values = g_slist_prepend(values, "1");
        values = g_slist_prepend(values, "2");
        values = g_slist_reverse(values);
    }

    *list = values;
}

/**
 * test_gi_gslist_utf8_container_out:
 * @list: (out) (element-type utf8) (transfer container):
 */
void
test_gi_gslist_utf8_container_out (GSList **list)
{
    *list = NULL;

    *list = g_slist_prepend(*list, "0");
    *list = g_slist_prepend(*list, "1");
    *list = g_slist_prepend(*list, "2");
    *list = g_slist_reverse(*list);
}

/**
 * test_gi_gslist_utf8_full_out:
 * @list: (out) (element-type utf8) (transfer full):
 */
void
test_gi_gslist_utf8_full_out (GSList **list)
{
    *list = NULL;

    *list = g_slist_prepend(*list, g_strdup("0"));
    *list = g_slist_prepend(*list, g_strdup("1"));
    *list = g_slist_prepend(*list, g_strdup("2"));
    *list = g_slist_reverse(*list);
}

/**
 * test_gi_gslist_utf8_none_inout:
 * @list: (inout) (element-type utf8) (transfer none):
 */
void
test_gi_gslist_utf8_none_inout (GSList **list)
{
    static GSList *values = NULL;

    g_assert(g_slist_length(*list) == 3);
    g_assert(strcmp(g_slist_nth_data(*list, 0), "0") == 0);
    g_assert(strcmp(g_slist_nth_data(*list, 1), "1") == 0);
    g_assert(strcmp(g_slist_nth_data(*list, 2), "2") == 0);

    if (values == NULL) {
        values = g_slist_prepend(values, "-2");
        values = g_slist_prepend(values, "-1");
        values = g_slist_prepend(values, "0");
        values = g_slist_prepend(values, "1");
        values = g_slist_reverse(values);
    }

    *list = values;
}

/**
 * test_gi_gslist_utf8_container_inout:
 * @list: (inout) (element-type utf8) (transfer container):
 */
void
test_gi_gslist_utf8_container_inout (GSList **list)
{
    g_assert(g_slist_length(*list) == 3);
    g_assert(strcmp(g_slist_nth_data(*list, 0), "0") == 0);
    g_assert(strcmp(g_slist_nth_data(*list, 1), "1") == 0);
    g_assert(strcmp(g_slist_nth_data(*list, 2), "2") == 0);

    *list = g_slist_remove_link(*list, g_slist_last(*list));

    *list = g_slist_prepend(*list, "-1");
    *list = g_slist_prepend(*list, "-2");
}

/**
 * test_gi_gslist_utf8_full_inout:
 * @list: (inout) (element-type utf8) (transfer full):
 */
void
test_gi_gslist_utf8_full_inout (GSList **list)
{
    gpointer *data;

    g_assert(g_slist_length(*list) == 3);
    g_assert(strcmp(g_slist_nth_data(*list, 0), "0") == 0);
    g_assert(strcmp(g_slist_nth_data(*list, 1), "1") == 0);
    g_assert(strcmp(g_slist_nth_data(*list, 2), "2") == 0);

    data = g_slist_last(*list)->data;
    *list = g_slist_remove(*list, data);
    g_free(data);

    *list = g_slist_prepend(*list, g_strdup("-1"));
    *list = g_slist_prepend(*list, g_strdup("-2"));
}


/**
 * test_gi_ghashtable_int_none_return:
 * Returns: (element-type gint gint) (transfer none):
 */
GHashTable *
test_gi_ghashtable_int_none_return (void)
{
    static GHashTable *hash_table = NULL;

    if (hash_table == NULL) {
        hash_table = g_hash_table_new(NULL, NULL);
        g_hash_table_insert(hash_table, GINT_TO_POINTER(-1), GINT_TO_POINTER(1));
        g_hash_table_insert(hash_table, GINT_TO_POINTER(0), GINT_TO_POINTER(0));
        g_hash_table_insert(hash_table, GINT_TO_POINTER(1), GINT_TO_POINTER(-1));
        g_hash_table_insert(hash_table, GINT_TO_POINTER(2), GINT_TO_POINTER(-2));
    }

    return hash_table;
}

/**
 * test_gi_ghashtable_utf8_none_return:
 * Returns: (element-type utf8 utf8) (transfer none):
 */
GHashTable *
test_gi_ghashtable_utf8_none_return (void)
{
    static GHashTable *hash_table = NULL;

    if (hash_table == NULL) {
        hash_table = g_hash_table_new(g_str_hash, g_str_equal);
        g_hash_table_insert(hash_table, "-1", "1");
        g_hash_table_insert(hash_table, "0", "0");
        g_hash_table_insert(hash_table, "1", "-1");
        g_hash_table_insert(hash_table, "2", "-2");
    }

    return hash_table;
}

/**
 * test_gi_ghashtable_utf8_container_return:
 * Returns: (element-type utf8 utf8) (transfer container):
 */
GHashTable *
test_gi_ghashtable_utf8_container_return (void)
{
    GHashTable *hash_table = NULL;

    hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(hash_table, "-1", "1");
    g_hash_table_insert(hash_table, "0", "0");
    g_hash_table_insert(hash_table, "1", "-1");
    g_hash_table_insert(hash_table, "2", "-2");

    return hash_table;
}

/**
 * test_gi_ghashtable_utf8_full_return:
 * Returns: (element-type utf8 utf8) (transfer full):
 */
GHashTable *
test_gi_ghashtable_utf8_full_return (void)
{
    GHashTable *hash_table = NULL;

    hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(hash_table, g_strdup("-1"), g_strdup("1"));
    g_hash_table_insert(hash_table, g_strdup("0"), g_strdup("0"));
    g_hash_table_insert(hash_table, g_strdup("1"), g_strdup("-1"));
    g_hash_table_insert(hash_table, g_strdup("2"), g_strdup("-2"));

    return hash_table;
}

/**
 * test_gi_ghashtable_int_none_in:
 * @hash_table: (element-type gint gint) (transfer none):
 */
void
test_gi_ghashtable_int_none_in (GHashTable *hash_table)
{
    g_assert(GPOINTER_TO_INT(g_hash_table_lookup(hash_table, GINT_TO_POINTER(-1))) == 1);
    g_assert(GPOINTER_TO_INT(g_hash_table_lookup(hash_table, GINT_TO_POINTER(0))) == 0);
    g_assert(GPOINTER_TO_INT(g_hash_table_lookup(hash_table, GINT_TO_POINTER(1))) == -1);
    g_assert(GPOINTER_TO_INT(g_hash_table_lookup(hash_table, GINT_TO_POINTER(2))) == -2);
}

/**
 * test_gi_ghashtable_utf8_none_in:
 * @hash_table: (element-type utf8 utf8) (transfer none):
 */
void
test_gi_ghashtable_utf8_none_in (GHashTable *hash_table)
{
    g_assert(strcmp(g_hash_table_lookup(hash_table, "-1"), "1") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "0"), "0") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "1"), "-1") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "2"), "-2") == 0);
}

/**
 * test_gi_ghashtable_utf8_container_in:
 * @hash_table: (element-type utf8 utf8) (transfer container):
 */
void
test_gi_ghashtable_utf8_container_in (GHashTable *hash_table)
{
    g_assert(strcmp(g_hash_table_lookup(hash_table, "-1"), "1") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "0"), "0") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "1"), "-1") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "2"), "-2") == 0);
    g_hash_table_steal_all(hash_table);
    g_hash_table_unref(hash_table);
}

/**
 * test_gi_ghashtable_utf8_full_in:
 * @hash_table: (element-type utf8 utf8) (transfer full):
 */
void
test_gi_ghashtable_utf8_full_in (GHashTable *hash_table)
{
    GHashTableIter hash_table_iter;
    gpointer key, value;

    g_assert(strcmp(g_hash_table_lookup(hash_table, "-1"), "1") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "0"), "0") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "1"), "-1") == 0);
    g_assert(strcmp(g_hash_table_lookup(hash_table, "2"), "-2") == 0);

    g_hash_table_iter_init(&hash_table_iter, hash_table);
    while (g_hash_table_iter_next(&hash_table_iter, &key, &value)) {
        g_free(key);
        g_free(value);
        g_hash_table_iter_steal(&hash_table_iter);
    }

    g_hash_table_unref(hash_table);
}

/**
 * test_gi_ghashtable_utf8_none_out:
 * @hash_table: (out) (element-type utf8 utf8) (transfer none):
 */
void
test_gi_ghashtable_utf8_none_out (GHashTable **hash_table)
{
    static GHashTable *new_hash_table = NULL;

    if (new_hash_table == NULL) {
        new_hash_table = g_hash_table_new(g_str_hash, g_str_equal);
        g_hash_table_insert(new_hash_table, "-1", "1");
        g_hash_table_insert(new_hash_table, "0", "0");
        g_hash_table_insert(new_hash_table, "1", "-1");
        g_hash_table_insert(new_hash_table, "2", "-2");
    }

    *hash_table = new_hash_table;
}

/**
 * test_gi_ghashtable_utf8_container_out:
 * @hash_table: (out) (element-type utf8 utf8) (transfer container):
 */
void
test_gi_ghashtable_utf8_container_out (GHashTable **hash_table)
{
    *hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(*hash_table, "-1", "1");
    g_hash_table_insert(*hash_table, "0", "0");
    g_hash_table_insert(*hash_table, "1", "-1");
    g_hash_table_insert(*hash_table, "2", "-2");
}

/**
 * test_gi_ghashtable_utf8_full_out:
 * @hash_table: (out) (element-type utf8 utf8) (transfer full):
 */
void
test_gi_ghashtable_utf8_full_out (GHashTable **hash_table)
{
    *hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(*hash_table, g_strdup("-1"), g_strdup("1"));
    g_hash_table_insert(*hash_table, g_strdup("0"), g_strdup("0"));
    g_hash_table_insert(*hash_table, g_strdup("1"), g_strdup("-1"));
    g_hash_table_insert(*hash_table, g_strdup("2"), g_strdup("-2"));
}

/**
 * test_gi_ghashtable_utf8_none_inout:
 * @hash_table: (inout) (element-type utf8 utf8) (transfer none):
 */
void
test_gi_ghashtable_utf8_none_inout (GHashTable **hash_table)
{
    static GHashTable *new_hash_table = NULL;

    g_assert(strcmp(g_hash_table_lookup(*hash_table, "-1"), "1") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "0"), "0") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "1"), "-1") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "2"), "-2") == 0);

    if (new_hash_table == NULL) {
        new_hash_table = g_hash_table_new(g_str_hash, g_str_equal);
        g_hash_table_insert(new_hash_table, "-1", "1");
        g_hash_table_insert(new_hash_table, "0", "0");
        g_hash_table_insert(new_hash_table, "1", "1");
    }

    *hash_table = new_hash_table;
}

/**
 * test_gi_ghashtable_utf8_container_inout:
 * @hash_table: (inout) (element-type utf8 utf8) (transfer container):
 */
void
test_gi_ghashtable_utf8_container_inout (GHashTable **hash_table)
{
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "-1"), "1") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "0"), "0") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "1"), "-1") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "2"), "-2") == 0);

    g_hash_table_steal(*hash_table, "2");
    g_hash_table_steal(*hash_table, "1");
    g_hash_table_insert(*hash_table, "1", "1");
}

/**
 * test_gi_ghashtable_utf8_full_inout:
 * @hash_table: (inout) (element-type utf8 utf8) (transfer full):
 */
void
test_gi_ghashtable_utf8_full_inout (GHashTable **hash_table)
{
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "-1"), "1") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "0"), "0") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "1"), "-1") == 0);
    g_assert(strcmp(g_hash_table_lookup(*hash_table, "2"), "-2") == 0);

    g_hash_table_steal(*hash_table, "2");
    g_hash_table_steal(*hash_table, "1");
    g_hash_table_insert(*hash_table, "1", g_strdup("1"));
}


/**
 * test_gi_gvalue_return:
 * Returns: (transfer none):
 */
GValue *
test_gi_gvalue_return (void)
{
    static GValue *value = NULL;

    if (value == NULL) {
        value = g_new0(GValue, 1);
        g_value_init(value, G_TYPE_INT);
        g_value_set_int(value, 42);
    }

    return value;
}

/**
 * test_gi_gvalue_in:
 * @value: (transfer none):
 */
void
test_gi_gvalue_in (GValue *value)
{
    g_assert(g_value_get_int(value) == 42);
}

/**
 * test_gi_gvalue_out:
 * @value: (out) (transfer none):
 */
void
test_gi_gvalue_out (GValue **value)
{
    static GValue *new_value = NULL;

    if (new_value == NULL) {
        new_value = g_new0(GValue, 1);
        g_value_init(new_value, G_TYPE_INT);
        g_value_set_int(new_value, 42);
    }

    *value = new_value;
}

/**
 * test_gi_gvalue_inout:
 * @value: (inout) (transfer none):
 */
void
test_gi_gvalue_inout (GValue **value)
{
    g_assert(g_value_get_int(*value) == 42);
    g_value_unset(*value);
    g_value_init(*value, G_TYPE_STRING);
    g_value_set_string(*value, "42");
}


/**
 * test_gi_gclosure_in:
 * @closure: (transfer none):
 */
void
test_gi_gclosure_in (GClosure *closure)
{
    GValue return_value = {0, };

    g_value_init (&return_value, G_TYPE_INT);

    g_closure_invoke (closure,
            &return_value,
            0, NULL,
            NULL);

    g_assert(g_value_get_int (&return_value) == 42);

    g_value_unset(&return_value);
}

gpointer
test_gi_pointer_in_return (gpointer pointer)
{
    return pointer;
}

GType
test_gi_enum_get_type (void)
{
    static GType type = 0;
    if (G_UNLIKELY(type == 0)) {
        static const GEnumValue values[] = {
            { TESTGI_ENUM_VALUE1, "TESTGI_ENUM_VALUE1", "value1" },
            { TESTGI_ENUM_VALUE2, "TESTGI_ENUM_VALUE2", "value2" },
            { TESTGI_ENUM_VALUE3, "TESTGI_ENUM_VALUE3", "value3" },
            { 0, NULL, NULL }
        };
        type = g_enum_register_static (g_intern_static_string ("TestGIEnum"), values);
    }

    return type;
}

TestGIEnum
test_gi_enum_return (void)
{
    return TESTGI_ENUM_VALUE3;
}

void
test_gi_enum_in (TestGIEnum enum_)
{
    g_assert(enum_ == TESTGI_ENUM_VALUE3);
}

/**
 * test_gi_enum_in_ptr:
 * @enum_: (in) (transfer none):
 */
void
test_gi_enum_in_ptr (TestGIEnum *enum_)
{
    g_assert(*enum_ == TESTGI_ENUM_VALUE3);
}

/**
 * test_gi_enum_out:
 * @enum_: (out):
 */
void
test_gi_enum_out (TestGIEnum *enum_)
{
    *enum_ = TESTGI_ENUM_VALUE3;
}

/**
 * test_gi_enum_inout:
 * @enum_: (inout):
 */
void
test_gi_enum_inout (TestGIEnum *enum_)
{
    g_assert(*enum_ == TESTGI_ENUM_VALUE3);
    *enum_ = TESTGI_ENUM_VALUE1;
}


GType
test_gi_flags_get_type (void)
{
    static GType type = 0;
    if (G_UNLIKELY(type == 0)) {
        static const GFlagsValue values[] = {
            { TESTGI_FLAGS_VALUE1, "TESTGI_FLAGS_VALUE1", "value1" },
            { TESTGI_FLAGS_VALUE2, "TESTGI_FLAGS_VALUE2", "value2" },
            { TESTGI_FLAGS_VALUE3, "TESTGI_FLAGS_VALUE3", "value3" },
            { 0, NULL, NULL }
        };
        type = g_flags_register_static (g_intern_static_string ("TestGIFlags"), values);
    }

    return type;
}

TestGIFlags
test_gi_flags_return (void)
{
    return TESTGI_FLAGS_VALUE2;
}

void
test_gi_flags_in (TestGIFlags flags_)
{
    g_assert(flags_ == TESTGI_FLAGS_VALUE2);
}

void
test_gi_flags_in_zero (TestGIFlags flags)
{
    g_assert(flags == 0);
}

/**
 * test_gi_flags_in_ptr:
 * @flags_: (in) (transfer none):
 */
void
test_gi_flags_in_ptr (TestGIFlags *flags_)
{
    g_assert(*flags_ == TESTGI_FLAGS_VALUE2);
}

/**
 * test_gi_flags_out:
 * @flags_: (out):
 */
void
test_gi_flags_out (TestGIFlags *flags_)
{
    *flags_ = TESTGI_FLAGS_VALUE2;
}

/**
 * test_gi_flags_inout:
 * @flags_: (inout):
 */
void
test_gi_flags_inout (TestGIFlags *flags_)
{
    g_assert(*flags_ == TESTGI_FLAGS_VALUE2);
    *flags_ = TESTGI_FLAGS_VALUE1;
}


/**
 * test_gi__simple_struct_return:
 * Returns: (transfer none):
 */
TestGISimpleStruct *
test_gi__simple_struct_return (void)
{
    static TestGISimpleStruct *struct_ = NULL;

    if (struct_ == NULL) {
        struct_ = g_new(TestGISimpleStruct, 1);

        struct_->long_ = 6;
        struct_->int8 = 7;
    }

    return struct_;
}

/**
 * test_gi__simple_struct_in:
 * @struct_: (transfer none):
 */
void
test_gi__simple_struct_in (TestGISimpleStruct *struct_)
{
    g_assert(struct_->long_ == 6);
    g_assert(struct_->int8 == 7);
}

/**
 * test_gi__simple_struct_out:
 * @struct_: (out) (transfer none):
 */
void
test_gi__simple_struct_out (TestGISimpleStruct **struct_)
{
    static TestGISimpleStruct *new_struct = NULL;

    if (new_struct == NULL) {
        new_struct = g_new(TestGISimpleStruct, 1);

        new_struct->long_ = 6;
        new_struct->int8 = 7;
    }

    *struct_ = new_struct;
}

/**
 * test_gi__simple_struct_inout:
 * @struct_: (inout) (transfer none):
 */
void
test_gi__simple_struct_inout (TestGISimpleStruct **struct_)
{
    g_assert((*struct_)->long_ == 6);
    g_assert((*struct_)->int8 == 7);

    (*struct_)->long_ = 7;
    (*struct_)->int8 = 6;
}

void
test_gi_simple_struct_method (TestGISimpleStruct *struct_)
{
    g_assert(struct_->long_ == 6);
    g_assert(struct_->int8 == 7);
}


GType
test_gi_pointer_struct_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_pointer_type_register_static ("TestGIPointerStruct");
    }

    return type;
}

/**
 * test_gi__pointer_struct_return:
 * Returns: (transfer none):
 */
TestGIPointerStruct *
test_gi__pointer_struct_return (void)
{
    static TestGIPointerStruct *struct_ = NULL;

    if (struct_ == NULL) {
        struct_ = g_new(TestGIPointerStruct, 1);

        struct_->long_ = 42;
    }

    return struct_;
}

/**
 * test_gi__pointer_struct_in:
 * @struct_: (transfer none):
 */
void
test_gi__pointer_struct_in (TestGIPointerStruct *struct_)
{
    g_assert(struct_->long_ == 42);
}

/**
 * test_gi__pointer_struct_out:
 * @struct_: (out) (transfer none):
 */
void
test_gi__pointer_struct_out (TestGIPointerStruct **struct_)
{
    static TestGIPointerStruct *new_struct = NULL;

    if (new_struct == NULL) {
        new_struct = g_new(TestGIPointerStruct, 1);

        new_struct->long_ = 42;
    }

    *struct_ = new_struct;
}

/**
 * test_gi__pointer_struct_inout:
 * @struct_: (inout) (transfer none):
 */
void
test_gi__pointer_struct_inout (TestGIPointerStruct **struct_)
{
    g_assert((*struct_)->long_ == 42);

    (*struct_)->long_ = 0;
}


TestGIBoxedStruct *
test_gi_boxed_struct_copy (TestGIBoxedStruct *struct_)
{
    TestGIBoxedStruct *new_struct;

    new_struct = g_slice_new (TestGIBoxedStruct);

    *new_struct = *struct_;

    return new_struct;
}

static void
test_gi_boxed_struct_free (TestGIBoxedStruct *struct_)
{
    g_slice_free (TestGIBoxedStruct, struct_);
}

GType
test_gi_boxed_struct_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_boxed_type_register_static ("TestGIBoxedStruct",
                (GBoxedCopyFunc) test_gi_boxed_struct_copy,
                (GBoxedFreeFunc) test_gi_boxed_struct_free);
    }

    return type;
}

TestGIBoxedStruct *
test_gi_boxed_struct_new (void)
{
    return g_slice_new (TestGIBoxedStruct);
}

/**
 * test_gi__boxed_struct_return:
 * Returns: (transfer none):
 */
TestGIBoxedStruct *
test_gi__boxed_struct_return (void)
{
    static TestGIBoxedStruct *struct_ = NULL;

    if (struct_ == NULL) {
        struct_ = g_new(TestGIBoxedStruct, 1);

        struct_->long_ = 42;
    }

    return struct_;
}

/**
 * test_gi__boxed_struct_in:
 * @struct_: (transfer none):
 */
void
test_gi__boxed_struct_in (TestGIBoxedStruct *struct_)
{
    g_assert(struct_->long_ == 42);
}

/**
 * test_gi__boxed_struct_out:
 * @struct_: (out) (transfer none):
 */
void
test_gi__boxed_struct_out (TestGIBoxedStruct **struct_)
{
    static TestGIBoxedStruct *new_struct = NULL;

    if (new_struct == NULL) {
        new_struct = g_new(TestGIBoxedStruct, 1);

        new_struct->long_ = 42;
    }

    *struct_ = new_struct;
}

/**
 * test_gi__boxed_struct_inout:
 * @struct_: (inout) (transfer none):
 */
void
test_gi__boxed_struct_inout (TestGIBoxedStruct **struct_)
{
    g_assert((*struct_)->long_ == 42);

    (*struct_)->long_ = 0;
}


enum
{
	PROP_0,
	PROP_INT_
};

G_DEFINE_TYPE (TestGIObject, test_gi_object, G_TYPE_OBJECT);

static void
test_gi_object_init (TestGIObject *object)
{
}

static void
test_gi_object_finalize (GObject *object)
{
	G_OBJECT_CLASS (test_gi_object_parent_class)->finalize (object);
}

static void
test_gi_object_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (TESTGI_IS_OBJECT (object));

	switch (prop_id) {
        case PROP_INT_:
            TESTGI_OBJECT (object)->int_ = g_value_get_int (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
	}
}

static void
test_gi_object_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (TESTGI_IS_OBJECT (object));

	switch (prop_id) {
        case PROP_INT_:
            g_value_set_int (value, TESTGI_OBJECT (object)->int_);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
	}
}

static void
test_gi_object_class_init (TestGIObjectClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
#if 0
	GObjectClass* parent_class = G_OBJECT_CLASS (klass);
#endif

	object_class->finalize = test_gi_object_finalize;
	object_class->set_property = test_gi_object_set_property;
	object_class->get_property = test_gi_object_get_property;

	g_object_class_install_property (object_class, PROP_INT_,
         g_param_spec_int ("int", "Integer", "An integer", G_MININT, G_MAXINT, 0,
              G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}


void
test_gi_object_static_method (void)
{
}

void
test_gi_object_method (TestGIObject *object)
{
	g_return_if_fail (TESTGI_IS_OBJECT (object));
    g_assert (object->int_ == 42);
}

void
test_gi_object_overridden_method (TestGIObject *object)
{
	g_return_if_fail (TESTGI_IS_OBJECT (object));
    g_assert (object->int_ == 0);
}

TestGIObject *
test_gi_object_new (gint int_)
{
    return g_object_new (TESTGI_TYPE_OBJECT, "int", int_, NULL);
}

/**
 * test_gi_object_method_array_in:
 * @ints: (array length=length):
 */
void
test_gi_object_method_array_in (TestGIObject *object, const gint *ints, gint length)
{
    g_assert(length == 4);
    g_assert(ints[0] == -1);
    g_assert(ints[1] == 0);
    g_assert(ints[2] == 1);
    g_assert(ints[3] == 2);
}

/**
 * test_gi_object_method_array_out:
 * @ints: (out) (array length=length) (transfer none):
 */
void
test_gi_object_method_array_out (TestGIObject *object, gint **ints, gint *length)
{
    static gint values[] = {-1, 0, 1, 2};

    *length = 4;
    *ints = values;
}

/**
 * test_gi_object_method_array_inout:
 * @ints: (inout) (array length=length) (transfer none):
 * @length: (inout):
 */
void
test_gi_object_method_array_inout (TestGIObject *object, gint **ints, gint *length)
{
    static gint values[] = {-2, -1, 0, 1, 2};

    g_assert(*length == 4);
    g_assert((*ints)[0] == -1);
    g_assert((*ints)[1] == 0);
    g_assert((*ints)[2] == 1);
    g_assert((*ints)[3] == 2);

    *length = 5;
    *ints = values;
}

/**
 * test_gi_object_method_array_return:
 * Returns: (array length=length):
 */
const gint *
test_gi_object_method_array_return (TestGIObject *object, gint *length)
{
    static gint ints[] = {-1, 0, 1, 2};

    *length = 4;
    return ints;
}


/**
 * test_gi__object_none_return:
 * Returns: (transfer none):
 */
TestGIObject *
test_gi__object_none_return (void)
{
    static TestGIObject *object = NULL;

    if (object == NULL) {
        object = g_object_new(TESTGI_TYPE_OBJECT, NULL);
    }

    return object;
}

/**
 * test_gi__object_full_return:
 * Returns: (transfer full):
 */
TestGIObject *
test_gi__object_full_return (void)
{
    return g_object_new(TESTGI_TYPE_OBJECT, NULL);
}

/**
 * test_gi__object_none_in:
 * @object: (transfer none):
 */
void
test_gi__object_none_in (TestGIObject *object)
{
    g_assert(object->int_ == 42);
}

/**
 * test_gi__object_full_in:
 * @object: (transfer full):
 */
void
test_gi__object_full_in (TestGIObject *object)
{
    g_assert(object->int_ == 42);
    g_object_unref(object);
}

/**
 * test_gi__object_none_out:
 * @object: (out) (transfer none):
 */
void
test_gi__object_none_out (TestGIObject **object)
{
    static TestGIObject *new_object = NULL;

    if (new_object == NULL) {
        new_object = g_object_new(TESTGI_TYPE_OBJECT, NULL);
    }

    *object = new_object;
}

/**
 * test_gi__object_full_out:
 * @object: (out) (transfer full):
 */
void
test_gi__object_full_out (TestGIObject **object)
{
    *object = g_object_new(TESTGI_TYPE_OBJECT, NULL);
}

/**
 * test_gi__object_none_inout:
 * @object: (inout) (transfer none):
 */
void
test_gi__object_none_inout (TestGIObject **object)
{
    static TestGIObject *new_object = NULL;

    g_assert((*object)->int_ == 42);

    if (new_object == NULL) {
        new_object = g_object_new(TESTGI_TYPE_OBJECT, NULL);
        new_object->int_ = 0;
    }

    *object = new_object;
}

/**
 * test_gi__object_full_inout:
 * @object: (inout) (transfer full):
 */
void
test_gi__object_full_inout (TestGIObject **object)
{
    g_assert((*object)->int_ == 42);
    g_object_unref(*object);

    *object = g_object_new(TESTGI_TYPE_OBJECT, NULL);
}

/**
 * test_gi__object_inout_same:
 * @object: (inout):
 */
void
test_gi__object_inout_same (TestGIObject **object)
{
    g_assert((*object)->int_ == 42);
    (*object)->int_ = 0;
}


G_DEFINE_TYPE (TestGISubObject, test_gi_sub_object, TESTGI_TYPE_OBJECT);

static void
test_gi_sub_object_init (TestGISubObject *object)
{
}

static void
test_gi_sub_object_finalize (GObject *object)
{
	G_OBJECT_CLASS(test_gi_sub_object_parent_class)->finalize(object);
}

static void
test_gi_sub_object_class_init (TestGISubObjectClass *klass)
{
	G_OBJECT_CLASS(klass)->finalize = test_gi_sub_object_finalize;
}

void
test_gi_sub_object_sub_method (TestGISubObject *object)
{
    g_assert(TESTGI_OBJECT(object)->int_ == 0);
}

void
test_gi_sub_object_overwritten_method (TestGISubObject *object)
{
    g_assert(TESTGI_OBJECT(object)->int_ == 0);
}

/* Interfaces */

static void
test_gi_interface_class_init(void *g_iface)
{
}

GType
test_gi_interface_get_type(void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                              "TestGIInterface",
                                              sizeof (TestGIInterfaceIface),
                                              (GClassInitFunc) test_gi_interface_class_init,
                                              0, NULL, 0);
    }

    return type;
}


/**
 * test_gi_int_out_out:
 * int0: (out):
 * int1: (out):
 */
void
test_gi_int_out_out (gint *int0, gint *int1)
{
    *int0 = 6;
    *int1 = 7;
}

/**
 * test_gi_int_return_out:
 * int_: (out):
 */
gint
test_gi_int_return_out (gint *int_)
{
    *int_ = 7;
    return 6;
}

/**
 * test_gi_ptr_return_null:
 * Returns: (allow-none):
 */
gint *
test_gi_int_return_ptr_null (void)
{
    return NULL;
}


TestGIOverridesStruct *
test_gi_overrides_struct_copy (TestGIOverridesStruct *struct_)
{
    TestGIOverridesStruct *new_struct;

    new_struct = g_slice_new (TestGIOverridesStruct);

    *new_struct = *struct_;

    return new_struct;
}

static void
test_gi_overrides_struct_free (TestGIOverridesStruct *struct_)
{
    g_slice_free (TestGIOverridesStruct, struct_);
}

GType
test_gi_overrides_struct_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_boxed_type_register_static ("TestGIOverridesStruct",
                (GBoxedCopyFunc) test_gi_overrides_struct_copy,
                (GBoxedFreeFunc) test_gi_overrides_struct_free);
    }

    return type;
}

TestGIOverridesStruct *
test_gi_overrides_struct_new (void)
{
    return g_slice_new (TestGIOverridesStruct);
}

glong
test_gi_overrides_struct_method (TestGIOverridesStruct *struct_)
{
    return 42;
}


/**
 * test_gi__overrides_struct_return:
 *
 * Returns: (transfer full):
 */
TestGIOverridesStruct *
test_gi__overrides_struct_return (void)
{
    return test_gi_overrides_struct_new();
}


G_DEFINE_TYPE (TestGIOverridesObject, test_gi_overrides_object, G_TYPE_OBJECT);

static void
test_gi_overrides_object_init (TestGIOverridesObject *object)
{
}

static void
test_gi_overrides_object_finalize (GObject *object)
{
	G_OBJECT_CLASS (test_gi_overrides_object_parent_class)->finalize (object);
}

static void
test_gi_overrides_object_class_init (TestGIOverridesObjectClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
#if 0
	GObjectClass* parent_class = G_OBJECT_CLASS (klass);
#endif

	object_class->finalize = test_gi_overrides_object_finalize;
}

TestGIOverridesObject *
test_gi_overrides_object_new (void)
{
    return g_object_new (TESTGI_TYPE_OVERRIDES_OBJECT, NULL);
}

glong
test_gi_overrides_object_method (TestGIOverridesObject *object)
{
    return 42;
}


/**
 * test_gi__overrides_object_return:
 *
 * Returns: (transfer full):
 */
TestGIOverridesObject *
test_gi__overrides_object_return (void)
{
    return g_object_new (TESTGI_TYPE_OVERRIDES_OBJECT, NULL);
}
