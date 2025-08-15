==========================
Python Override Guidelines
==========================

This document serves as a guide for developers creating new PyGObject
overrides or modifying existing ones. This document is not intended as hard
rules as there may always be pragmatic exceptions to what is listed here. It
is also a good idea to study the Zen of Python by Tim Peters in  :pep:`20`.

In general, overrides should be minimized and preference should always be
placed on updating the underlying API to be more bindable, adding features to
GI to support the requirement, or adding mechanical features to PyGObject
which can apply generically to all overrides (:bzbug:`721226` and
:bzbug:`640812`).

If a GI feature or more bindable API for a library is in the works, it is a
good idea to avoid the temptation to add temporary short term workarounds in
overrides. The reason is this can creaste unnecessary conflicts when the
bindable API becomes a reality (:bzbug:`707280`).

* Minimize class overrides when possible.

  *Reason*: Class overrides incur a load time performance penalty because
  they require the classes GType and all of the Python method bindings to be
  created. See :bzbug:`705810`

* Prefer monkey patching methods on repository classes over inheritance.

  *Reason*: Class overrides add an additional level to the method
  resolution order (mro) which has a performance penalty. Since overrides are
  designed for specific repository library APIs, monkey patching is
  reasonable because it is utilized in a controlled manner by the API
  designer (as opposed to monkey patching a third-party library which is more
  fragile).

* Avoid overriding ``__init__``
  *Reason*: Sub-classing the overridden class then becomes challenging and
  has the potential to cause bugs (see :bzbug:`711487` and reasoning
  listed in :doc:`initializer_deprecations`).

* Unbindable functions which take variadic arguments are generally ok to add
  Python implementations, but keep in mind the prior noted guidelines. A lot
  of times adding bindable versions of the functions to the underlying library
  which take a list is acceptable. For example: :bzbug:`706119`. Another
  problem here is if an override is added, then later a bindable version of
  the API is added which takes a list, there is a good chance we have to live
  with the override forever which masks a working version implemented by GI.

* Avoid side effects beyond the intended repositories API in function/method
  overrides.

  *Reason*: This conflates the original API and adds a documentation burden
  on the override maintainer.

* Don't change function signatures from the original API and don't add default
  values.

  *Reason*: This turns into a documentation discrepancy between the libraries
  API and the Python version of the API. Default value work should focus on
  bug :bzbug:`558620`, not cherry-picking individual Python functions and
  adding defaults.

* Avoid implicit side effects to the Python standard library (or anywhere).

  * Don't modify or use sys.argv

    *Reason*: sys.argv should only be explicitly controlled by application
    developers. Otherwise it requires hacks to work around a module modifying
    or using the developers command line args which they rightfully own.

    .. code:: python

        saved_argv = sys.argv.copy()
        sys.argv = []
        from gi.repository import Gtk
        sys.argv = saved_argv

  * Never set Pythons default encoding.

    *Reason*: Read or watch Ned Batchelders "`Pragmatic Unicode
    <https://nedbatchelder.com/text/unipain.html>`__"

* Prefer adapter patterns over of inheritance and overrides.

  *Reason*: An adapter allows more flexibility and less dependency on
  overrides. It allows application developers to use the raw GI API without
  having to think about if a particular typelibs overrides have been installed
  or not.
