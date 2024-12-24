import os
import sys
import signal
import subprocess
import atexit
import warnings

import pytest
import contextlib


@pytest.hookimpl(hookwrapper=True, tryfirst=True)
def pytest_runtest_call(item):
    """A pytest hook which takes over sys.excepthook and raises any uncaught
    exception (with PyGObject this happesn often when we get called from C,
    like any signal handler, vfuncs tc).
    """
    exceptions = []

    def on_hook(type_, value, tback):
        exceptions.append((type_, value, tback))

    orig_excepthook = sys.excepthook
    sys.excepthook = on_hook
    try:
        outcome = yield
    finally:
        sys.excepthook = orig_excepthook
        if exceptions:
            tp, value, tb = exceptions[0]
            outcome.force_exception(tp(value).with_traceback(tb))


def set_dll_search_path():
    # Python 3.8 no longer searches for DLLs in PATH, so we have to add
    # everything in PATH manually. Note that unlike PATH add_dll_directory
    # has no defined order, so if there are two cairo DLLs in PATH we
    # might get a random one.
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return
    for p in os.environ.get("PATH", "").split(os.pathsep):
        with contextlib.suppress(OSError):
            os.add_dll_directory(p)


def os_environ_prepend(envvar, path):
    current = os.environ.get(envvar)
    os.environ[envvar] = os.path.pathsep.join([path, current] if current else [path])


def init_test_environ():
    set_dll_search_path()

    def dbus_launch_session():
        if os.name == "nt" or sys.platform == "darwin":
            return (-1, "")

        try:
            out = subprocess.check_output(
                [
                    "dbus-daemon",
                    "--session",
                    "--fork",
                    "--print-address=1",
                    "--print-pid=1",
                ]
            )
        except (subprocess.CalledProcessError, OSError):
            return (-1, "")
        else:
            out = out.decode("utf-8")
            addr, pid = out.splitlines()
            return int(pid), addr

    pid, addr = dbus_launch_session()
    if pid >= 0:
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = addr
        atexit.register(os.kill, pid, signal.SIGKILL)
    else:
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = "."

    # force untranslated messages, as we check for them in some tests
    os.environ["LC_MESSAGES"] = "C"
    os.environ["G_DEBUG"] = "fatal-warnings fatal-criticals"
    if sys.platform == "darwin" or os.name == "nt":
        # gtk 3.22 has warnings and ciriticals on OS X, ignore for now.
        # On Windows glib will create an error dialog which will block tests
        # so it's never a good idea there to make things fatal.
        os.environ["G_DEBUG"] = ""

    # First add test directory, since we have a gi package there
    tests_srcdir = os.path.abspath(os.path.dirname(__file__))
    srcdir = os.path.dirname(tests_srcdir)

    sys.path.insert(0, tests_srcdir)
    sys.path.insert(0, srcdir)

    import gi

    gi_builddir = os.path.dirname(gi._gi.__file__)
    builddir = os.path.dirname(gi_builddir)
    tests_builddir = os.path.join(builddir, "tests")

    sys.path.insert(0, tests_builddir)
    sys.path.insert(0, builddir)

    # make Gio able to find our gschemas.compiled in tests/. This needs to be set
    # before importing Gio. Support a separate build tree, so look in build dir
    # first.
    os.environ["GSETTINGS_BACKEND"] = "memory"
    os.environ["GSETTINGS_SCHEMA_DIR"] = tests_builddir
    os.environ["G_FILENAME_ENCODING"] = "UTF-8"

    # Avoid accessibility dbus warnings
    os.environ["NO_AT_BRIDGE"] = "1"

    # A workaround for https://gitlab.gnome.org/GNOME/glib/-/issues/2251
    # The gtk4 a11y stack calls get_dbus_object_path() on the default app
    os.environ["GTK_A11Y"] = "none"

    # Force the default theme so broken themes don't affect the tests
    os.environ["GTK_THEME"] = "Adwaita"

    gi_gir_path = os.path.join(
        builddir, "subprojects", "glib", "girepository", "introspection"
    )
    if os.path.exists(gi_gir_path):
        os_environ_prepend("GI_TYPELIB_PATH", gi_gir_path)

    gi.require_version("GIRepository", "3.0")
    repo = gi.Repository.get_default()

    gi_tests_path = os.path.join(builddir, "subprojects", "gobject-introspection-tests")
    repo.prepend_library_path(gi_tests_path)
    repo.prepend_search_path(gi_tests_path)

    def try_require_version(namespace, version):
        try:
            gi.require_version(namespace, version)
        except ValueError:
            # prevent tests from running with the wrong version
            sys.modules["gi.repository." + namespace] = None

    # Optional
    try_require_version("Gtk", os.environ.get("TEST_GTK_VERSION", "3.0"))
    try_require_version("Gdk", os.environ.get("TEST_GTK_VERSION", "3.0"))
    try_require_version("GdkPixbuf", "2.0")
    try_require_version("Pango", "1.0")
    try_require_version("PangoCairo", "1.0")
    try_require_version("Atk", "1.0")

    # Required
    gi.require_versions(
        {
            "GIMarshallingTests": "1.0",
            "Regress": "1.0",
            "GLib": "2.0",
            "Gio": "2.0",
            "GObject": "2.0",
        }
    )

    # It's disabled for stable releases by default, this makes sure it's
    # always on for the tests.
    warnings.simplefilter("default", gi.PyGIDeprecationWarning)

    # Otherwise we crash on the first gtk use when e.g. DISPLAY isn't set
    try:
        from gi.repository import Gtk
    except ImportError:
        pass
    else:
        res = Gtk.init_check() if Gtk._version == "4.0" else Gtk.init_check([])[0]
        if not res:
            raise RuntimeError("Gtk available, but Gtk.init_check() failed")


init_test_environ()
