// AVL tree library. See documentation and README for more information.

#ifndef _AVL_TREE_H
#define _AVL_TREE_H

#include <algorithm>
#include <functional>
// type_traits: had some changes in C++17
#include <memory>
#include <stdexcept>
#include <type_traits>

#if __cplusplus >= 201703L
// invoke_result: as of C++17
#define avl_invoke_result(T, ...) std::invoke_result<T, __VA_ARGS__>
// optional: as of C++17
#include <optional>
#define avl_optional std::optional
#else
// result_of: before C++17
#define avl_invoke_result(T, ...) std::result_of<T(__VA_ARGS__)>
// optional: as of C++17
#include <experimental/optional>
#define avl_optional std::experimental::optional
#endif

//! AVL tree library with an extensible AVL tree class.
/*!
 * An AVL tree implementation and some common collection types based on it.
 * Includes drop-in replacements for vector, set, multiset, and map.
 *
 * This AVL tree implementation can support additional features optionally:
 * - Indexable
 * - Ordered
 * - Range queries
 *
 * Note that the compiler may require you implement certain things even if
 * that code will never be run.
 *
 * How to use range queries:
 * - Elements will first individually be preprocessed, then combined left to
 * right, then the result is postprocessed.
 * - Combine must be associative.
 *
 * How to use merge:
 * - Merge will either merge 2 entries and return true or do nothing and return
 * false.
 * - Left argument to merge is the "merge target", and will be kept if merged.
 * The right argument would be discarded.
 *
 * Possible weird behaviour:
 * - Indexing relies on the same stuff as sizing, so if the tree is not made
 * indexable, you also will not know its size Also note that various operations
 * are assumed to be O(1) such as the range combine. If the complexity is not
 * O(1), it's up to you to determine the actual complexity in any complexity
 * analysis.
 */
namespace avl {

//! Empty 0 sized struct, which has only 1 possible state, hence the name.
/*!
 * The empty 0 sized struct.
 * There's lots of reimplementations of this struct out there,
 * and this one defines various operations for completeness.
 * In mathematical terms, this can be modeled as just 0,
 * which when combined with itself produces 0.
 */
struct monostate {
  monostate();
  template <typename T>
  monostate(const T &);
  template <typename T>
  monostate operator()(const T &) const;
};

monostate::monostate() {}
template <typename T>
monostate::monostate(const T &t) {}
template <typename T>
monostate monostate::operator()(const T &) const {
  return monostate();
}
bool constexpr operator==(const monostate &lhs, const monostate &rhs) noexcept {
  return true;
}
bool constexpr operator>=(const monostate &lhs, const monostate &rhs) noexcept {
  return true;
}
bool constexpr operator<=(const monostate &lhs, const monostate &rhs) noexcept {
  return true;
}
bool constexpr operator!=(const monostate &lhs, const monostate &rhs) noexcept {
  return false;
}
bool constexpr operator<(const monostate &lhs, const monostate &rhs) noexcept {
  return false;
}
bool constexpr operator>(const monostate &lhs, const monostate &rhs) noexcept {
  return false;
}
monostate const operator+(const monostate &lhs, const monostate &rhs) noexcept {
  return monostate();
}
monostate const operator-(const monostate &lhs, const monostate &rhs) noexcept {
  return monostate();
}
monostate const operator*(const monostate &lhs, const monostate &rhs) noexcept {
  return monostate();
}
monostate const operator|(const monostate &lhs, const monostate &rhs) noexcept {
  return monostate();
}
monostate const operator&(const monostate &lhs, const monostate &rhs) noexcept {
  return monostate();
}

//! Ad-hoc identity function object.
/*!
 * Identity function. It's in std::functional as of C++20, but it's currently
 * 2019 and support is still on its way. For the time being, we use this as a
 * drop-in replacement. It's semantically not exactly the same (this one is
 * jankier for sure) but it does the job.
 */
template <typename T>
struct identity {
  const T &operator()(const T &value) const noexcept { return value; }
};

//! A basic merger: Never merge.
/*!
 * One of the basic mergers: never merges.
 * May be useful for implementing a simple list
 * which does not care about duplicates.
 */
template <typename T>
struct no_merge {
  const bool operator()(T &, const T &) const;
};

template <typename T>
const bool no_merge<T>::operator()(T &to, const T &from) const {
  return false;
}

//! A basic merger: merge if equal, and do nothing.
/*!
 * One of the basic mergers: merge if equal, and do nothing.
 * May be useful for implementing a simple set which does not allow duplicates elements.
 */
template <typename T>
struct merge_if_equal {
  const bool operator()(T &, const T &) const;
};

template <typename T>
const bool merge_if_equal<T>::operator()(T &to, const T &from) const {
  return to == from;
}

//! A less basic merger: merge if the first of the pair is equal, and add the second of the pair.
/*!
 * One of the less basic mergers: if the key (first of the pair) is equal,
 * merge, and combine the counters (second of the pair).
 * May be useful for implementing a simple multiset/bag
 * which compactly represents duplicates as the element and a counter.
 */
template <typename T, typename C>
struct merge_count {
  const bool operator()(std::pair<T, C> &, const std::pair<T, C> &) const;
};

template <typename T, typename C>
const bool merge_count<T, C>::operator()(std::pair<T, C> &to,
                                         const std::pair<T, C> &from) const {
  if (to != from) return false;
  to.second += from.second;
  return true;
}

template <typename _Element, typename _Size = std::size_t,
          typename _Range_Type_Intermediate = monostate>
class avl_node;

// forward declarations for helper functions

template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
_Size avl_node_size(avl_node<_Element, _Size, _Range_Type_Intermediate> *node);

template <typename _Element_2, typename _Size_2, typename _Range_Type_Intermediate_2>
const _Element_2&
avl_node_get_at_index(
    const avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2>*, _Size_2);

template <typename _Element_2, typename _Size_2,
          typename _Range_Type_Intermediate_2, typename _Merge,
          typename _Range_Preprocess, typename _Range_Combine, typename _Alloc>
std::pair<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, bool>
avl_node_insert_at_index(
    avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Size_2,
    _Element_2, const _Merge &, const _Range_Preprocess &,
    const _Range_Combine &, _Alloc);

template <typename _Element_2, typename _Size_2,
          typename _Range_Type_Intermediate_2, typename _Compare,
          typename _Merge, typename _Range_Preprocess, typename _Range_Combine,
          typename _Alloc>
std::tuple<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, bool,
           _Size_2>
avl_node_insert_ordered(
    avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Element_2,
    const _Compare &, const _Merge &, const _Range_Preprocess &,
    const _Range_Combine &, _Alloc);

template <typename _Element_2, typename _Size_2,
          typename _Range_Type_Intermediate_2, typename _Range_Preprocess,
          typename _Range_Combine, typename _Alloc>
std::tuple<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, bool,
           _Element_2>
avl_node_remove_at_index(
    avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Size_2,
    const _Range_Preprocess &, const _Range_Combine &, _Alloc);

template <typename _Element_2, typename _Size_2,
          typename _Range_Type_Intermediate_2, typename _Compare,
          typename _Range_Preprocess, typename _Range_Combine, typename _Alloc>
std::tuple<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, bool,
           avl_optional<_Size_2>>
avl_node_remove_ordered(
    avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Element_2,
    const _Compare &, const _Range_Preprocess &, const _Range_Combine &,
    _Alloc);

template <typename _Element_2, typename _Size_2, typename _Range_Type_Intermediate_2,
          typename _Merge, typename _Range_Preprocess, typename _Range_Combine,
          typename _Alloc>
std::pair<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, bool>
avl_node_replace_at_index(
    avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Size_2,
    _Element_2, const _Merge &, const _Range_Preprocess &,
    const _Range_Combine &, _Alloc);

template <typename _Element_2, typename _Size_2, typename _Range_Type_Intermediate_2,
          typename _Compare, typename _Merge,
          typename _Range_Preprocess, typename _Range_Combine,
          typename _Alloc>
std::tuple<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, bool, avl_optional<std::pair<_Size_2,_Size_2>>>
avl_node_replace_ordered(
    avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Element_2,
    _Element_2, const _Compare &,
    const _Merge &, const _Range_Preprocess &,
    const _Range_Combine &, _Alloc);

// declaration for avl_node

//! AVL tree node; for internal use.
/*!
 * Represents a single AVL tree node.
 * Stores left and right child pointers, the actual data element,
 * the subtree's size (number of nodes contained), the balance factor,
 * and the intermediate range value.
 * Designated for internal use; to enforce this,
 * data members are private and only exposed through the helper functions
 * which work on the tree at a higher level.
 * It's intentional that end users can see the class (they need it for type declarations),
 * but those users aren't meant to manipulate the nodes directly.
 * Subtrees are represented as pointers to nodes,
 * with the null pointer being an empty subtree.
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
class avl_node {
 private:
  //! Left child pointer.
  /*!
   * Pointer to the left child at this node.
   */
  avl_node *left;
 private:
  //! Value of this node.
  /*!
   * The single value of this node.
   * May also be called the data value or label.
   */
  [[no_unique_address]] _Element value;
 private:
  //! Right child pointer.
  /*!
   * Pointer to the right child at this node.
   */
  avl_node *right;
  //! Size of the subtree rooted at this node.
  /*!
   * The size (number of nodes contained) of the subtree rooted at this node.
   */
  [[no_unique_address]] _Size size;
  //! Balance factor of this node.
  /*!
   * The "balance factor" of this node in the AVL tree.
   * Balance factors are used for memory efficient (better than storing the height) implementations of AVL trees.
   * The balance factor is always either -1, 0, or 1, equal to the height of the right subtree minus the height of the left subtree.
   */
  char balance;
  //! Range intermediate value for this subtree.
  /*!
   * The range intermediate value for this subtree.
   * Used for range operations.
   *
   * \sa avl_tree
   */
  [[no_unique_address]] _Range_Type_Intermediate subrange;

 public:
  //! Construct from data.
  /*!
   * Construct a lone node given an element and range intermediate value.
   * Other data members are set automatically to match those of a single node with no children.
   * \param i_value the element value
   * \param i_subrange the range intermediate value for just this element
   */
  avl_node(const _Element &i_value,
           const _Range_Type_Intermediate &i_subrange) {
    left = nullptr;
    value = i_value;
    right = nullptr;
    size = _Size(1);
    balance = char(0);
    subrange = i_subrange;
  }

  // these helper functions are friends

  template <typename _Element_2, typename _Size_2,
            typename _Range_Type_Intermediate_2>
  friend _Size_2 avl::avl_node_size(
      avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *);

  template <typename _Element_2, typename _Size_2, typename _Range_Type_Intermediate_2>
  friend const _Element_2&
  avl::avl_node_get_at_index(
    const avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2>*, _Size_2);

  template <typename _Element_2, typename _Size_2,
            typename _Range_Type_Intermediate_2, typename _Merge,
            typename _Range_Preprocess, typename _Range_Combine,
            typename _Alloc>
  friend std::pair<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *,
                   bool>
  avl::avl_node_insert_at_index(
      avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Size_2,
      _Element_2, const _Merge &, const _Range_Preprocess &,
      const _Range_Combine &, _Alloc);

  template <typename _Element_2, typename _Size_2,
            typename _Range_Type_Intermediate_2, typename _Compare,
            typename _Merge, typename _Range_Preprocess,
            typename _Range_Combine, typename _Alloc>
  friend std::tuple<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *,
                    bool, _Size_2>
  avl::avl_node_insert_ordered(
      avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Element_2,
      const _Compare &, const _Merge &, const _Range_Preprocess &,
      const _Range_Combine &, _Alloc);

  template <typename _Element_2, typename _Size_2,
            typename _Range_Type_Intermediate_2, typename _Range_Preprocess,
            typename _Range_Combine, typename _Alloc>
  friend std::tuple<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *,
                    bool, _Element_2>
  avl::avl_node_remove_at_index(
      avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Size_2,
      const _Range_Preprocess &, const _Range_Combine &, _Alloc);

  template <typename _Element_2, typename _Size_2,
            typename _Range_Type_Intermediate_2, typename _Compare,
            typename _Range_Preprocess, typename _Range_Combine,
            typename _Alloc>
  friend std::tuple<avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *,
                    bool, avl_optional<_Size_2>>
  avl::avl_node_remove_ordered(
      avl_node<_Element_2, _Size_2, _Range_Type_Intermediate_2> *, _Element_2,
      const _Compare &, const _Range_Preprocess &, const _Range_Combine &,
      _Alloc);

  // avl_node_replace_at_index does not need friend
  // avl_node_replace_ordered does not need friend

  // these are our methods

  template <typename _Range_Preprocess, typename _Range_Combine>
  void update(const _Range_Preprocess &, const _Range_Combine &);
  template <typename _Range_Preprocess, typename _Range_Combine>
  avl_node *rotate_left(const _Range_Preprocess &_rpre,
                        const _Range_Combine &_rcomb);
  template <typename _Range_Preprocess, typename _Range_Combine>
  avl_node *rotate_right(const _Range_Preprocess &_rpre,
                         const _Range_Combine &_rcomb);
  template <typename _Range_Preprocess, typename _Range_Combine>
  avl_node *ensure_not_right_heavy(const _Range_Preprocess &_rpre,
                                   const _Range_Combine &_rcomb);
  template <typename _Range_Preprocess, typename _Range_Combine>
  avl_node *ensure_not_left_heavy(const _Range_Preprocess &_rpre,
                                  const _Range_Combine &_rcomb);
  template <typename _Range_Preprocess, typename _Range_Combine>
  avl_node *rebalance_right_heavy(const _Range_Preprocess &_rpre,
                                  const _Range_Combine &_rcomb);
  template <typename _Range_Preprocess, typename _Range_Combine>
  avl_node *rebalance_left_heavy(const _Range_Preprocess &_rpre,
                                 const _Range_Combine &_rcomb);
};

//! Get the size of the subtree.
/*!
 * Get the number of nodes in a subtree.
 * A subtree represented by the null pointer is an empty tree, so its size is 0.
 *
 * \param node the node to get the size of
 * \return how many nodes are in the subtree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
_Size avl_node_size(avl_node<_Element, _Size, _Range_Type_Intermediate> *node) {
  if (node == nullptr) return 0;
  return node->size;
}

//! Update size and range intermediate values at this node.
/*!
 * Updates size and range intermediate values at this node. Assumes its children
 * (if they exist) already have correct values.
 *
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \sa avl_tree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
template <typename _Range_Preprocess, typename _Range_Combine>
void avl_node<_Element, _Size, _Range_Type_Intermediate>::update(
    const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb) {
  size = _Size(1);
  subrange = _rpre(value);
  if (left != nullptr) {
    size = left->size + size;
    subrange = _rcomb(left->subrange, subrange);
  }
  if (right != nullptr) {
    size = size + right->size;
    subrange = _rcomb(subrange, right->subrange);
  }
}

//! Perform a left rotation on this subtree, and return the new root.
/*!
 * Performs a left rotation on this subtree.
 * Will also update the sizes and range intermediate values, so if you know a
 * tree rotation is needed, you can avoid explicitly calling update (the freshly
 * updated values would have been discarded anyway, and this method will make
 * the correct updates after performing the rotation)
 *
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \return the new subtree root
 * \sa avl_tree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
template <typename _Range_Preprocess, typename _Range_Combine>
avl_node<_Element, _Size, _Range_Type_Intermediate>
    *avl_node<_Element, _Size, _Range_Type_Intermediate>::rotate_left(
        const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb) {
  avl_node *pivot = this->right;
  this->right = pivot->left;
  pivot->left = this;
  this->balance -= 1 + std::max(char(0), pivot->balance);
  pivot->balance -= 1 - std::min(char(0), this->balance);
  this->update(_rpre, _rcomb);
  pivot->update(_rpre, _rcomb);
  return pivot;
}

//! Perform a right rotation on this subtree, and return the new root.
/*!
 * The mirrored version of rotate_left. See docs for rotate_left.
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
template <typename _Range_Preprocess, typename _Range_Combine>
avl_node<_Element, _Size, _Range_Type_Intermediate>
    *avl_node<_Element, _Size, _Range_Type_Intermediate>::rotate_right(
        const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb) {
  avl_node *pivot = this->left;
  this->left = pivot->right;
  pivot->right = this;
  this->balance += 1 - std::min(char(0), pivot->balance);
  pivot->balance += 1 + std::max(char(0), this->balance);
  this->update(_rpre, _rcomb);
  pivot->update(_rpre, _rcomb);
  return pivot;
}

//! Ensure this subtree is not right heavy, and rotates if needed. Returns the new root.
/*!
 * If the subtree is right heavy (right subtree is taller than left), does a
 * left rotation to make it not right heavy anymore.
 *
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \return the new subtree root
 * \sa avl_tree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
template <typename _Range_Preprocess, typename _Range_Combine>
avl_node<_Element, _Size, _Range_Type_Intermediate> *
avl_node<_Element, _Size, _Range_Type_Intermediate>::ensure_not_right_heavy(
    const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb) {
  if (this->balance <= 0) return this;
  return this->rotate_left(_rpre, _rcomb);
}

//! Ensure this subtree is not left heavy, and rotates if needed. Returns the new root.
/*!
 * Mirrored version of ensure_not_right_heavy.
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
template <typename _Range_Preprocess, typename _Range_Combine>
avl_node<_Element, _Size, _Range_Type_Intermediate>
    *avl_node<_Element, _Size, _Range_Type_Intermediate>::ensure_not_left_heavy(
        const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb) {
  if (this->balance >= 0) return this;
  return this->rotate_right(_rpre, _rcomb);
}

//! Knowing that this node's balance factor is 2 (very right heavy), rotate to correct the imbalance, and return the new root.
/*!
 * Rebalances the tree when the only imbalance is at this node and its balance
 * factor is 2. (overly right heavy)
 *
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \sa avl_tree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
template <typename _Range_Preprocess, typename _Range_Combine>
avl_node<_Element, _Size, _Range_Type_Intermediate>
    *avl_node<_Element, _Size, _Range_Type_Intermediate>::rebalance_right_heavy(
        const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb) {
  if (this->right != nullptr)
    this->right = this->right->ensure_not_left_heavy(_rpre, _rcomb);
  return this->rotate_left(_rpre, _rcomb);
}

//! Knowing that this node's balance factor is -2 (very left heavy), rotate to correct the imbalance, and return the new root.
/*!
 * Mirrored version of rebalance_right_heavy.
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
template <typename _Range_Preprocess, typename _Range_Combine>
avl_node<_Element, _Size, _Range_Type_Intermediate>
    *avl_node<_Element, _Size, _Range_Type_Intermediate>::rebalance_left_heavy(
        const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb) {
  if (this->left != nullptr)
    this->left = this->left->ensure_not_right_heavy(_rpre, _rcomb);
  return this->rotate_right(_rpre, _rcomb);
}

//! Get the element at a specific index in the subtree.
/*!
 * Get (a const reference to) the element at a specific index.
 *
 * \param node root of the subtree, which must not be null
 * \param node index to get the element of, in range [0, size of tree)
 * \return (a const reference to) the element at that index
 * \exception std::out_of_range If the requested index is outside the range [0, size of subtree)
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate>
const _Element&
avl_node_get_at_index(
    const avl_node<_Element, _Size, _Range_Type_Intermediate> *node, _Size index) {
  if (node == nullptr) [[unlikely]] {
    throw std::out_of_range(
      "AVL tree operation get at index tried to get from an empty "
      "subtree. This happens when the index is outside of the range of "
      "valid indices for this tree.");
  }
  _Size left_size = avl_node_size(node->left);
  if (index == left_size) {
    // at this node
    return node->value;
  } else if (index < left_size) {
    // on the left
    return avl_node_get_at_index(node->left, index);
  } else {
    // on the right
    return avl_node_get_at_index(node->left, index - (left_size + _Size(1)));
  }
}

//! Insert an element just before the given index in the subtree.
/**
 * Inserts the new element just at the given index.
 * To be specific, if there is some element already at that index,
 * then the new element will be directly before that other element.
 * To insert after the current last element, use (the size of the subtree)
 * as the index.
 *
 * The size of the subtree increases, unless a merge occurs,
 * in which case the size of the subtree stays the same.
 *
 * \param node the root of the subtree
 * \param index the index to insert at
 * \param value the value to be inserted at that index
 * \param _merge merge function
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \param _alloc allocator object
 * \return tuple: (new subtree root, whether it got taller)
 * \sa avl_tree
 * \exception std::out_of_range If the requested insertion index is outside the range [0, size of subtree + 1)
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate,
          typename _Merge, typename _Range_Preprocess, typename _Range_Combine,
          typename _Alloc>
std::pair<avl_node<_Element, _Size, _Range_Type_Intermediate> *, bool>
avl_node_insert_at_index(
    avl_node<_Element, _Size, _Range_Type_Intermediate> *node, _Size index,
    _Element value, const _Merge &_merge, const _Range_Preprocess &_rpre,
    const _Range_Combine &_rcomb, _Alloc _alloc) {
  // empty node special case
  if (node == nullptr) {
    // only valid index for empty tree is 0
    if (index != _Size(0)) [[unlikely]] {
      throw std::out_of_range(
        "AVL tree operation insert at index tried to insert before the"
        "first valid index or after the last valid index.");
    }
    node = _alloc.allocate(1);
    _alloc.construct(node, value, _rpre(value));
    return std::make_pair(node, true);
  }
  // attempt merge
  if (_merge(node->value, value)) {
    return std::make_pair(node, true);
  }
  // do regular insert
  _Size left_size = avl_node_size(node->left);
  if (index <= left_size) {
    auto partial = avl_node_insert_at_index(node->left, index, value, _merge,
                                            _rpre, _rcomb, _alloc);
    node->left = partial.first;
    bool taller = partial.second;
    node->balance -= taller;
    if (!taller || node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_pair(node, false);
    } else if (node->balance == -1) {
      node->update(_rpre, _rcomb);
      return std::make_pair(node, true);
    }
    return std::make_pair(node->rebalance_left_heavy(_rpre, _rcomb), false);
  } else {
    auto partial = avl_node_insert_at_index(
        node->right, index - (avl_node_size(node->left) + _Size(1)), value,
        _merge, _rpre, _rcomb, _alloc);
    node->right = partial.first;
    bool taller = partial.second;
    node->balance += taller;
    if (!taller || node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_pair(node, false);
    } else if (node->balance == 1) {
      node->update(_rpre, _rcomb);
      return std::make_pair(node, true);
    }
    return std::make_pair(node->rebalance_right_heavy(_rpre, _rcomb), false);
  }
}

//! Insert a new element in the subtree just after all elements that are less than it.
/*!
 * Insert a new element in the subtree just after all elements that are less than it.
 * As currently implemented, elements which are equivalent or incomparable are considered
 * greater than the new element being inserted
 * for purposes of determining where to insert,
 * though it is only required that the new value is inserted after all elements less than it
 * and before all elements greater than it.
 * It is required as a precondition that the subtree is already sorted in ascending order,
 * that is, for all pairs of elements, the left element is not greater than the right element.
 *
 * The size of the subtree increases, unless a merge occurs,
 * in which case the size of the subtree stays the same.
 *
 * One of the return values is the insertion index,
 * which is the index of the newly inserted value,
 * or if it was merged, the index of the merged value.
 *
 * \param node the root of the subtree
 * \param value the value to be inserted
 * \param _less less than function
 * \param _merge merge function
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \param _alloc allocator object
 * \return tuple: (new subtree root, whether it got taller, index of the inserted value)
 * \sa avl_tree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate,
          typename _Compare, typename _Merge, typename _Range_Preprocess,
          typename _Range_Combine, typename _Alloc>
std::tuple<avl_node<_Element, _Size, _Range_Type_Intermediate> *, bool, _Size>
avl_node_insert_ordered(
    avl_node<_Element, _Size, _Range_Type_Intermediate> *node, _Element value,
    const _Compare &_less, const _Merge &_merge, const _Range_Preprocess &_rpre,
    const _Range_Combine &_rcomb, _Alloc _alloc) {
  // empty node special case
  if (node == nullptr) {
    node = _alloc.allocate(1);
    _alloc.construct(node, value, _rpre(value));
    return std::make_tuple(node, true, 0);
  }
  // attempt merge
  if (_merge(node->value, value)) {
    return std::make_tuple(node, false, 0);
  }
  // insert normally
  if (!_less(node->value, value)) {
    auto partial = avl_node_insert_ordered(node->left, value, _less, _merge,
                                           _rpre, _rcomb, _alloc);
    node->left = std::get<0>(partial);
    bool taller = std::get<1>(partial);
    _Size index = std::get<2>(partial);
    node->balance -= taller;
    if (!taller || node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, index);
    } else if (node->balance == -1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, index);
    }
    return std::make_tuple(node->rebalance_left_heavy(_rpre, _rcomb), false,
                           index);
  } else {
    auto partial = avl_node_insert_ordered(node->right, value, _less, _merge,
                                           _rpre, _rcomb, _alloc);
    node->right = std::get<0>(partial);
    bool taller = std::get<1>(partial);
    _Size index = avl_node_size(node->left) + _Size(1) + std::get<2>(partial);
    node->balance += taller;
    if (!taller || node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, index);
    } else if (node->balance == 1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, index);
    }
    return std::make_tuple(node->rebalance_right_heavy(_rpre, _rcomb), false,
                           index);
  }
}

//! Remove a node at a specific index in the subtree.
/*!
 * Remove an element at a specific index, and return the element that was removed.
 *
 * \param node the root of the subtree
 * \param index the index of the element to remove
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \param _alloc allocator object
 * \return tuple: (new subtree root, whether it got shorter, the removed element)
 * \sa avl_tree
 * \exception std::out_of_range If the requested removal index is outside the range [0, size of subtree)
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate,
          typename _Range_Preprocess, typename _Range_Combine, typename _Alloc>
std::tuple<avl_node<_Element, _Size, _Range_Type_Intermediate> *, bool,
           _Element>
avl_node_remove_at_index(
    avl_node<_Element, _Size, _Range_Type_Intermediate> *node, _Size index,
    const _Range_Preprocess &_rpre, const _Range_Combine &_rcomb,
    _Alloc _alloc) {
  if (node == nullptr) [[unlikely]] {
      throw std::out_of_range(
          "AVL tree operation remove at index tried to remove from an empty "
          "subtree. This happens when the index is outside of the range of "
          "valid indices for this tree.");
    }
  _Size left_size = avl_node_size(node->left);
  if (index == left_size) {
    // we must delete this node
    _Element result = node->value;
    if (node->left == nullptr && node->right == nullptr) {
      _alloc.destroy(node);
      _alloc.deallocate(node, 1);
      return std::make_tuple(nullptr, true, result);
    }
    if (node->left == nullptr) {
      auto child = node->right;
      _alloc.destroy(node);
      _alloc.deallocate(node, 1);
      return std::make_tuple(child, true, result);
    }
    if (node->right == nullptr) {
      auto child = node->left;
      _alloc.destroy(node);
      _alloc.deallocate(node, 1);
      return std::make_tuple(child, true, result);
    }
    auto partial =
        avl_node_remove_at_index(node->right, 0, _rpre, _rcomb, _alloc);
    node->right = std::get<0>(partial);
    bool shorter = std::get<1>(partial);
    node->value = std::get<2>(partial);
    node->balance -= shorter;
    if (!shorter || node->balance == -1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, result);
    } else if (node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, result);
    }
    node = node->rebalance_left_heavy(_rpre, _rcomb);
    shorter = node->balance == 0;
    return std::make_tuple(node, shorter, result);
  } else if (index < left_size) {
    // it's on the left
    auto partial =
        avl_node_remove_at_index(node->left, index, _rpre, _rcomb, _alloc);
    node->left = std::get<0>(partial);
    bool shorter = std::get<1>(partial);
    _Element result = std::get<2>(partial);
    node->balance += shorter;
    if (!shorter || node->balance == 1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, result);
    } else if (node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, result);
    }
    node = node->rebalance_right_heavy(_rpre, _rcomb);
    shorter = node->balance == 0;
    return std::make_tuple(node, shorter, result);
  } else {
    // it's on the right
    auto partial = avl_node_remove_at_index(
        node->right, index - (left_size + _Size(1)), _rpre, _rcomb, _alloc);
    node->right = std::get<0>(partial);
    bool shorter = std::get<1>(partial);
    _Element result = std::get<2>(partial);
    node->balance -= shorter;
    if (!shorter || node->balance == -1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, result);
    } else if (node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, result);
    }
    node = node->rebalance_left_heavy(_rpre, _rcomb);
    shorter = node->balance == 0;
    return std::make_tuple(node, shorter, result);
  }
}

//! Attempt to remove 1 instance of an element from a sorted subtree.
/*!
 * Searches for 1 instance of an element within a sorted (non-decreasing) subtree,
 * and if it is found, removes it.
 * Exact equality using the == operator is required.
 * Note that if the search value is within a contiguous subrange of at least 2
 * incomparable elements, the search may be unable to locate it.
 * This is because the search assumes a total ordering, and will not bother
 * searching in an incomparable subrange.
 *
 * If you instead want to find and possibly remove an incomparable element which may
 * not be exactly equal, try using the search methods instead, which are meant for searching
 * rather than removing a very specific value.
 *
 * One of the return values is the index of the element (before it was removed).
 * If the remove was successful, that value will be the actual index,
 * otherwise, it will be the empty optional.
 *
 * \param node the root of the subtree
 * \param value the value to search for and remove
 * \param _less less than function
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \param _alloc allocator object
 * \return tuple: (new subtree root, whether it got shorter, optional: the index of the removed element)
 * \sa avl_tree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate,
          typename _Compare, typename _Range_Preprocess,
          typename _Range_Combine, typename _Alloc>
std::tuple<avl_node<_Element, _Size, _Range_Type_Intermediate> *, bool,
           avl_optional<_Size>>
avl_node_remove_ordered(
    avl_node<_Element, _Size, _Range_Type_Intermediate> *node, _Element value,
    const _Compare &_less, const _Range_Preprocess &_rpre,
    const _Range_Combine &_rcomb, _Alloc _alloc) {
  avl_optional<_Size> index;
  // empty node -> do nothing, report nothing to delete
  if (node == nullptr) {
    return std::make_tuple(node, false, index);
  }
  if (node->value == value) {
    index = avl_node_size(node->left);
    // we must delete this node
    if (node->left == nullptr && node->right == nullptr) {
      _alloc.destroy(node);
      _alloc.deallocate(node, 1);
      return std::make_tuple(nullptr, true, index);
    }
    if (node->left == nullptr) {
      auto child = node->right;
      _alloc.destroy(node);
      _alloc.deallocate(node, 1);
      return std::make_tuple(child, true, index);
    }
    if (node->right == nullptr) {
      auto child = node->left;
      _alloc.destroy(node);
      _alloc.deallocate(node, 1);
      return std::make_tuple(child, true, index);
    }
    auto partial =
        avl_node_remove_at_index(node->right, 0, _rpre, _rcomb, _alloc);
    node->right = std::get<0>(partial);
    bool shorter = std::get<1>(partial);
    node->value = std::get<2>(partial);
    node->balance -= shorter;
    if (!shorter || node->balance == -1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, index);
    } else if (node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, index);
    }
    node = node->rebalance_left_heavy(_rpre, _rcomb);
    shorter = node->balance == 0;
    return std::make_tuple(node, shorter, index);
  } else if (_less(value, node->value)) {
    // it's on the left
    auto partial = avl_node_remove_ordered(node->left, value, _less, _rpre,
                                           _rcomb, _alloc);
    node->left = std::get<0>(partial);
    bool shorter = std::get<1>(partial);
    index = std::get<2>(partial);
    if (!index) {
      // remove did nothing
      return std::make_tuple(node, false, index);
    }
    node->balance += shorter;
    if (!shorter || node->balance == 1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, index);
    } else if (node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, index);
    }
    node = node->rebalance_right_heavy(_rpre, _rcomb);
    shorter = node->balance == 0;
    return std::make_tuple(node, shorter, index);
  } else {
    // it's on the right
    auto partial = avl_node_remove_ordered(node->right, value, _less, _rpre,
                                           _rcomb, _alloc);
    node->right = std::get<0>(partial);
    bool shorter = std::get<1>(partial);
    index = std::get<2>(partial);
    if (!index) {
      // remove did nothing
      return std::make_tuple(node, false, index);
    }
    index = avl_node_size(node->left) + _Size(1) + index.value();
    node->balance -= shorter;
    if (!shorter || node->balance == -1) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, false, index);
    } else if (node->balance == 0) {
      node->update(_rpre, _rcomb);
      return std::make_tuple(node, true, index);
    }
    node = node->rebalance_left_heavy(_rpre, _rcomb);
    shorter = node->balance == 0;
    return std::make_tuple(node, shorter, index);
  }
}

//! Removes a node at the specified index in the subtree, then inserts the new element at that index.
/*!
 * Removes a node at the specified index, then inserts the new element at that index.
 *
 * The size of the subtree stays the same, unless a merge occurs, in which case
 * the size of the subtree decreases by 1.
 *
 * \param node the root of the subtree
 * \param index the index to replace at
 * \param new_value the new value to insert at that index
 * \param _merge merge function
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \param _alloc allocator object
 * \return tuple: (new subtree root, whether it got smaller)
 * \sa avl_tree
 * \exception std::out_of_range If the requested insertion index is outside the range [0, size of subtree)
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate,
          typename _Merge, typename _Range_Preprocess, typename _Range_Combine,
          typename _Alloc>
std::pair<avl_node<_Element, _Size, _Range_Type_Intermediate> *, bool>
avl_node_replace_at_index(
    avl_node<_Element, _Size, _Range_Type_Intermediate> *node, _Size index,
    _Element new_value, const _Merge &_merge, const _Range_Preprocess &_rpre,
    const _Range_Combine &_rcomb, _Alloc _alloc) {
    auto old_size = avl_node_size(node);
    auto remove_result = avl_node_remove_at_index(node, index, _rpre, _rcomb, _alloc);
    node = std::get<0>(remove_result);
    auto insert_result = avl_node_insert_at_index(node, index, new_value, _merge, _rpre, _rcomb, _alloc);
    node = std::get<0>(insert_result);
    auto new_size = avl_node_size(node);
    bool did_merge = old_size != new_size;
    return std::make_pair(node, did_merge);
}

/**
 * If the requested old value exists in the tree, replace it by the
 * new value, otherwise, do nothing.
 * The re-insertion may cause a merge, in which case the tree size decreasees by 1.
 * Return tuple is (new subtree root, whether a replacement was made,
 * whether the merge was successful).
 */
//! Make a single replacement in the subtree if the value exists, otherwise do nothing.
/**
 * Search for the old element within the subtree, assuming sorted order,
 * and remove it if it exists. If the removal was successful, then inserts the new element
 * in the subtree according to the order. The removal index and insertion index are not necessarily the same.
 * Size of the subtree remains the same, unless replacement occurs and merge occurs.
 *
 * One of the return values is an optional tuple of the removal index and the insertion index.
 * It exists if the replacement is successful.
 * Note that if the insertion index is less than or equal to the removal index and no merge occurs,
 * the actual location where the removal occured is shifted to the right by 1.
 * This function includes correction for this shifting, and reports this actual location, instead of
 * where in the original subtree the removal occurred.
 *
 * \param node the root of the subtree
 * \param old_value the old value to search for and replace
 * \param new_value the new value to insert at that index
 * \param _less less than function
 * \param _merge merge function
 * \param _rpre range preprocess function
 * \param _rcomb range combine function
 * \param _alloc allocator object
 * \return tuple: (new subtree root, whether it got smaller, optional: (removal index, insertion index))
 * \sa avl_tree
 */
template <typename _Element, typename _Size, typename _Range_Type_Intermediate,
          typename _Compare, typename _Merge,
          typename _Range_Preprocess, typename _Range_Combine,
          typename _Alloc>
std::tuple<avl_node<_Element, _Size, _Range_Type_Intermediate> *, bool, avl_optional<std::pair<_Size,_Size>>>
avl_node_replace_ordered(
    avl_node<_Element, _Size, _Range_Type_Intermediate> *node, _Element old_value,
    _Element new_value, const _Compare &_less,
    const _Merge &_merge, const _Range_Preprocess &_rpre,
    const _Range_Combine &_rcomb, _Alloc _alloc) {
    auto old_size = avl_node_size(node);
    auto remove_result = avl_node_remove_ordered(node, old_value, _less, _rpre, _rcomb, _alloc);
    avl_optional<_Size> remove_index = std::get<2>(remove_result);
    avl_optional<std::pair<_Size,_Size>> index_result;
    // if remove failed, do nothing
    if(!remove_index){
      return std::make_tuple(node, false, index_result);
    }
    node = std::get<0>(remove_result);
    auto insert_result = avl_node_insert_ordered(node, new_value, _less, _merge, _rpre, _rcomb, _alloc);
    node = std::get<0>(insert_result);
    auto new_size = avl_node_size(node);
    bool did_merge = old_size != new_size;
    _Size corrected_remove_index = *remove_index;
    _Size insert_index = std::get<2>(insert_result);
    corrected_remove_index += _Size(insert_index <= corrected_remove_index && !did_merge);
    index_result = std::make_pair(corrected_remove_index, insert_index);
    return std::make_tuple(node, did_merge, index_result);
}

// the avl tree class

//! The AVL tree class, the most basic and extensible data structure in the public API.
/*!
 * The AVL tree class which is actually exposed to the user and encapsulates a lot of the
 * messy details involved in the implementation. The base extendable data structure on top
 * of which more useful objects are built.
 *
 * To demonstrate use of the range queries, consider an implementation of a simple list
 * which also wants O(log N) hashing of sublists. We will use range queries here to implement only hashing.
 * Whatever the element type, we will use range preprocess as
 * X --> (hash(X), 1). Intermediate values will be tuples of (hash for this range, number of elements in the range).
 * Hashes will be combined using the Rabin hash, so this is the range combine:
 * (U, N), (V, M) --> (P ^ N * U + V, N + M) for some odd multiplier P, using ^ to denote exponentiation.
 * Finally, we want to return a single number as the hash, so the hash postprocess will simply drop the element count:
 * (U, N) --> U
 *
 * \tparam _Element The type of the element stored in the tree collection.
 * To differentiate it from other data, you may think of the "elements" as the original data.
 * Without the elements, it's just a plain tree and not useful.
 * \tparam _Element_Compare A class for a function that takes 2 elements and returns true if the
 * left argument is less than the right argument. You will only actually need this for methods
 * which care about the ordering, otherwise, you don't need to have this actually implemented at all.
 * As a consequence of how templating works in C++, if you never use those ordered functions,
 * the type-specific code will never be generated, so the compiler won't notice that the comparison function supplied
 * doesn't actually work.
 * In the future, with good reason, a major update
 * may change this to use a 3-way comparison function.
 * \tparam _Size The integer or integer-like type used for indices and sizes.
 * Recommended to use std::size_t (which is the default) unless you have a good reason
 * to use another type.
 * \tparam _Merge A class for a function that takes 2 element references, and tries to merge the right
 * argument into the left argument. It will either return false and leave both arguments unchanged, or return true and
 * combine the right argument with the left argument in some way. The actual behaviour of the merge depends
 * on what merger you use, and this will depend on your needs. The left argument is kept in place, and if the merge
 * is successful, the right argument will be discarded, so a destructive merge operation is okay.
 * \tparam _Range_Preprocess A class for a function, designated the range preprocess function.
 * For range operations, all elements will individually be mapped first through this function,
 * to get an object of type _Range_Type_Intermediate.
 * Often necessary because the intermediate value type differs from the element type
 * and some conversion work is needed.
 * \tparam _Range_Type_Intermediate The type of intermediate results of range operations.
 * \tparam _Range_Combine A class for a function, designated the range combine function.
 * Represents the binary operator that combines 2 _Range_Type_Intermediate values into 1.
 * Assumed to be associative, otherwise, the result of a range operation is ill defined.
 * Only requires associativity, not commutativity or invertability.
 * \tparam _Range_Postprocess A class for a function, designated the range postprocess function.
 * After computing the intermediate value for a range, that intermediate value is mapped through the
 * range postprocess function to get the final result of the range query.
 * A typical use of this is to drop information that is only relevant for intermediate values.
 * \tparam _Alloc The allocator class for the nodes, which will be used for managing dynamic memory in place
 * of using new and delete. By default, is the default allocator, which actually has the same behaviour as new and delete.
 * If you want more control over how the nodes, you can change this.
 */
template <typename _Element, typename _Element_Compare = std::less<_Element>,
          typename _Size = std::size_t, typename _Merge = no_merge<_Element>,
          typename _Range_Preprocess = monostate,
          typename _Range_Type_Intermediate =
              avl_invoke_result(_Range_Preprocess, _Element),
          typename _Range_Combine = std::plus<_Range_Type_Intermediate>,
          typename _Range_Postprocess = identity<_Range_Type_Intermediate>,
          typename _Alloc = std::allocator<
              avl_node<_Element, _Size, _Range_Type_Intermediate>>>
class avl_tree {
 private:
  avl_node<_Element, _Size, _Range_Type_Intermediate> *root;
  [[no_unique_address]] _Element_Compare _less;
  [[no_unique_address]] _Merge _merge;
  [[no_unique_address]] _Range_Preprocess _rpre;
  [[no_unique_address]] _Range_Combine _rcomb;
  [[no_unique_address]] _Range_Postprocess _rpost;
  [[no_unique_address]] _Alloc _alloc;

 public:
  avl_tree();
  ~avl_tree();
  std::size_t size();
  _Element get_item(std::size_t);
  std::result_of<_Range_Postprocess(_Range_Type_Intermediate)> get_range(
      std::size_t, std::size_t);
  void insert(std::size_t, _Element);
  _Element remove(std::size_t);
  _Element replace(std::size_t, _Element);
};

}  // namespace avl

#undef avl_invoke_result
#undef avl_optional

#endif

// TODO remove test main when we're sure it compiles and runs fine
// the test main is only to check if the API works at all, it's not a comprehensive unit test
// it is useful right now for spotting big errors during development
#include <iostream>
int main() {
  // c++ version
  std::cout << __cplusplus << std::endl;
  // test node instantiation
  // (300)
  avl::avl_node<int, int, int> *node =
      new avl::avl_node<int, int, int>(300, 300);
  std::cout << avl::avl_node_size(node) << " (expected 1)" << std::endl;
  // test some insertion by index
  // (100 300)
  node = avl::avl_node_insert_at_index(
             node, 0, 100, avl::no_merge<int>(), avl::identity<int>(),
             std::plus<int>(), std::allocator<avl::avl_node<int, int, int>>())
             .first;
  std::cout << avl::avl_node_size(node) << " (expected 2)" << std::endl;
  // test some insertion ordered
  // (100 100 300)
  node = std::get<0>(avl::avl_node_insert_ordered(
      node, 100, std::less<int>(), avl::no_merge<int>(), avl::identity<int>(),
      std::plus<int>(), std::allocator<avl::avl_node<int, int, int>>()));
  std::cout << avl::avl_node_size(node) << " (expected 3)" << std::endl;
  // test some removal
  // (100 300)
  node = std::get<0>(avl::avl_node_remove_at_index(
      node, 1, avl::identity<int>(), std::plus<int>(),
      std::allocator<avl::avl_node<int, int, int>>()));
  std::cout << avl::avl_node_size(node) << " (expected 2)" << std::endl;
  // test some removal ordered
  // (100)
  node = std::get<0>(avl::avl_node_remove_ordered(
      node, 300, std::less<int>(), avl::identity<int>(), std::plus<int>(),
      std::allocator<avl::avl_node<int, int, int>>()));
  std::cout << avl::avl_node_size(node) << " (expected 1)" << std::endl;
  // test some element get
  // (100)
  std::cout << avl::avl_node_get_at_index(node, 0) << " (expected 100)" << std::endl;
  // test some element replace by index
  // (150)
  node = std::get<0>(avl::avl_node_replace_at_index(
      node, 0, 150, avl::no_merge<int>(), avl::identity<int>(),
      std::plus<int>(), std::allocator<avl::avl_node<int, int, int>>()
      ));
  std::cout << avl::avl_node_get_at_index(node, 0) << " (expected 150)" << std::endl;
  std::cout << avl::avl_node_size(node) << " (expected 1)" << std::endl;
  // test some element replace ordered
  // (150)
  node = std::get<0>(avl::avl_node_replace_ordered(
      node, 250, 350, std::less<int>(), avl::no_merge<int>(), avl::identity<int>(),
      std::plus<int>(), std::allocator<avl::avl_node<int, int, int>>()
      ));
  std::cout << avl::avl_node_get_at_index(node, 0) << " (expected 150)" << std::endl;
  std::cout << avl::avl_node_size(node) << " (expected 1)" << std::endl;
  node = std::get<0>(avl::avl_node_replace_ordered(
      node, 150, 350, std::less<int>(), avl::no_merge<int>(), avl::identity<int>(),
      std::plus<int>(), std::allocator<avl::avl_node<int, int, int>>()
      ));
  std::cout << avl::avl_node_get_at_index(node, 0) << " (expected 350)" << std::endl;
  std::cout << avl::avl_node_size(node) << " (expected 1)" << std::endl;
}
