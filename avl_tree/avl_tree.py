"""
Basic implementation of a list using an AVL tree.
Purpose is to have a working AVL tree, not conform to any API.
Supported operations:
- Get size (number of elements)
- Inorder traversal (iteration)
- Insert value at index
- Pop (delete) at index and return popped value
Additional features:
- Does not have a parent pointer
- Stores only balance factor, not height
Notes on extras:
- Size is only needed to implement an indexable sequence such as a list
How does it work? Why does it work? It's basically magic.
Look at the Wikipedia page for AVL trees and other resources to learn more about them.
The main function at the bottom is just a test, which you can delete.
"""

class avl_node(object):
    def __init__(self, value):
        self.value = value
        self.size = 1
        self.balance = 0
        self.left = None
        self.right = None
    def __iter__(self):
        if self.left is not None:
            yield from self.left
        yield self.value
        if self.right is not None:
            yield from self.right
    def __str__(self,indent=0):
        bits = []
        if self.left is not None:
            bits.append(self.left.__str__(indent=indent+1))
            bits.append(' '*indent+'/')
        bits.append(' '*indent+f'{self.value} ({self.balance})')
        if self.right is not None:
            bits.append(' '*indent+'\\')
            bits.append(self.right.__str__(indent=indent+1))
        return '\n'.join(bits)
    def resize(self):
        self.size = avl_size(self.left) + 1 + avl_size(self.right)
    def rotate_left(self):
        pivot = self.right
        self.right = pivot.left
        pivot.left = self
        self.balance -= 1 + max(0, pivot.balance)
        pivot.balance -= 1 - min(0, self.balance)
        self.resize()
        pivot.resize()
        return pivot
    def rotate_right(self):
        pivot = self.left
        self.left = pivot.right
        pivot.right = self
        self.balance += 1 - min(0, pivot.balance)
        pivot.balance += 1 + max(0, self.balance)
        self.resize()
        pivot.resize()
        return pivot
    def ensure_left(self):
        if self.balance <= 0:
            return self
        return self.rotate_left()
    def ensure_right(self):
        if self.balance >= 0:
            return self
        return self.rotate_right()
    def rebalance_left(self):
        if self.right is not None:
            self.right = self.right.ensure_right()
        return self.rotate_left()
    def rebalance_right(self):
        if self.left is not None:
            self.left = self.left.ensure_left()
        return self.rotate_right()

def avl_size(node):
    if node is None:
        return 0
    return node.size

def avl_insert(node, index, value):
    if node is None:
        node = avl_node(value)
        return (node, True)
    left_size = avl_size(node.left)
    if index <= left_size:
        node.left, taller = avl_insert(node.left, index, value)
        node.resize()
        node.balance -= taller
        if not taller or node.balance == 0:
            return (node, False)
        if node.balance == -1:
            return (node, True)
        return (node.rebalance_right(), False)
    else:
        node.right, taller = avl_insert(node.right, index - left_size - 1, value)
        node.resize()
        node.balance += taller
        if not taller or node.balance == 0:
            return (node, False)
        if node.balance == 1:
            return (node, True)
        return (node.rebalance_left(), False)

def avl_delete(node, index):
    left_size = avl_size(node.left)
    if index == left_size:
        if node.left is None and node.right is None:
            return (None, True, node.value)
        if node.left is None or node.right is None:
            return (node.left or node.right, True, node.value)
        result = node.value
        node.right, shorter, node.value = avl_delete(node.right, 0)
        node.balance -= shorter
        node.resize()
        if not shorter or node.balance == -1:
            return (node, False, result)
        if node.balance == 0:
            return (node, True, result)
        node = node.rebalance_right()
        shorter = node.balance == 0
        return (node, shorter, result)
    elif index < left_size:
        node.left, shorter, result = avl_delete(node.left, index)
        node.balance += shorter
        node.resize()
        if not shorter or node.balance == 1:
            return (node, False, result)
        if node.balance == 0:
            return (node, True, result)
        node = node.rebalance_left()
        shorter = node.balance == 0
        return (node, shorter, result)
    else:
        node.right, shorter, result = avl_delete(node.right, index - left_size - 1)
        node.balance -= shorter
        node.resize()
        if not shorter or node.balance == -1:
            return (node, False, result)
        if node.balance == 0:
            return (node, True, result)
        node = node.rebalance_right()
        shorter = node.balance == 0
        return (node, shorter, result)
    
class avl_tree(object):
    def __init__(self):
        self.root = None
    def __len__(self):
        return avl_size(self.root)
    def __iter__(self):
        return iter(self.root or ())
    def __str__(self):
        return str(self.root)
    def insert(self, index, value):
        self.root = avl_insert(self.root, index, value)[0]
    def delete(self, index):
        self.root, _, result = avl_delete(self.root, index)
        return result
