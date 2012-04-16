"""Unit tests for the copy module."""

import copy
import copy_reg
import weakref

import unittest
from test import test_support

class TestCopy(unittest.TestCase):

    # Attempt full line coverage of copy.py from top to bottom

    def test_exceptions(self):
        self.assertTrue(copy.Error is copy.error)
        self.assertTrue(issubclass(copy.Error, Exception))

    # The copy() method

    def test_copy_basic(self):
        x = 42
        y = copy.copy(x)
        self.assertEqual(x, y)

    def test_copy_copy(self):
        class C(object):
            def __init__(self, foo):
                self.foo = foo
            def __copy__(self):
                return C(self.foo)
        x = C(42)
        y = copy.copy(x)
        self.assertEqual(y.__class__, x.__class__)
        self.assertEqual(y.foo, x.foo)

    def test_copy_registry(self):
        class C(object):
            def __new__(cls, foo):
                obj = object.__new__(cls)
                obj.foo = foo
                return obj
        def pickle_C(obj):
            return (C, (obj.foo,))
        x = C(42)
        self.assertRaises(TypeError, copy.copy, x)
        copy_reg.pickle(C, pickle_C, C)
        y = copy.copy(x)

    def test_copy_reduce_ex(self):
        class C(object):
            def __reduce_ex__(self, proto):
                return ""
            def __reduce__(self):
                raise test_support.TestFailed, "shouldn't call this"
        x = C()
        y = copy.copy(x)
        self.assertTrue(y is x)

    def test_copy_reduce(self):
        class C(object):
            def __reduce__(self):
                return ""
        x = C()
        y = copy.copy(x)
        self.assertTrue(y is x)

    def test_copy_cant(self):
        class C(object):
            def __getattribute__(self, name):
                if name.startswith("__reduce"):
                    raise AttributeError, name
                return object.__getattribute__(self, name)
        x = C()
        self.assertRaises(copy.Error, copy.copy, x)

    # Type-specific _copy_xxx() methods

    def test_copy_atomic(self):
        class Classic:
            pass
        class NewStyle(object):
            pass
        def f():
            pass
        tests = [None, 42, 2L**100, 3.14, True, False, 1j,
                 "hello", u"hello\u1234", f.func_code,
                 NewStyle, xrange(10), Classic, max]
        for x in tests:
            self.assertTrue(copy.copy(x) is x, repr(x))

    def test_copy_list(self):
        x = [1, 2, 3]
        self.assertEqual(copy.copy(x), x)

    def test_copy_tuple(self):
        x = (1, 2, 3)
        self.assertEqual(copy.copy(x), x)

    def test_copy_dict(self):
        x = {"foo": 1, "bar": 2}
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_vanilla(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_copy(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __copy__(self):
                return C(self.foo)
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_getinitargs(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getinitargs__(self):
                return (self.foo,)
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_getstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return {"foo": self.foo}
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __setstate__(self, state):
                self.foo = state["foo"]
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_getstate_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return self.foo
            def __setstate__(self, state):
                self.foo = state
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    # The deepcopy() method

    def test_deepcopy_basic(self):
        x = 42
        y = copy.deepcopy(x)
        self.assertEqual(y, x)

    def test_deepcopy_memo(self):
        # Tests of reflexive objects are under type-specific sections below.
        # This tests only repetitions of objects.
        x = []
        x = [x, x]
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y[0] is not x[0])
        self.assertTrue(y[0] is y[1])

    def test_deepcopy_issubclass(self):
        # XXX Note: there's no way to test the TypeError coming out of
        # issubclass() -- this can only happen when an extension
        # module defines a "type" that doesn't formally inherit from
        # type.
        class Meta(type):
            pass
        class C:
            __metaclass__ = Meta
        self.assertEqual(copy.deepcopy(C), C)

    def test_deepcopy_deepcopy(self):
        class C(object):
            def __init__(self, foo):
                self.foo = foo
            def __deepcopy__(self, memo=None):
                return C(self.foo)
        x = C(42)
        y = copy.deepcopy(x)
        self.assertEqual(y.__class__, x.__class__)
        self.assertEqual(y.foo, x.foo)

    def test_deepcopy_registry(self):
        class C(object):
            def __new__(cls, foo):
                obj = object.__new__(cls)
                obj.foo = foo
                return obj
        def pickle_C(obj):
            return (C, (obj.foo,))
        x = C(42)
        self.assertRaises(TypeError, copy.deepcopy, x)
        copy_reg.pickle(C, pickle_C, C)
        y = copy.deepcopy(x)

    def test_deepcopy_reduce_ex(self):
        class C(object):
            def __reduce_ex__(self, proto):
                return ""
            def __reduce__(self):
                raise test_support.TestFailed, "shouldn't call this"
        x = C()
        y = copy.deepcopy(x)
        self.assertTrue(y is x)

    def test_deepcopy_reduce(self):
        class C(object):
            def __reduce__(self):
                return ""
        x = C()
        y = copy.deepcopy(x)
        self.assertTrue(y is x)

    def test_deepcopy_cant(self):
        class C(object):
            def __getattribute__(self, name):
                if name.startswith("__reduce"):
                    raise AttributeError, name
                return object.__getattribute__(self, name)
        x = C()
        self.assertRaises(copy.Error, copy.deepcopy, x)

    # Type-specific _deepcopy_xxx() methods

    def test_deepcopy_atomic(self):
        class Classic:
            pass
        class NewStyle(object):
            pass
        def f():
            pass
        tests = [None, 42, 2L**100, 3.14, True, False, 1j,
                 "hello", u"hello\u1234", f.func_code,
                 NewStyle, xrange(10), Classic, max]
        for x in tests:
            self.assertTrue(copy.deepcopy(x) is x, repr(x))

    def test_deepcopy_list(self):
        x = [[1, 2], 3]
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(x is not y)
        self.assertTrue(x[0] is not y[0])

    def test_deepcopy_reflexive_list(self):
        x = []
        x.append(x)
        y = copy.deepcopy(x)
        self.assertRaises(RuntimeError, cmp, y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y[0] is y)
        self.assertEqual(len(y), 1)

    def test_deepcopy_tuple(self):
        x = ([1, 2], 3)
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(x is not y)
        self.assertTrue(x[0] is not y[0])

    def test_deepcopy_reflexive_tuple(self):
        x = ([],)
        x[0].append(x)
        y = copy.deepcopy(x)
        self.assertRaises(RuntimeError, cmp, y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y[0] is not x[0])
        self.assertTrue(y[0][0] is y)

    def test_deepcopy_dict(self):
        x = {"foo": [1, 2], "bar": 3}
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(x is not y)
        self.assertTrue(x["foo"] is not y["foo"])

    def test_deepcopy_reflexive_dict(self):
        x = {}
        x['foo'] = x
        y = copy.deepcopy(x)
        self.assertRaises(RuntimeError, cmp, y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y['foo'] is y)
        self.assertEqual(len(y), 1)

    def test_deepcopy_keepalive(self):
        memo = {}
        x = 42
        y = copy.deepcopy(x, memo)
        self.assertTrue(memo[id(x)] is x)

    def test_deepcopy_inst_vanilla(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y.foo is not x.foo)

    def test_deepcopy_inst_deepcopy(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __deepcopy__(self, memo):
                return C(copy.deepcopy(self.foo, memo))
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y.foo is not x.foo)

    def test_deepcopy_inst_getinitargs(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getinitargs__(self):
                return (self.foo,)
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y.foo is not x.foo)

    def test_deepcopy_inst_getstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return {"foo": self.foo}
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y.foo is not x.foo)

    def test_deepcopy_inst_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __setstate__(self, state):
                self.foo = state["foo"]
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y.foo is not x.foo)

    def test_deepcopy_inst_getstate_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return self.foo
            def __setstate__(self, state):
                self.foo = state
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y is not x)
        self.assertTrue(y.foo is not x.foo)

    def test_deepcopy_reflexive_inst(self):
        class C:
            pass
        x = C()
        x.foo = x
        y = copy.deepcopy(x)
        self.assertTrue(y is not x)
        self.assertTrue(y.foo is y)

    # _reconstruct()

    def test_reconstruct_string(self):
        class C(object):
            def __reduce__(self):
                return ""
        x = C()
        y = copy.copy(x)
        self.assertTrue(y is x)
        y = copy.deepcopy(x)
        self.assertTrue(y is x)

    def test_reconstruct_nostate(self):
        class C(object):
            def __reduce__(self):
                return (C, ())
        x = C()
        x.foo = 42
        y = copy.copy(x)
        self.assertTrue(y.__class__ is x.__class__)
        y = copy.deepcopy(x)
        self.assertTrue(y.__class__ is x.__class__)

    def test_reconstruct_state(self):
        class C(object):
            def __reduce__(self):
                return (C, (), self.__dict__)
            def __cmp__(self, other):
                return cmp(self.__dict__, other.__dict__)
            __hash__ = None # Silence Py3k warning
        x = C()
        x.foo = [42]
        y = copy.copy(x)
        self.assertEqual(y, x)
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y.foo is not x.foo)

    def test_reconstruct_state_setstate(self):
        class C(object):
            def __reduce__(self):
                return (C, (), self.__dict__)
            def __setstate__(self, state):
                self.__dict__.update(state)
            def __cmp__(self, other):
                return cmp(self.__dict__, other.__dict__)
            __hash__ = None # Silence Py3k warning
        x = C()
        x.foo = [42]
        y = copy.copy(x)
        self.assertEqual(y, x)
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assertTrue(y.foo is not x.foo)

    def test_reconstruct_reflexive(self):
        class C(object):
            pass
        x = C()
        x.foo = x
        y = copy.deepcopy(x)
        self.assertTrue(y is not x)
        self.assertTrue(y.foo is y)

    # Additions for Python 2.3 and pickle protocol 2

    def test_reduce_4tuple(self):
        class C(list):
            def __reduce__(self):
                return (C, (), self.__dict__, iter(self))
            def __cmp__(self, other):
                return (cmp(list(self), list(other)) or
                        cmp(self.__dict__, other.__dict__))
            __hash__ = None # Silence Py3k warning
        x = C([[1, 2], 3])
        y = copy.copy(x)
        self.assertEqual(x, y)
        self.assertTrue(x is not y)
        self.assertTrue(x[0] is y[0])
        y = copy.deepcopy(x)
        self.assertEqual(x, y)
        self.assertTrue(x is not y)
        self.assertTrue(x[0] is not y[0])

    def test_reduce_5tuple(self):
        class C(dict):
            def __reduce__(self):
                return (C, (), self.__dict__, None, self.iteritems())
            def __cmp__(self, other):
                return (cmp(dict(self), list(dict)) or
                        cmp(self.__dict__, other.__dict__))
            __hash__ = None # Silence Py3k warning
        x = C([("foo", [1, 2]), ("bar", 3)])
        y = copy.copy(x)
        self.assertEqual(x, y)
        self.assertTrue(x is not y)
        self.assertTrue(x["foo"] is y["foo"])
        y = copy.deepcopy(x)
        self.assertEqual(x, y)
        self.assertTrue(x is not y)
        self.assertTrue(x["foo"] is not y["foo"])

    def test_copy_slots(self):
        class C(object):
            __slots__ = ["foo"]
        x = C()
        x.foo = [42]
        y = copy.copy(x)
        self.assertTrue(x.foo is y.foo)

    def test_deepcopy_slots(self):
        class C(object):
            __slots__ = ["foo"]
        x = C()
        x.foo = [42]
        y = copy.deepcopy(x)
        self.assertEqual(x.foo, y.foo)
        self.assertTrue(x.foo is not y.foo)

    def test_deepcopy_dict_subclass(self):
        class C(dict):
            def __init__(self, d=None):
                if not d:
                    d = {}
                self._keys = list(d.keys())
                dict.__init__(self, d)
            def __setitem__(self, key, item):
                dict.__setitem__(self, key, item)
                if key not in self._keys:
                    self._keys.append(key)
        x = C(d={'foo':0})
        y = copy.deepcopy(x)
        self.assertEqual(x, y)
        self.assertEqual(x._keys, y._keys)
        self.assertTrue(x is not y)
        x['bar'] = 1
        self.assertNotEqual(x, y)
        self.assertNotEqual(x._keys, y._keys)

    def test_copy_list_subclass(self):
        class C(list):
            pass
        x = C([[1, 2], 3])
        x.foo = [4, 5]
        y = copy.copy(x)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.foo, y.foo)
        self.assertTrue(x[0] is y[0])
        self.assertTrue(x.foo is y.foo)

    def test_deepcopy_list_subclass(self):
        class C(list):
            pass
        x = C([[1, 2], 3])
        x.foo = [4, 5]
        y = copy.deepcopy(x)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.foo, y.foo)
        self.assertTrue(x[0] is not y[0])
        self.assertTrue(x.foo is not y.foo)

    def test_copy_tuple_subclass(self):
        class C(tuple):
            pass
        x = C([1, 2, 3])
        self.assertEqual(tuple(x), (1, 2, 3))
        y = copy.copy(x)
        self.assertEqual(tuple(y), (1, 2, 3))

    def test_deepcopy_tuple_subclass(self):
        class C(tuple):
            pass
        x = C([[1, 2], 3])
        self.assertEqual(tuple(x), ([1, 2], 3))
        y = copy.deepcopy(x)
        self.assertEqual(tuple(y), ([1, 2], 3))
        self.assertTrue(x is not y)
        self.assertTrue(x[0] is not y[0])

    def test_getstate_exc(self):
        class EvilState(object):
            def __getstate__(self):
                raise ValueError, "ain't got no stickin' state"
        self.assertRaises(ValueError, copy.copy, EvilState())

    def test_copy_function(self):
        self.assertEqual(copy.copy(global_foo), global_foo)
        def foo(x, y): return x+y
        self.assertEqual(copy.copy(foo), foo)
        bar = lambda: None
        self.assertEqual(copy.copy(bar), bar)

    def test_deepcopy_function(self):
        self.assertEqual(copy.deepcopy(global_foo), global_foo)
        def foo(x, y): return x+y
        self.assertEqual(copy.deepcopy(foo), foo)
        bar = lambda: None
        self.assertEqual(copy.deepcopy(bar), bar)

    def _check_weakref(self, _copy):
        class C(object):
            pass
        obj = C()
        x = weakref.ref(obj)
        y = _copy(x)
        self.assertTrue(y is x)
        del obj
        y = _copy(x)
        self.assertTrue(y is x)

    def test_copy_weakref(self):
        self._check_weakref(copy.copy)

    def test_deepcopy_weakref(self):
        self._check_weakref(copy.deepcopy)

    def _check_copy_weakdict(self, _dicttype):
        class C(object):
            pass
        a, b, c, d = [C() for i in xrange(4)]
        u = _dicttype()
        u[a] = b
        u[c] = d
        v = copy.copy(u)
        self.assertFalse(v is u)
        self.assertEqual(v, u)
        self.assertEqual(v[a], b)
        self.assertEqual(v[c], d)
        self.assertEqual(len(v), 2)
        del c, d
        self.assertEqual(len(v), 1)
        x, y = C(), C()
        # The underlying containers are decoupled
        v[x] = y
        self.assertNotIn(x, u)

    def test_copy_weakkeydict(self):
        self._check_copy_weakdict(weakref.WeakKeyDictionary)

    def test_copy_weakvaluedict(self):
        self._check_copy_weakdict(weakref.WeakValueDictionary)

    def test_deepcopy_weakkeydict(self):
        class C(object):
            def __init__(self, i):
                self.i = i
        a, b, c, d = [C(i) for i in xrange(4)]
        u = weakref.WeakKeyDictionary()
        u[a] = b
        u[c] = d
        # Keys aren't copied, values are
        v = copy.deepcopy(u)
        self.assertNotEqual(v, u)
        self.assertEqual(len(v), 2)
        self.assertFalse(v[a] is b)
        self.assertFalse(v[c] is d)
        self.assertEqual(v[a].i, b.i)
        self.assertEqual(v[c].i, d.i)
        del c
        self.assertEqual(len(v), 1)

    def test_deepcopy_weakvaluedict(self):
        class C(object):
            def __init__(self, i):
                self.i = i
        a, b, c, d = [C(i) for i in xrange(4)]
        u = weakref.WeakValueDictionary()
        u[a] = b
        u[c] = d
        # Keys are copied, values aren't
        v = copy.deepcopy(u)
        self.assertNotEqual(v, u)
        self.assertEqual(len(v), 2)
        (x, y), (z, t) = sorted(v.items(), key=lambda pair: pair[0].i)
        self.assertFalse(x is a)
        self.assertEqual(x.i, a.i)
        self.assertTrue(y is b)
        self.assertFalse(z is c)
        self.assertEqual(z.i, c.i)
        self.assertTrue(t is d)
        del x, y, z, t
        del d
        self.assertEqual(len(v), 1)

    def test_deepcopy_bound_method(self):
        class Foo(object):
            def m(self):
                pass
        f = Foo()
        f.b = f.m
        g = copy.deepcopy(f)
        self.assertEqual(g.m, g.b)
        self.assertTrue(g.b.im_self is g)
        g.b()


def global_foo(x, y): return x+y

def test_main():
    test_support.run_unittest(TestCopy)

if __name__ == "__main__":
    test_main()
