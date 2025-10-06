# ruff: noqa: T201
from mypy.plugin import Plugin
from mypy.nodes import SymbolTableNode, MypyFile, ImportFrom


class GIPlugin(Plugin):
    def lookup_fully_qualified(self, fullname: str) -> SymbolTableNode | None:
        print("lookup_fully_qualified", fullname)
        return super().lookup_fully_qualified(fullname)

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

    def get_additional_deps(self, file: MypyFile) -> list[tuple[int, str, int]]:
        """Tell mypy about gi.repository.* modules that are dynamically imported.

        Returns a list of `(priority, module_name, line)` tuples representing additional
        dependencies that mypy should be aware of.
        """
        deps = []
        gi_imports = _extract_gi_repository_imports(file)

        if gi_imports:
            print(f"Found gi.repository imports: {gi_imports}")

            # Add dependencies for each imported namespace that exists
            for namespace in gi_imports:
                module_name = f"gi.repository.{namespace}"
                # Priority 10 (normal), line 1 (beginning of file)
                deps.append((10, module_name, 1))
                print(f"Added dependency: {module_name}")

        return deps

    # def get_class_attribute_hook(self, fullname: str):
    #     # if fullname.startswith("gi.repository."):
    #     print("get_class_attribute_hook", fullname)

    # def get_dynamic_class_hook(self, fullname: str):
    #     # if fullname.startswith("gi.repository."):
    #     print("get_dynamic_class_hook", fullname)


def _extract_gi_repository_imports(file: MypyFile) -> set[str]:
    """Extract gi.repository.* imports from the file."""
    gi_imports = set()

    for stmt in file.defs:
        if isinstance(stmt, ImportFrom):
            # Handle: from gi.repository import Gtk, GLib
            if stmt.id == "gi.repository":
                if stmt.names:
                    for name, _ in stmt.names:
                        gi_imports.add(name)

            # Handle: from gi.repository.Gtk import Window
            elif stmt.id and stmt.id.startswith("gi.repository."):
                namespace = stmt.id.split(".", 2)[
                    2
                ]  # Extract namespace from gi.repository.Namespace
                gi_imports.add(namespace)

    return gi_imports


def plugin(version: str):
    # ignore version argument if the plugin works with all mypy versions.
    return GIPlugin
