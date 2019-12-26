# AVL Trees

## Usage guide

### Python

The AVL tree implementation in `avl_tree.py` is very barebones and is not a suitable replacement for `list`. Please use the `blist` library instead for a high performance tree-based list.

### C++

The file `avl_tree.cpp` is a self contained AVL tree data structure library, all under the `avl` namespace. Included are drop-in replacements for `std::vector`, `std::set`, `std::multiset`, and `std::map`. These all have the same name, so just use ex. `avl::vector`. They should behave like their standard library equivalents.

The base `avl_tree` class is quite customizable, and can be made to support various range operations and behave like a list, set, or other data structure. However, it takes some work to get running.

## Why use AVL Trees?

Lists are sequences of items
which typically support operations such as:

- Get size
- Get item at index
- Insert item at index
- Delete item at index

Lists are most commonly implemented as an "array list",
where the entire list is stored in a single large array,
and the array is remade when the original array is full
and a new element must be added.

While array lists achieve O(1) amortized time for insertion at the end
and are capable of O(1) overhead memory which does not include the items,
they have O(N) time insertion and deletion.

For applications where the list is large
and insertions and deletions are common,
the array list becomes quite bad.

A list based on a balanced tree,
also known as a "tree list",
achieves O(logN) for insertion and deletion.
In fact,
for most possible operations,
tree lists achieve O(logN) or better,
even concatenation,
which is O(N) for array lists.
Some operations,
such as search in an unsorted list,
are impossible to do better than O(N) on a classical computer,
but if a list operation can be done in O(logN) or better,
a tree list can probably do it.
Tree lists also have O(N) overhead memory
as a consequence of storing tree node related information.

| Language (Implementation/Variant) | Standard list is a | Tree list provided by |
| --- | --- | --- |
| C++ | [array list](http://www.cplusplus.com/reference/vector/vector/) | |
| Java | [array list](https://docs.oracle.com/javase/10/docs/api/java/util/ArrayList.html) | [Apache Commons Collections](https://commons.apache.org/proper/commons-collections/apidocs/org/apache/commons/collections4/list/TreeList.html) |
| Python (CPython) | [array list](https://wiki.python.org/moin/TimeComplexity) | [blist](https://pypi.org/project/blist/) |

More advanced list-like data structures may also support operations such as range sum.
