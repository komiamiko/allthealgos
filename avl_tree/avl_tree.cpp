/*
 * An AVL tree implementation and some common collection types based on it.
 * Included collections and what they can be used as drop-in replacements for:
 * - List (vector)
 * - Set (set)
 * - Bag (multiset)
 * - Map (map)
 * This AVL tree implementation can support additional features optionally:
 * - Indexable
 * - Ordered
 * - Range queries
 * Note that the compiler may require you implement certain things even if
 * that code will never be run.
 * How to use range queries:
 * - Elements will first individually be preprocessed, then combined left to right, then the result is postprocessed.
 * - Combine must be associative.
 * How to use merge:
 * - Merge will either merge 2 entries and return true or do nothing and return false.
 * - Left argument to merge is the "merge target", and will be kept if merged. The right argument would be discarded.
 * Possible weird behaviour:
 * - Indexing relies on the same stuff as sizing, so if the tree is not made indexable,
 *   you also will not know its size
 * Also note that various operations are assumed to be O(1) such as the range combine.
 * If the complexity is not O(1), it's up to you to determine the actual complexity in any complexity analysis.
 */

#ifndef _AVL_TREE_H
#define _AVL_TREE_H

#include <algorithm>
#include <functional>

namespace avl {

    /**
     * The empty 0 sized struct.
     * There's lots of reimplementations of this struct out there,
     * and this one defines various operations for completeness.
     * In mathematical terms, this can be modeled as just 0,
     * which when combined with itself produces 0.
     */
    struct monostate {
        monostate();
        template <typename T> monostate(const T&);
        template <typename T> monostate operator () (const T&);
    };

    monostate::monostate() {}
    template <typename T> monostate::monostate(const T& t) {}
    template <typename T> monostate monostate::operator () (const T&) {return monostate();}
    bool constexpr operator == (const monostate& lhs, const monostate& rhs) noexcept {return true;}
    bool constexpr operator >= (const monostate& lhs, const monostate& rhs) noexcept {return true;}
    bool constexpr operator <= (const monostate& lhs, const monostate& rhs) noexcept {return true;}
    bool constexpr operator != (const monostate& lhs, const monostate& rhs) noexcept {return false;}
    bool constexpr operator < (const monostate& lhs, const monostate& rhs) noexcept {return false;}
    bool constexpr operator > (const monostate& lhs, const monostate& rhs) noexcept {return false;}
    monostate const operator + (const monostate& lhs, const monostate& rhs) noexcept {return monostate();}
    monostate const operator - (const monostate& lhs, const monostate& rhs) noexcept {return monostate();}
    monostate const operator * (const monostate& lhs, const monostate& rhs) noexcept {return monostate();}
    monostate const operator | (const monostate& lhs, const monostate& rhs) noexcept {return monostate();}
    monostate const operator & (const monostate& lhs, const monostate& rhs) noexcept {return monostate();}

    /**
     * One of the basic mergers: never merges
     */
    struct no_merge {
        template <typename T> bool operator () (T&, const T&);
    };

    template <typename T> bool no_merge::operator () (T& to, const T& from) {
        return false;
    }

    /**
     * One of the basic mergers: merge if equal
     */
    struct merge_if_equal {
        template <typename T> bool operator () (T&, const T&);
    };

    template <typename T> bool merge_if_equal::operator () (T& to, const T& from) {
        return to == from;
    }

    /**
     * One of the less basic mergers: if equal, merge counter
     */
    struct merge_count {
        template <typename T, typename C> bool operator () (std::pair<T,C>&, const std::pair<T,C>&);
    };

    template <typename T, typename C> bool merge_count::operator () (std::pair<T,C>& to, const std::pair<T,C>& from) {
        if(to != from)return false;
        to.second += from.second;
        return true;
    }

    template <
    typename _Element,
    typename _Size = std::size_t,
    typename _Range_Type_Intermediate = monostate
    > class avl_node;

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    > _Size avl_node_size(avl_node<_Element,_Size,_Range_Type_Intermediate>* node);

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    > class avl_node {
    private:
        avl_node *left;
        [[no_unique_address]] _Element value;
        avl_node *right;
        [[no_unique_address]] _Size size;
        char balance;
        [[no_unique_address]] _Range_Type_Intermediate subrange;
    public:
        friend _Size avl::avl_node_size(avl_node*);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > void update(const _Range_Preprocess&,const _Range_Combine&);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rotate_left();
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rotate_right();
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* ensure_not_right_heavy();
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* ensure_not_left_heavy();
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rebalance_right_heavy();
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rebalance_left_heavy();
    };

    template <
    typename _Element,
    typename _Size = std::size_t,
    typename _Range_Type_Intermediate = monostate
    > _Size avl_node_size(avl_node<_Element,_Size,_Range_Type_Intermediate>* node) {
        if(node == nullptr)return 0;
        return node->size;
    }

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    >
    template <
    typename _Range_Preprocess,
    typename _Range_Combine
    > void avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >::update(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb) {
        size = _Size(1);
        subrange = _rpre(value);
        if(left != nullptr){
            size += left->size;
            subrange = _rcomb(left->subrange, subrange);
        }
        if(right != nullptr){
            size += right->size;
            subrange = _rcomb(subrange, right->subrange);
        }
    }

    template <
    typename _Element,
    typename _Element_Compare = std::less<_Element>,
    typename _Size = std::size_t,
    typename _Size_Compare = std::less<_Size>,
    typename _Merge = no_merge,
    typename _Range_Preprocess = monostate,
    typename _Range_Type_Intermediate = monostate,
    typename _Range_Combine = std::plus<_Range_Type_Intermediate>,
    typename _Range_Postprocess = monostate,
    typename _Alloc = std::allocator<_Element>
    > class avl_tree {
    private:
        avl_node<
        _Element,
        _Size,
        _Range_Type_Intermediate
        > *root;
    };

}

#endif
