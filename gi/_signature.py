# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2024 James Henstridge <james@jamesh.id.au>
#
#   _signature.py: callable signature generator for gi.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

from inspect import Parameter, Signature

from ._gi import VFuncInfo, FunctionInfo, CallableInfo, Direction

__all__ = ["generate_signature"]


def generate_signature(info: CallableInfo) -> Signature:
    params = []
    if isinstance(info, VFuncInfo) or (
        isinstance(info, FunctionInfo) and info.is_method()
    ):
        params.append(Parameter("self", Parameter.POSITIONAL_OR_KEYWORD))

    args = info.get_arguments()

    # Build lists of indices prior to adding the docs because it is possible
    # the index retrieved comes before input arguments being used.
    ignore_indices = {info.get_return_type().get_array_length()}
    user_data_indices = set()
    for arg in args:
        ignore_indices.add(arg.get_destroy())
        ignore_indices.add(arg.get_type().get_array_length())
        user_data_indices.add(arg.get_closure())

    for i, arg in enumerate(args):
        if arg.get_direction() == Direction.OUT:
            continue  # skip exclusively output args
        if i in ignore_indices:
            continue

        default = Parameter.empty
        if arg.may_be_null() or i in user_data_indices:
            # allow-none or user_data from a closure
            default = None
        elif arg.is_optional():
            # TODO: Can we retrieve the default value?
            default = ...

        params.append(
            Parameter(arg.get_name(), Parameter.POSITIONAL_OR_KEYWORD, default=default)
        )

    return Signature(params)
