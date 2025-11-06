/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
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

#include "pygi-info.h"
#include "pygi-type.h"
#include "pygi-cache-private.h"

static void
_interface_cache_free_func (PyGIInterfaceCache *cache)
{
    if (cache != NULL) {
        Py_XDECREF (cache->py_type);
        if (cache->type_name != NULL) g_free (cache->type_name);
        if (cache->interface_info != NULL)
            gi_base_info_unref ((GIBaseInfo *)cache->interface_info);
        g_slice_free (PyGIInterfaceCache, cache);
    }
}

/* pygi_arg_interface_setup:
 * arg_cache: argument cache to initialize
 * type_info: source for type related attributes to cache
 * arg_info: (allow-none): source for argument related attributes to cache
 * transfer: transfer mode to store in the argument cache
 * direction: marshaling direction to store in the cache
 * iface_info: interface info to cache
 *
 * Initializer for PyGIInterfaceCache
 *
 * Returns: TRUE on success and FALSE on failure
 */
static gboolean
pygi_arg_interface_setup (
    PyGIInterfaceCache *iface_cache, GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction,
    GIRegisteredTypeInfo *iface_info)
{
    GIBaseInfo *base_info = GI_BASE_INFO (iface_info);

    pygi_arg_base_setup ((PyGIArgCache *)iface_cache, type_info, arg_info,
                         transfer, direction);

    ((PyGIArgCache *)iface_cache)->destroy_notify =
        (GDestroyNotify)_interface_cache_free_func;

    gi_base_info_ref (base_info);
    iface_cache->interface_info = iface_info;
    iface_cache->arg_cache.type_tag = GI_TYPE_TAG_INTERFACE;
    iface_cache->type_name = _pygi_gi_base_info_get_fullname (base_info);
    iface_cache->g_type = gi_registered_type_info_get_g_type (iface_info);
    iface_cache->py_type = pygi_type_import_by_gi_info (base_info);

    if (g_type_is_a (iface_cache->g_type, G_TYPE_OBJECT)) {
        if (g_str_equal (g_type_name (iface_cache->g_type), "GCancellable"))
            iface_cache->arg_cache.async_context =
                PYGI_ASYNC_CONTEXT_CANCELLABLE;
    }

    if (iface_cache->py_type == NULL) {
        return FALSE;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_interface_new_from_info (
    GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction,
    GIRegisteredTypeInfo *iface_info)
{
    PyGIInterfaceCache *ic;

    ic = g_slice_new0 (PyGIInterfaceCache);
    if (!pygi_arg_interface_setup (ic, type_info, arg_info, transfer,
                                   direction, iface_info)) {
        pygi_arg_cache_free ((PyGIArgCache *)ic);
        return NULL;
    }

    return (PyGIArgCache *)ic;
}
