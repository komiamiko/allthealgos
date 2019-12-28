/*
 * An AVL tree implementation and some common collection types based on it.
 * Includes drop-in replacements for vector, set, multiset, and map.
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
#include <type_traits>
#include <memory>

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
        template <typename T> monostate operator () (const T&) const;
    };

    monostate::monostate() {}
    template <typename T> monostate::monostate(const T& t) {}
    template <typename T> monostate monostate::operator () (const T&) const {return monostate();}
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
     * Identity function. It's in std::functional as of C++20, but it's currently 2019 and support is still on its way. For the time being, we use this as a drop-in replacement.
     * It's semantically not exactly the same (this one is jankier for sure) but it does the job.
     */
    template <typename T> struct identity {
        const T& operator() (const T& value) const noexcept { return value; }
    };

    /**
     * One of the basic mergers: never merges
     */
    template <typename T> struct no_merge {
        const bool operator () (T&, const T&) const;
    };

    template <typename T> const bool no_merge<T>::operator () (T& to, const T& from) const {
        return false;
    }

    /**
     * One of the basic mergers: merge if equal
     */
    template <typename T> struct merge_if_equal {
        const bool operator () (T&, const T&) const;
    };

    template <typename T> const bool merge_if_equal<T>::operator () (T& to, const T& from) const {
        return to == from;
    }

    /**
     * One of the less basic mergers: if equal, merge counter
     */
    template <typename T, typename C> struct merge_count {
        const bool operator () (std::pair<T,C>&, const std::pair<T,C>&) const;
    };

    template <typename T, typename C> const bool merge_count<T,C>::operator () (std::pair<T,C>& to, const std::pair<T,C>& from) const {
        if(to != from)return false;
        to.second += from.second;
        return true;
    }

    template <
    typename _Element,
    typename _Size = std::size_t,
    typename _Range_Type_Intermediate = monostate
    > class avl_node;

    // forward declarations for helper functions

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    > _Size avl_node_size(avl_node<_Element,_Size,_Range_Type_Intermediate>* node);

    template <
    typename _Element_2,
    typename _Size_2,
    typename _Range_Type_Intermediate_2,
    typename _Merge,
    typename _Range_Preprocess,
    typename _Range_Combine,
    typename _Alloc
    > std::pair<avl_node<
    _Element_2,
    _Size_2,
    _Range_Type_Intermediate_2
    >*, bool> avl_node_insert_at_index(avl_node<
    _Element_2,
    _Size_2,
    _Range_Type_Intermediate_2>*, _Size_2, _Element_2, const _Merge&, const _Range_Preprocess&, const _Range_Combine&, _Alloc);

    template <
    typename _Element_2,
    typename _Size_2,
    typename _Range_Type_Intermediate_2,
    typename _Compare,
    typename _Merge,
    typename _Range_Preprocess,
    typename _Range_Combine,
    typename _Alloc
    > std::tuple<avl_node<
    _Element_2,
    _Size_2,
    _Range_Type_Intermediate_2
    >*, bool, _Size_2> avl_node_insert_ordered(avl_node<
    _Element_2,
    _Size_2,
    _Range_Type_Intermediate_2>*, _Element_2, const _Compare&, const _Merge&, const _Range_Preprocess&, const _Range_Combine&, _Alloc);

    // declaration for avl_node

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
        avl_node(const _Element& i_value, const _Range_Type_Intermediate& i_subrange) {
            left = nullptr;
            value = i_value;
            right = nullptr;
            size = _Size(1);
            balance = char(0);
            subrange = i_subrange;
        }
        
        // these helper functions are friends
        
        template <
    typename _Element_2,
    typename _Size_2,
    typename _Range_Type_Intermediate_2
    > friend _Size_2 avl::avl_node_size(avl_node<
    _Element_2,
    _Size_2,
    _Range_Type_Intermediate_2
    >*);
        
        template <
        typename _Element_2,
        typename _Size_2,
        typename _Range_Type_Intermediate_2,
        typename _Merge,
        typename _Range_Preprocess,
        typename _Range_Combine,
        typename _Alloc
        > friend std::pair<avl_node<
        _Element_2,
        _Size_2,
        _Range_Type_Intermediate_2
        >*, bool> avl::avl_node_insert_at_index(avl_node<
        _Element_2,
        _Size_2,
        _Range_Type_Intermediate_2>*, _Size_2, _Element_2, const _Merge&, const _Range_Preprocess&, const _Range_Combine&, _Alloc);
        
        template <
        typename _Element_2,
        typename _Size_2,
        typename _Range_Type_Intermediate_2,
        typename _Compare,
        typename _Merge,
        typename _Range_Preprocess,
        typename _Range_Combine,
        typename _Alloc
        > friend std::tuple<avl_node<
        _Element_2,
        _Size_2,
        _Range_Type_Intermediate_2
        >*, bool, _Size_2> avl::avl_node_insert_ordered(avl_node<
        _Element_2,
        _Size_2,
        _Range_Type_Intermediate_2>*, _Element_2, const _Compare&, const _Merge&, const _Range_Preprocess&, const _Range_Combine&, _Alloc);
        
        // these are our methods
        
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > void update(const _Range_Preprocess&,const _Range_Combine&);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rotate_left(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rotate_right(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* ensure_not_right_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* ensure_not_left_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rebalance_right_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb);
        template <
        typename _Range_Preprocess,
        typename _Range_Combine
        > avl_node* rebalance_left_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb);
    };

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
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
            size = left->size + size;
            subrange = _rcomb(left->subrange, subrange);
        }
        if(right != nullptr){
            size = size + right->size;
            subrange = _rcomb(subrange, right->subrange);
        }
    }

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    >
    template <
    typename _Range_Preprocess,
    typename _Range_Combine
    > avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >* avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >::rotate_left(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb) {
        avl_node* pivot = this->right;
        this->right = pivot->left;
        pivot->left = this;
        this->balance -= 1 + std::max(char(0), pivot->balance);
        pivot->balance -= 1 - std::min(char(0), this->balance);
        this->update(_rpre, _rcomb);
        pivot->update(_rpre, _rcomb);
        return pivot;
    }

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    >
    template <
    typename _Range_Preprocess,
    typename _Range_Combine
    > avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >* avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >::rotate_right(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb) {
        avl_node* pivot = this->left;
        this->left = pivot->right;
        pivot->right = this;
        this->balance += 1 - std::min(char(0), pivot->balance);
        pivot->balance += 1 + std::max(char(0), this->balance);
        this->update(_rpre, _rcomb);
        pivot->update(_rpre, _rcomb);
        return pivot;
    }

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    >
    template <
    typename _Range_Preprocess,
    typename _Range_Combine
    > avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >* avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >::ensure_not_right_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb) {
        if(this->balance<=0)return this;
        return this->rotate_left(_rpre,_rcomb);
    }

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    >
    template <
    typename _Range_Preprocess,
    typename _Range_Combine
    > avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >* avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >::ensure_not_left_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb) {
        if(this->balance>=0)return this;
        return this->rotate_right(_rpre,_rcomb);
    }

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    >
    template <
    typename _Range_Preprocess,
    typename _Range_Combine
    > avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >* avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >::rebalance_right_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb) {
        if(this->right != nullptr)this->right = this->right->ensure_not_left_heavy(_rpre, _rcomb);
        return this->rotate_left(_rpre, _rcomb);
    }

    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate
    >
    template <
    typename _Range_Preprocess,
    typename _Range_Combine
    > avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >* avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >::rebalance_left_heavy(const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb) {
        if(this->left != nullptr)this->left = this->left->ensure_not_right_heavy(_rpre, _rcomb);
        return this->rotate_right(_rpre, _rcomb);
    }

    /**
     * Inserts the element just before the element at a specific index.
     * May merge, in which case the tree does not grow.
     * Return tuple is (new root, whether it got taller)
     */
    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate,
    typename _Merge,
    typename _Range_Preprocess,
    typename _Range_Combine,
    typename _Alloc
    > std::pair<avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >*, bool> avl_node_insert_at_index(avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >* node, _Size index, _Element value,
                              const _Merge& _merge,
                              const _Range_Preprocess& _rpre,
                              const _Range_Combine& _rcomb,
                            _Alloc _alloc) {
        // empty node special case
        if(node == nullptr){
            node = _alloc.allocate(1);
            _alloc.construct(node, value,_rpre(value));
            return std::make_pair(node, true);
        }
        // attempt merge
        if(_merge(node->value,value)){
            return std::make_pair(node, true);
        }
        // do regular insert
        _Size left_size = avl_node_size(node->left);
        if(index <= left_size){
            auto partial = avl_node_insert_at_index(node->left,index,value,_merge,_rpre,_rcomb,_alloc);
            node->left = partial.first;
            bool taller = partial.second;
            node->balance -= taller;
            if(!taller || node->balance==0){
                node->update(_rpre,_rcomb);
                return std::make_pair(node,false);
            }else if(node->balance==-1){
                node->update(_rpre,_rcomb);
                return std::make_pair(node,true);
            }
            return std::make_pair(node->rebalance_left_heavy(_rpre, _rcomb),false);
        }else{
            auto partial = avl_node_insert_at_index(node->right,index-(avl_node_size(node->left)+1),value,_merge,_rpre,_rcomb,_alloc);
            node->right = partial.first;
            bool taller = partial.second;
            node->balance += taller;
            if(!taller || node->balance==0){
                node->update(_rpre,_rcomb);
                return std::make_pair(node,false);
            }else if(node->balance==1){
                node->update(_rpre,_rcomb);
                return std::make_pair(node,true);
            }
            return std::make_pair(node->rebalance_right_heavy(_rpre, _rcomb),false);
        }
    }

    /**
     * Inserts the element where it belongs using the given less than function (compare).
     * This could be done in separate stages with finding the insertion index and then inserting there,
     * but a fused find and insert is faster.
     * Insertion index will be the leftmost possible, so it will be to the left of all identical values.
     * Return type is (new root, whether it got taller, insertion index used)
     */
    template <
    typename _Element,
    typename _Size,
    typename _Range_Type_Intermediate,
    typename _Compare,
    typename _Merge,
    typename _Range_Preprocess,
    typename _Range_Combine,
    typename _Alloc
    > std::tuple<avl_node<
    _Element,
    _Size,
    _Range_Type_Intermediate
    >*, bool, _Size> avl_node_insert_ordered(avl_node<
    _Element,
    _Size, _Range_Type_Intermediate>* node, _Element value, const _Compare& _less, const _Merge& _merge, const _Range_Preprocess& _rpre, const _Range_Combine& _rcomb, _Alloc _alloc) {
        // empty node special case
        if(node == nullptr){
            node = _alloc.allocate(1);
            _alloc.construct(node, value,_rpre(value));
            return std::make_tuple(node, true, 0);
        }
        // attempt merge
        if(_merge(node->value,value)){
            return std::make_tuple(node, false, 0);
        }
        // insert normally
        if(!_less(node->value,value)){
            auto partial = avl_node_insert_ordered(node->left,value,_less,_merge,_rpre,_rcomb,_alloc);
            node->left = std::get<0>(partial);
            bool taller = std::get<1>(partial);
            _Size index = std::get<2>(partial);
            node->balance -= taller;
            if(!taller || node->balance==0){
                node->update(_rpre,_rcomb);
                return std::make_tuple(node,false,index);
            }else if(node->balance==-1){
                node->update(_rpre,_rcomb);
                return std::make_tuple(node,true,index);
            }
            return std::make_tuple(node->rebalance_left_heavy(_rpre, _rcomb),false,index);
        }else{
            auto partial = avl_node_insert_ordered(node->right,value,_less,_merge,_rpre,_rcomb,_alloc);
            node->right = std::get<0>(partial);
            bool taller = std::get<1>(partial);
            _Size index = avl_node_size(node->left) + 1 + std::get<2>(partial);
            node->balance += taller;
            if(!taller || node->balance==0){
                node->update(_rpre,_rcomb);
                return std::make_tuple(node,false,index);
            }else if(node->balance==1){
                node->update(_rpre,_rcomb);
                return std::make_tuple(node,true,index);
            }
            return std::make_tuple(node->rebalance_right_heavy(_rpre, _rcomb),false,index);
        }
    }

    // the avl tree class

    template <
    typename _Element,
    typename _Element_Compare = std::less<_Element>,
    typename _Size = std::size_t,
    typename _Merge = no_merge<_Element>,
    typename _Range_Preprocess = monostate,
    typename _Range_Type_Intermediate = std::result_of<_Range_Preprocess(_Element)>,
    typename _Range_Combine = std::plus<_Range_Type_Intermediate>,
    typename _Range_Postprocess = identity<_Range_Type_Intermediate>,
    typename _Alloc = std::allocator<avl_node<_Element,_Size,_Range_Type_Intermediate>>
    > class avl_tree {
    private:
        avl_node<
        _Element,
        _Size,
        _Range_Type_Intermediate
        > *root;
        avl_tree();
        ~avl_tree();
        std::size_t size();
        _Element get_item(std::size_t);
        std::result_of<_Range_Postprocess(_Range_Type_Intermediate)> get_range(std::size_t, std::size_t);
        void insert(std::size_t, _Element);
        _Element remove(std::size_t);
        _Element replace(std::size_t, _Element);
    };

}

#endif

// TODO remove test main when we're sure it compiles and runs fine
#include <iostream>
int main(){
    // test node instantiation
    avl::avl_node<int,int,int>* node = new avl::avl_node<int,int,int>(300,300);
    std::cout << avl::avl_node_size(node) << std::endl;
    // test some insertion by index
    node = avl::avl_node_insert_at_index(node, 0, 100, avl::no_merge<int>(), avl::identity<int>(), std::plus<int>(), std::allocator<avl::avl_node<int,int,int>>()).first;
    std::cout << avl::avl_node_size(node) << std::endl;
    // test some insertion ordered
    node = std::get<0>(avl::avl_node_insert_ordered(node, 100, std::less<int>(), avl::no_merge<int>(), avl::identity<int>(), std::plus<int>(), std::allocator<avl::avl_node<int,int,int>>()));
    std::cout << avl::avl_node_size(node) << std::endl;
}
