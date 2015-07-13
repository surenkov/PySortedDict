import unittest
from sorteddict import SortedDict


class SortedDictTests(unittest.TestCase):

    def test_init(self):
        a = SortedDict()
        self.assertEqual(len(a), 0)

        b = SortedDict({'a': 1, 'b': 2})
        self.assertEqual(len(b), 2)
        self.assertNotEqual(a, b)

        a = SortedDict({'a': 1, 'b': 2})
        self.assertEqual(a, b)

    def test_get(self):
        a = SortedDict({'a': 1, 'b': 2})
        self.assertEqual(a['a'], 1)
        self.assertEqual(a['b'], 2)
        self.assertRaises(KeyError, lambda: a['c'])
        self.assertEqual(a.get('a'), 1)
        self.assertEqual(a.get('b'), 2)
        self.assertEqual(a.get('c', 3), 3)
        self.assertIs(a.get('c'), None)

    def test_set(self):
        a = SortedDict()

        a['a'] = 1
        a['b'] = 2
        self.assertEqual(a['a'], 1)
        self.assertEqual(a['b'], 2)

    def test_in(self):
        a = SortedDict({'a': 1})
        self.assertTrue('a' in a)
        self.assertTrue('b' not in a)

    def test_iter(self):
        a = SortedDict({'a': 1, 'b': 2})
        i = 1
        for k in a:
            self.assertEqual(a[k], i)
            i += 1

    def test_update(self):
        a = SortedDict({'a': 1})
        expected = SortedDict({'a': 1, 'b': 2})

        a.update({'b': 2})
        self.assertEqual(a, expected)

    def test_clear(self):
        a = SortedDict({'a': 1, 'b': 2})
        a.clear()
        self.assertEqual(len(a), 0)
        self.assertTrue('a' not in a)

    def test_keys_values_items(self):
        a = SortedDict({'a': 1, 'b': 2})
        self.assertListEqual(a.keys(), ['a', 'b'])
        self.assertListEqual(a.values(), [1, 2])
        self.assertListEqual(a.items(), [('a', 1), ('b', 2)])

    def test_pop_popitem(self):
        a = SortedDict({'a': 1, 'b': 2})
        self.assertEqual(a.pop('b'), 2)
        self.assertTrue('b' not in a)
        self.assertEqual(a.pop('b', 3), 3)
        self.assertEqual(a.pop(), 1)
        self.assertTrue('a' not in a)
        self.assertRaises(KeyError, lambda: a.pop())

        a = SortedDict({'a': 1})
        self.assertEqual(a.popitem(), ('a', 1))
        self.assertTrue('a' not in a)
        self.assertEqual(len(a), 0)
        self.assertRaises(KeyError, lambda: a.popitem())

    def test_repr(self):
        a = SortedDict({'a': 1, 'b': 2})
        self.assertEqual(repr(a), "{'a': 1, 'b': 2}")


if __name__ == '__main__':
    unittest.main()
