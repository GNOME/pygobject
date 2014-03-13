# -*- Mode: Python; py-indent-offset: 4 -*-
# test_generictreemodel - Tests for GenericTreeModel
# Copyright (C) 2013 Simon Feltman
#
#   test_generictreemodel.py: Tests for GenericTreeModel
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
# License along with this library; if not, see <http://www.gnu.org/licenses/>.


# system
import gc
import sys
import weakref
import unittest

# pygobject
from gi.repository import GObject

try:
    from gi.repository import Gtk
    from pygtkcompat.generictreemodel import GenericTreeModel
    from pygtkcompat.generictreemodel import _get_user_data_as_pyobject
    has_gtk = True
except ImportError:
    GenericTreeModel = object
    has_gtk = False


class Node(object):
    """Represents a generic node with name, value, and children."""
    def __init__(self, name, value, *children):
        self.name = name
        self.value = value
        self.children = list(children)
        self.parent = None
        self.next = None

        for i, child in enumerate(children):
            child.parent = weakref.ref(self)
            if i < len(children) - 1:
                child.next = weakref.ref(children[i + 1])

    def __repr__(self):
        return 'Node("%s", %s)' % (self.name, self.value)


class TesterModel(GenericTreeModel):
    def __init__(self):
        super(TesterModel, self).__init__()
        self.root = Node('root', 0,
                         Node('spam', 1,
                              Node('sushi', 2),
                              Node('bread', 3)
                         ),
                         Node('eggs', 4)
                        )

    def on_get_flags(self):
        return 0

    def on_get_n_columns(self):
        return 2

    def on_get_column_type(self, n):
        return (str, int)[n]

    def on_get_iter(self, path):
        node = self.root
        path = list(path)
        idx = path.pop(0)
        while path:
            idx = path.pop(0)
            node = node.children[idx]
        return node

    def on_get_path(self, node):
        def rec_get_path(n):
            for i, child in enumerate(n.children):
                if child == node:
                    return [i]
                else:
                    res = rec_get_path(child)
                    if res:
                        res.insert(0, i)

        return rec_get_path(self.root)

    def on_get_value(self, node, column):
        if column == 0:
            return node.name
        elif column == 1:
            return node.value

    def on_iter_has_child(self, node):
        return bool(node.children)

    def on_iter_next(self, node):
        if node.next:
            return node.next()

    def on_iter_children(self, node):
        if node:
            return node.children[0]
        else:
            return self.root

    def on_iter_n_children(self, node):
        if node is None:
            return 1
        return len(node.children)

    def on_iter_nth_child(self, node, n):
        if node is None:
            assert n == 0
            return self.root
        return node.children[n]

    def on_iter_parent(self, child):
        if child.parent:
            return child.parent()


@unittest.skipUnless(has_gtk, 'Gtk not available')
class TestReferences(unittest.TestCase):
    def setUp(self):
        pass

    def test_c_tree_iter_user_data_as_pyobject(self):
        obj = object()
        obj_id = id(obj)
        ref_count = sys.getrefcount(obj)

        # This is essentially a stolen ref in the context of _CTreeIter.get_user_data_as_pyobject
        it = Gtk.TreeIter()
        it.user_data = obj_id

        obj2 = _get_user_data_as_pyobject(it)
        self.assertEqual(obj, obj2)
        self.assertEqual(sys.getrefcount(obj), ref_count + 1)

    def test_leak_references_on(self):
        model = TesterModel()
        obj_ref = weakref.ref(model.root)
        # Initial refcount is 1 for model.root + the temporary
        self.assertEqual(sys.getrefcount(model.root), 2)

        # Iter increases by 1 do to assignment to iter.user_data
        res, it = model.do_get_iter([0])
        self.assertEqual(id(model.root), it.user_data)
        self.assertEqual(sys.getrefcount(model.root), 3)

        # Verify getting a TreeIter more then once does not further increase
        # the ref count.
        res2, it2 = model.do_get_iter([0])
        self.assertEqual(id(model.root), it2.user_data)
        self.assertEqual(sys.getrefcount(model.root), 3)

        # Deleting the iter does not decrease refcount because references
        # leak by default (they are stored in the held_refs pool)
        del it
        gc.collect()
        self.assertEqual(sys.getrefcount(model.root), 3)

        # Deleting a model should free all held references to user data
        # stored by TreeIters
        del model
        gc.collect()
        self.assertEqual(obj_ref(), None)

    def test_row_deleted_frees_refs(self):
        model = TesterModel()
        obj_ref = weakref.ref(model.root)
        # Initial refcount is 1 for model.root + the temporary
        self.assertEqual(sys.getrefcount(model.root), 2)

        # Iter increases by 1 do to assignment to iter.user_data
        res, it = model.do_get_iter([0])
        self.assertEqual(id(model.root), it.user_data)
        self.assertEqual(sys.getrefcount(model.root), 3)

        # Notifying the underlying model of a row_deleted should decrease the
        # ref count.
        model.row_deleted(Gtk.TreePath('0'), model.root)
        self.assertEqual(sys.getrefcount(model.root), 2)

        # Finally deleting the actual object should collect it completely
        del model.root
        gc.collect()
        self.assertEqual(obj_ref(), None)

    def test_leak_references_off(self):
        model = TesterModel()
        model.leak_references = False

        obj_ref = weakref.ref(model.root)
        # Initial refcount is 1 for model.root + the temporary
        self.assertEqual(sys.getrefcount(model.root), 2)

        # Iter does not increas count by 1 when leak_references is false
        res, it = model.do_get_iter([0])
        self.assertEqual(id(model.root), it.user_data)
        self.assertEqual(sys.getrefcount(model.root), 2)

        # Deleting the iter does not decrease refcount because assigning user_data
        # eats references and does not release them.
        del it
        gc.collect()
        self.assertEqual(sys.getrefcount(model.root), 2)

        # Deleting the model decreases the final ref, and the object is collected
        del model
        gc.collect()
        self.assertEqual(obj_ref(), None)

    def test_iteration_refs(self):
        # Pull iterators off the model using the wrapped C API which will
        # then call back into the python overrides.
        model = TesterModel()
        nodes = [node for node in model.iter_depth_first()]
        values = [node.value for node in nodes]

        # Verify depth first ordering
        self.assertEqual(values, [0, 1, 2, 3, 4])

        # Verify ref counts for each of the nodes.
        # 5 refs for each node at this point:
        #   1 - ref held in getrefcount function
        #   2 - ref held by "node" var during iteration
        #   3 - ref held by local "nodes" var
        #   4 - ref held by the root/children graph itself
        #   5 - ref held by the model "held_refs" instance var
        for node in nodes:
            self.assertEqual(sys.getrefcount(node), 5)

        # A second iteration and storage of the nodes in a new list
        # should only increase refcounts by 1 even though new
        # iterators are created and assigned.
        nodes2 = [node for node in model.iter_depth_first()]
        for node in nodes2:
            self.assertEqual(sys.getrefcount(node), 6)

        # Hold weak refs and start verifying ref collection.
        node_refs = [weakref.ref(node) for node in nodes]

        # First round of collection
        del nodes2
        gc.collect()
        for node in nodes:
            self.assertEqual(sys.getrefcount(node), 5)

        # Second round of collection, no more local lists of nodes.
        del nodes
        gc.collect()
        for ref in node_refs:
            node = ref()
            self.assertEqual(sys.getrefcount(node), 4)

        # Using invalidate_iters or row_deleted(path, node) will clear out
        # the pooled refs held internal to the GenericTreeModel implementation.
        model.invalidate_iters()
        self.assertEqual(len(model._held_refs), 0)
        gc.collect()
        for ref in node_refs:
            node = ref()
            self.assertEqual(sys.getrefcount(node), 3)

        # Deleting the root node at this point should allow all nodes to be collected
        # as there is no longer a way to reach the children
        del node  # node still in locals() from last iteration
        del model.root
        gc.collect()
        for ref in node_refs:
            self.assertEqual(ref(), None)


@unittest.skipUnless(has_gtk, 'Gtk not available')
class TestIteration(unittest.TestCase):
    def test_iter_next_root(self):
        model = TesterModel()
        it = model.get_iter([0])
        self.assertEqual(it.user_data, id(model.root))
        self.assertEqual(model.root.next, None)

        it = model.iter_next(it)
        self.assertEqual(it, None)

    def test_iter_next_multiple(self):
        model = TesterModel()
        it = model.get_iter([0, 0])
        self.assertEqual(it.user_data, id(model.root.children[0]))

        it = model.iter_next(it)
        self.assertEqual(it.user_data, id(model.root.children[1]))

        it = model.iter_next(it)
        self.assertEqual(it, None)


class ErrorModel(GenericTreeModel):
    # All on_* methods will raise a NotImplementedError by default
    pass


@unittest.skipUnless(has_gtk, 'Gtk not available')
class ExceptHook(object):
    """
    Temporarily installs an exception hook in a context which
    expects the given exc_type to be raised. This allows verification
    of exceptions that occur within python gi callbacks but
    are never bubbled through from python to C back to python.
    This works because exception hooks are called in PyErr_Print.
    """
    def __init__(self, *expected_exc_types):
        self._expected_exc_types = expected_exc_types
        self._exceptions = []

    def _excepthook(self, exc_type, value, traceback):
        self._exceptions.append((exc_type, value))

    def __enter__(self):
        self._oldhook = sys.excepthook
        sys.excepthook = self._excepthook
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        sys.excepthook = self._oldhook
        error_message = 'Expecting the following exceptions: %s, got: %s' % \
            (str(self._expected_exc_types), '\n'.join([str(item) for item in self._exceptions]))

        assert len(self._expected_exc_types) == len(self._exceptions), error_message

        for expected, got in zip(self._expected_exc_types, [exc[0] for exc in self._exceptions]):
            assert issubclass(got, expected), error_message


@unittest.skipUnless(has_gtk, 'Gtk not available')
class TestReturnsAfterError(unittest.TestCase):
    def setUp(self):
        self.model = ErrorModel()

    def test_get_flags(self):
        with ExceptHook(NotImplementedError):
            flags = self.model.get_flags()
        self.assertEqual(flags, 0)

    def test_get_n_columns(self):
        with ExceptHook(NotImplementedError):
            count = self.model.get_n_columns()
        self.assertEqual(count, 0)

    def test_get_column_type(self):
        with ExceptHook(NotImplementedError, TypeError):
            col_type = self.model.get_column_type(0)
        self.assertEqual(col_type, GObject.TYPE_INVALID)

    def test_get_iter(self):
        with ExceptHook(NotImplementedError):
            self.assertRaises(ValueError, self.model.get_iter, Gtk.TreePath(0))

    def test_get_path(self):
        it = self.model.create_tree_iter('foo')
        with ExceptHook(NotImplementedError):
            path = self.model.get_path(it)
        self.assertEqual(path, None)

    def test_get_value(self):
        it = self.model.create_tree_iter('foo')
        with ExceptHook(NotImplementedError):
            try:
                self.model.get_value(it, 0)
            except TypeError:
                pass  # silence TypeError converting None to GValue

    def test_iter_has_child(self):
        it = self.model.create_tree_iter('foo')
        with ExceptHook(NotImplementedError):
            res = self.model.iter_has_child(it)
        self.assertEqual(res, False)

    def test_iter_next(self):
        it = self.model.create_tree_iter('foo')
        with ExceptHook(NotImplementedError):
            res = self.model.iter_next(it)
        self.assertEqual(res, None)

    def test_iter_children(self):
        with ExceptHook(NotImplementedError):
            res = self.model.iter_children(None)
        self.assertEqual(res, None)

    def test_iter_n_children(self):
        with ExceptHook(NotImplementedError):
            res = self.model.iter_n_children(None)
        self.assertEqual(res, 0)

    def test_iter_nth_child(self):
        with ExceptHook(NotImplementedError):
            res = self.model.iter_nth_child(None, 0)
        self.assertEqual(res, None)

    def test_iter_parent(self):
        child = self.model.create_tree_iter('foo')
        with ExceptHook(NotImplementedError):
            res = self.model.iter_parent(child)
        self.assertEqual(res, None)

if __name__ == '__main__':
    unittest.main()
