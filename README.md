# allthealgos
Collection of useful algorithms, data structures, and other concepts in computer science
which are likely not in a standard library.

Implementations are in various languages.
Some are just barebones reference implementations,
and others are high quality and ready for integration with user code.

Unless otherwise stated,
all code here is under this same license,
which means,
in short,
that it's free to use
for any purpose
as long as you give credit.

## Why does this repository exist? What purpose does it serve?

We compute at a scale where fast algorithms matter.
These fast algorithms are typically also much harder to implement.
That's why we provide the implementations.
We try to make it easy for you to write better code.

This should be a standard library's job,
but most standard libraries don't and likely will never
provide these kinds of things,
at least in an accessible way.
Common libraries like Boost help with coverage,
but still typically leave many gaps for actually common use cases.
More on this after a blurb about why
compilers and AI
don't do this job either.

Compilers are currently quite good at micro-optimizing small details
but very bad at macro scale optimizations.
While these constant factor optimizations are
indeed important to the speed of modern software,
our compilers aren't anywhere near good enough
to invent new data structures based on needs.
For now, it's up to humans to examine
the properties of various constructs
and find clever ways
to reduce the
time complexity, memory complexity, or some other cost metric
for a set of tasks.

AI methods have the potential
to do much better than
humans and traditional approaches.
For AI to win,
it would need to do the tasks or produce code which does the task,
which is at least as fast as what humans could do,
and is at least as reliable as what humans could do.
AI in general struggles with reliability,
and at the time of writing,
AI also doesn't outperform humans here.

Blurb over.
Now we criticize some common utilities in popular languages
as examples
to make a point.
You should feel free to add other languages to the comparison,
and also make corrections if any of the information is wrong.

### Lists based on trees

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
