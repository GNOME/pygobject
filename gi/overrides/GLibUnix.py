# Copyright 2025 Marco Trevisan
# SPDX-License-Identifier: LGPL-2.1-or-later

from ..module import get_introspection_module
from ..overrides import deprecated_attr

GLibUnix = get_introspection_module("GLibUnix")

__all__ = []

if hasattr(GLibUnix, "signal_add"):
    signal_add_full = GLibUnix.signal_add
    __all__.append("signal_add_full")
    deprecated_attr("GLibUnix", "signal_add_full", "GLibUnix.signal_add")
