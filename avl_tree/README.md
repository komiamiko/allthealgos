# AVL Trees

## Usage guide

### Python

The AVL tree implementation in `avl_tree.py` is very barebones and is not a suitable replacement for `list`. Please use the `blist` library instead for a high performance tree-based list.

### C++

The file `avl_tree.cpp` is a self contained AVL tree data structure library, all under the `avl` namespace. Included are drop-in replacements for `std::vector`, `std::set`, `std::multiset`, and `std::map`. These all have the same name, so just use ex. `avl::vector`. They should behave like their standard library equivalents.

The base `avl_tree` class is quite customizable, and can be made to support various range operations and behave like a list, set, or other data structure. However, it takes some work to get running.

**IMPORTANT**: This library uses some features and standard library contents which were added in C++20. At the time of writing (late 2019), your compiler may not have support for these, and be unable to compile the library. If there is demand for it, we will make a workaround to support older versions of C++, though the performance may be degraded. Also, some features from C++17 (ex. `std::optional`) are required for this library.

#### Advanced usage

On closer inspection of the `avl_tree` class' template typing, you will see:

- `_Element` which is used to determine what data type the nodes of the tree will store.
- `_Element_Compare` which looks like the less than operator, and is used to determine ordering of values, where it matters. Useful for maintaining a sorted list, sorted set, etc. For operations involving the ordering, it is required to be a total ordering, or else some operations will break. By default is `std::less`.
- `_Size` which is used for the size type. In general a `std::size_t` should work well for this, though if you are working with small trees it may reduce the memory footprint to use a smaller type, say, `uint16_t`.
- `_Merge` which will either return false or merge its right argument into its left argument (which are both references). For performance reasons, the first successful merge will always be taken where applicable, which means that if there are multiple nodes which are capable of accepting a merge, there is no guarantee made on which will actually be merged into. We ask that the merger is well behaved in the sense that, in such an event, no possible outcome is an invalid tree.
- `_Range_Preprocess`, `_Range_Type_Intermediate`, `_Range_Combine`, `_Range_Postprocess` used to define the range operations. Each node's value is first put through the `_Range_Preprocess` operation, producing a value of type `_Range_Type_Intermediate`. These are then combined left to right using `_Range_Combine`. As long as that operation is associative, this will be well behaved. The final combined value across a range is put through `_Range_Postprocess` to get the final result of the range query. The reason why `_Range_Type_Intermediate` matters at all is because each node will store one, which is the intermediate result across the range that is the subtree rooted at that node.
- `_Alloc` is used to manage memory, in place of the standard `new` and `delete`. It can be customized if needed.

You can define all sorts of esoteric data structures. For example, to make a sorted 32-bit integer list where range queries get the difference between the smallest and largest elements, the recipe would look something like this:

- `_Element` as `int32_t`
- `_Range_Preprocess` as *x ↦ (x, x)*
- `_Range_Combine` as *(w, x), (y, z) ↦ (min{w, y}, max{x, z})*
- `_Range_Postprocess` as *(x, y) ↦ y - x*

They're written here mathematically, though they could be written more verbosely as proper C++ callable objects.

#### Test coverage

Basic development tests compile correctly and pass fine on:

- clang 7.0.0 with C++14

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
