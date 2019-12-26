"""
Tests for the AVL tree list written in Python.
"""

import unittest

class TestCorrectTree(unittest.TestCase):
    """
    Tests the correctness of the tree implementation.
    """
    def test_basics(self):
        """
        Very basic test.
        Inserts, checks length, iterates over the items to check correctness, and then deletes.
        Does this a few more times for good measure.
        """
        from avl_tree import avl_tree
        from random import randint
        
        tree = avl_tree()
        li = []
        
        def check_eq():
            self.assertEqual(len(li), len(tree))
            self.assertEqual(li, list(tree))

        # fully scripted tests
        
        check_eq()
        
        for i in range(10):
            v = i
            tree.insert(i, v)
            li.insert(i, v)
            check_eq()

        for i in range(10):
            v = -i
            tree.insert(0, v)
            li.insert(0, v)
            check_eq()

        for i in range(10):
            v = i + 20
            tree.insert(i*2, v)
            li.insert(i*2, v)
            check_eq()

        for i in range(10):
            v = i + 40
            tree.insert(i*2+1, v)
            li.insert(i*2+1, v)
            check_eq()

        for i in range(10):
            u = tree.delete(i*3)
            v = li.pop(i*3)
            self.assertEqual(u, v)
            check_eq()

        for i in range(10):
            u = tree.delete(0)
            v = li.pop(0)
            self.assertEqual(u, v)
            check_eq()

        for i in range(19,-1,-1):
            u = tree.delete(i)
            v = li.pop(i)
            self.assertEqual(u, v)
            check_eq()

        # randomized larger load test

        for _ in range(1000):
            if li and randint(0, 2) == 0:
                i = randint(0, len(li)-1)
                u = tree.delete(i)
                v = li.pop(i)
                self.assertEqual(u, v)
            else:
                i = randint(0, len(li))
                v = randint(0, 999)
                tree.insert(i, v)
                li.insert(i, v)
            check_eq()

        while li:
            u = tree.delete(0)
            v = li.pop(0)
            self.assertEqual(u, v)
            check_eq()
                
if __name__ == '__main__':
    unittest.main()
