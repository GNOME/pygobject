/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2013 Simon Feltman <sfeltman@gnome.org>
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

#ifndef __PYGI_HASHTABLE_H__
#define __PYGI_HASHTABLE_H__

#include <girepository.h>
#include "pygi-cache.h"

G_BEGIN_DECLS

PyGIArgCache *pygi_arg_hash_table_new_from_info (GITypeInfo         *type_info,
                                                 GIArgInfo          *arg_info,   /* may be null */
                                                 GITransfer          transfer,
                                                 PyGIDirection       direction,
                                                 PyGICallableCache  *callable_cache);

G_END_DECLS

#endif /*__PYGI_HASHTABLE_H__*/
