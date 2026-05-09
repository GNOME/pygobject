/*
 * Copyright (C) 2020 Red Hat, Inc.
 * Copyright (C) 2026 Arjan Molenaar <amolenaar@gnome.org>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <glib.h>

/* Keep in sync with DebugFlags in _debug.py */
typedef enum {
    PYGI_DEBUG_MARSHALLER = 1 << 0,
    PYGI_DEBUG_LIFECYCLE = 1 << 1,
} PyGIDebugFlags;

#define PYGI_DEBUG_CHECK(type)                                                \
    G_UNLIKELY (pygi_get_debug_flags () & PYGI_DEBUG_##type)

#define PYGI_DEBUG(type, ...)                                                 \
    G_STMT_START                                                              \
    {                                                                         \
        if (PYGI_DEBUG_CHECK (type)) pygi_debug_message (#type, __VA_ARGS__); \
    }                                                                         \
    G_STMT_END

PyGIDebugFlags pygi_get_debug_flags (void);
void pygi_set_debug_flags (PyGIDebugFlags flags);

static inline void pygi_debug_message (const char *type, const char *format,
                                       ...) G_GNUC_PRINTF (2, 3);
static inline void
pygi_debug_message (const char *type, const char *format, ...)
{
    va_list args;

    fprintf (stderr, "PYGI(%s): ", type);

    va_start (args, format);
#ifdef GLIB_USING_SYSTEM_PRINTF
    vfprintf (stderr, format, args);
#else
    g_vfprintf (stderr, format, args);
#endif
    va_end (args);

    fprintf (stderr, "\n");
}
