===========
Basic Types
===========

PyGObject will automatically convert between C types and Python types. In
cases where it's appropriate it will use default Python types like :obj:`int`,
:obj:`list`, and :obj:`dict`.


Number Types
------------

All glib integer types get mapped to :obj:`int` and :obj:`float`.
Since the glib integer types are always range limited, conversions from Python
int/long can fail with :class:`OverflowError`:

.. code:: pycon

    >>> GLib.random_int_range(0, 2**31-1)
    1684142898
    >>> GLib.random_int_range(0, 2**31)
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    OverflowError: 2147483648 not in range -2147483648 to 2147483647
    >>>


Text Types
----------

Text types are mapped to :obj:`str`.


Other Types
-----------

* GList <-> :obj:`list`
* GSList <-> :obj:`list`
* GHashTable <-> :obj:`dict`
* arrays <-> :obj:`list`
