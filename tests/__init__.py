import os
import sys
import signal
import subprocess
import atexit
import warnings


def set_dll_search_path():
    # Python 3.8 no longer searches for DLLs in PATH, so we have to add
    # everything in PATH manually. Note that unlike PATH add_dll_directory
    # has no defined order, so if there are two cairo DLLs in PATH we
    # might get a random one.
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return
    for p in os.environ.get("PATH", "").split(os.pathsep):
        try:
            os.add_dll_directory(p)
        except OSError:
            pass


def init_test_environ():

    set_dll_search_path()

    def dbus_launch_session():
        if os.name == "nt" or sys.platform == "darwin":
            return (-1, "")

        try:
            out = subprocess.check_output([
                "dbus-daemon", "--session", "--fork", "--print-address=1",
                "--print-pid=1"])
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

    tests_builddir = os.path.abspath(os.environ.get('TESTS_BUILDDIR', os.path.dirname(__file__)))
    builddir = os.path.dirname(tests_builddir)
    tests_srcdir = os.path.abspath(os.path.dirname(__file__))
    srcdir = os.path.dirname(tests_srcdir)

    sys.path.insert(0, os.path.join(builddir, 'gi'))
    sys.path.insert(0, tests_srcdir)
    sys.path.insert(0, srcdir)
    sys.path.insert(0, tests_builddir)
    sys.path.insert(0, builddir)

    # force untranslated messages, as we check for them in some tests
    os.environ['LC_MESSAGES'] = 'C'
    os.environ['G_DEBUG'] = 'fatal-warnings fatal-criticals'
    if sys.platform == "darwin" or os.name == "nt":
        # gtk 3.22 has warnings and ciriticals on OS X, ignore for now.
        # On Windows glib will create an error dialog which will block tests
        # so it's never a good idea there to make things fatal.
        os.environ['G_DEBUG'] = ''

    # make Gio able to find our gschemas.compiled in tests/. This needs to be set
    # before importing Gio. Support a separate build tree, so look in build dir
    # first.
    os.environ['GSETTINGS_BACKEND'] = 'memory'
    os.environ['GSETTINGS_SCHEMA_DIR'] = tests_builddir
    os.environ['G_FILENAME_ENCODING'] = 'UTF-8'

    # Avoid accessibility dbus warnings
    os.environ['NO_AT_BRIDGE'] = '1'

    # A workaround for https://gitlab.gnome.org/GNOME/glib/-/issues/2251
    # The gtk4 a11y stack calls get_dbus_object_path() on the default app
    os.environ['GTK_A11Y'] = 'none'

    # Force the default theme so broken themes don't affect the tests
    os.environ['GTK_THEME'] = 'Adwaita'

    import gi
    gi.require_version("GIRepository", "2.0")
    from gi.repository import GIRepository
    repo = GIRepository.Repository.get_default()
    repo.prepend_library_path(os.path.join(tests_builddir))
    repo.prepend_search_path(tests_builddir)

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
    gi.require_versions({
        "GIMarshallingTests": "1.0",
        "Regress": "1.0",
        "GLib": "2.0",
        "Gio": "2.0",
        "GObject": "2.0",
    })

    # It's disabled for stable releases by default, this makes sure it's
    # always on for the tests.
    warnings.simplefilter('default', gi.PyGIDeprecationWarning)

    # Otherwise we crash on the first gtk use when e.g. DISPLAY isn't set
    try:
        from gi.repository import Gtk
    except ImportError:
        pass
    else:
        if Gtk._version == "4.0":
            res = Gtk.init_check()
        else:
            res = Gtk.init_check([])[0]
        if not res:
            raise RuntimeError("Gtk available, but Gtk.init_check() failed")


init_test_environ()

__path__ = __import__('pkgutil').extend_path(__path__, __name__)
