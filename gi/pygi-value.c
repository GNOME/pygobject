
/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "pygi-value.h"


GIArgument
_pygi_argument_from_g_value(const GValue *value,
                            GITypeInfo *type_info)
{
    GIArgument arg = { 0, };

    GITypeTag type_tag = g_type_info_get_tag (type_info);

    /* For the long handling: long can be equivalent to
       int32 or int64, depending on the architecture, but
       gi doesn't tell us (and same for ulong)
    */
    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            arg.v_boolean = g_value_get_boolean (value);
            break;
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_INT32:
	    if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_LONG))
		arg.v_int = g_value_get_long (value);
	    else
		arg.v_int = g_value_get_int (value);
            break;
        case GI_TYPE_TAG_INT64:
	    if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_LONG))
		arg.v_int64 = g_value_get_long (value);
	    else
		arg.v_int64 = g_value_get_int64 (value);
            break;
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_UINT32:
	    if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_ULONG))
		arg.v_uint = g_value_get_ulong (value);
	    else
		arg.v_uint = g_value_get_uint (value);
            break;
        case GI_TYPE_TAG_UINT64:
	    if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_ULONG))
		arg.v_uint64 = g_value_get_ulong (value);
	    else
		arg.v_uint64 = g_value_get_uint64 (value);
            break;
        case GI_TYPE_TAG_UNICHAR:
            arg.v_uint32 = g_value_get_schar (value);
            break;
        case GI_TYPE_TAG_FLOAT:
            arg.v_float = g_value_get_float (value);
            break;
        case GI_TYPE_TAG_DOUBLE:
            arg.v_double = g_value_get_double (value);
            break;
        case GI_TYPE_TAG_GTYPE:
            arg.v_long = g_value_get_gtype (value);
            break;
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            arg.v_string = g_value_dup_string (value);
            break;
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
            arg.v_pointer = g_value_get_pointer (value);
            break;
        case GI_TYPE_TAG_ARRAY:
        case GI_TYPE_TAG_GHASH:
            if (G_VALUE_HOLDS_BOXED (value))
                arg.v_pointer = g_value_get_boxed (value);
            else
                /* e. g. GSettings::change-event */
                arg.v_pointer = g_value_get_pointer (value);
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface (type_info);
            info_type = g_base_info_get_type (info);

            g_base_info_unref (info);

            switch (info_type) {
                case GI_INFO_TYPE_FLAGS:
                    arg.v_uint = g_value_get_flags (value);
                    break;
                case GI_INFO_TYPE_ENUM:
                    arg.v_int = g_value_get_enum (value);
                    break;
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    if (G_VALUE_HOLDS_PARAM (value))
                      arg.v_pointer = g_value_get_param (value);
                    else
                      arg.v_pointer = g_value_get_object (value);
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                    if (G_VALUE_HOLDS(value, G_TYPE_BOXED)) {
                        arg.v_pointer = g_value_get_boxed (value);
                    } else if (G_VALUE_HOLDS(value, G_TYPE_VARIANT)) {
                        arg.v_pointer = g_value_get_variant (value);
                    } else {
                        arg.v_pointer = g_value_get_pointer (value);
                    }
                    break;
                default:
                    g_warning("Converting of type '%s' is not implemented", g_info_type_to_string(info_type));
                    g_assert_not_reached();
            }
            break;
        }
        case GI_TYPE_TAG_ERROR:
            arg.v_pointer = g_value_get_boxed (value);
            break;
        case GI_TYPE_TAG_VOID:
            arg.v_pointer = g_value_get_pointer (value);
            break;
    }

    return arg;
}
