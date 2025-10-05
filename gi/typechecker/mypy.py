# ruff: noqa: T201
from mypy.plugin import Plugin


class GIPlugin(Plugin):
    def get_type_analyze_hook(self, fullname: str):
        # if fullname == "gi.reposi"
        if fullname.startswith("gi.repository"):
            print("get_type_analyze_hook", fullname)

    def get_function_signature_hook(self, fullname: str):
        if fullname.startswith("gi.repository."):
            print("get_function_signature_hook", fullname)

    def get_attribute_hook(self, fullname: str):
        if fullname.startswith("gi.repository."):
            print("get_attribute_hook", fullname)

    def get_class_attribute_hook(self, fullname: str):
        # if fullname.startswith("gi.repository."):
        print("get_class_attribute_hook", fullname)

    def get_dynamic_class_hook(self, fullname: str):
        # if fullname.startswith("gi.repository."):
        print("get_dynamic_class_hook", fullname)


def plugin(version: str):
    # ignore version argument if the plugin works with all mypy versions.
    return GIPlugin
