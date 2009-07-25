/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 */

#include "pygi-private.h"

GArray *
_g_array_values(GArray *array)
{
	GArray *values;
	gsize element_size;

	element_size = g_array_get_element_size(array);

	values = g_array_sized_new(FALSE, FALSE, element_size, array->len);

	if (values == NULL) {
		return NULL;
	}

	g_array_insert_vals(values, 0, array->data, array->len);

	return values;
}
