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

from importlib import import_module
from inspect import Parameter, Signature
from typing import Any, Callable, Optional

from ._error import GError
from ._gi import (
    VFuncInfo,
    FunctionInfo,
    CallableInfo,
    CallbackInfo,
    Direction,
    GType,
    TypeInfo,
    TypeTag,
)

__all__ = ["generate_signature"]


tag_pytype = {
    TypeTag.BOOLEAN: bool,
    TypeTag.INT8: int,
    TypeTag.UINT8: int,
    TypeTag.INT16: int,
    TypeTag.UINT16: int,
    TypeTag.INT32: int,
    TypeTag.UINT32: int,
    TypeTag.INT64: int,
    TypeTag.UINT64: int,
    TypeTag.FLOAT: float,
    TypeTag.DOUBLE: float,
    TypeTag.GTYPE: GType,
    TypeTag.UTF8: str,
    TypeTag.FILENAME: str,
    TypeTag.UNICHAR: str,
    TypeTag.ERROR: GError,
}

list_tag_types = {TypeTag.GLIST, TypeTag.GSLIST, TypeTag.ARRAY}


def get_pytype(gi_type: TypeInfo) -> object:
    tag = gi_type.get_tag()
    if pytype := tag_pytype.get(tag):
        return pytype
    if tag == TypeTag.VOID:
        if gi_type.is_pointer():
            return Any
        return None
    if tag in list_tag_types:
        value_type = get_pytype(gi_type.get_param_type(0))
        if value_type is Parameter.empty:
            return list
        return list[value_type]
    if tag == TypeTag.GHASH:
        key_type = get_pytype(gi_type.get_param_type(0))
        value_type = get_pytype(gi_type.get_param_type(1))
        if key_type is Parameter.empty or value_type is Parameter.empty:
            return dict
        return dict[key_type, value_type]
    if tag == TypeTag.INTERFACE:
        info = gi_type.get_interface()
        if isinstance(info, CallbackInfo):
            sig = generate_signature(info)
            return Callable[
                [param.annotation for param in sig.parameters.values()],
                sig.return_annotation,
            ]
        info_name = info.get_name()
        if not info_name:
            return gi_type.get_tag_as_string()
        info_namespace = info.get_namespace()
        module = import_module(f"gi.repository.{info_namespace}")
        try:
            return getattr(module, info_name)
        except NotImplementedError:
            return f"{info_namespace}.{info_name}"
    else:
        return gi_type.get_tag_as_string()


def generate_signature(info: CallableInfo) -> Signature:
    args = info.get_arguments()
    arg_names = {arg.get_name() for arg in args}

    def unique(name):
        while name in arg_names:
            name += "_"
        arg_names.add(name)
        return name

    params = []
    if isinstance(info, FunctionInfo):
        if info.is_constructor():
            params.append(Parameter(unique("cls"), Parameter.POSITIONAL_OR_KEYWORD))
        elif info.is_method():
            params.append(Parameter(unique("self"), Parameter.POSITIONAL_OR_KEYWORD))
    elif isinstance(info, VFuncInfo):
        params.append(Parameter(unique("type"), Parameter.POSITIONAL_OR_KEYWORD))
        params.append(Parameter(unique("self"), Parameter.POSITIONAL_OR_KEYWORD))

    # Build lists of indices prior to adding the docs because it is possible
    # the index retrieved comes before input arguments being used.
    ignore_indices = {info.get_return_type().get_array_length_index()}
    user_data_indices = set()
    for arg in args:
        ignore_indices.add(arg.get_destroy_index())
        ignore_indices.add(arg.get_type_info().get_array_length_index())
        user_data_indices.add(arg.get_closure_index())

    for i, arg in enumerate(args):
        if arg.get_direction() == Direction.OUT:
            continue  # skip exclusively output args
        if i in ignore_indices:
            continue

        default = Parameter.empty
        annotation = get_pytype(arg.get_type_info())
        if arg.may_be_null() or i in user_data_indices:
            # allow-none or user_data from a closure
            default = None
            if annotation is not Parameter.empty and annotation is not Any:
                annotation = Optional[annotation]
        elif arg.is_optional():
            # TODO: Can we retrieve the default value?
            default = ...

        params.append(
            Parameter(
                arg.get_name(),
                Parameter.POSITIONAL_OR_KEYWORD,
                default=default,
                annotation=annotation,
            )
        )

    # Remove defaults from params after the last required parameter.
    last_required = max(
        (i for (i, param) in enumerate(params) if param.default is Parameter.empty),
        default=0,
    )
    for i, param in enumerate(params):
        if i >= last_required:
            break
        if param.default is not Parameter.empty:
            params[i] = param.replace(default=Parameter.empty)

    return_annotation = get_pytype(info.get_return_type())
    if info.may_return_null():
        return_annotation = Optional[return_annotation]

    out_args = []
    for i, arg in enumerate(args):
        if arg.get_direction() == Direction.IN:
            continue  # skip exclusively input args
        if i in ignore_indices:
            continue
        annotation = get_pytype(arg.get_type_info())
        if arg.may_be_null() and annotation is not Parameter.empty:
            annotation = Optional[annotation]
        out_args.append(annotation)

    if return_annotation is not None:
        out_args.insert(0, return_annotation)
    if len(out_args) > 1:
        return_annotation = tuple[tuple(out_args)]
    elif len(out_args) == 1:
        return_annotation = out_args[0]

    return Signature(params, return_annotation=return_annotation)
