#pragma once
#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>

#ifndef SYS_PROG_BS_TREE_H
#define SYS_PROG_BS_TREE_H

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BS_tree final : private compare
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:

    static constexpr const size_t minimum_keys_in_node = 2 * t - 1;
    static constexpr const size_t maximum_keys_in_node = 3 * t - 1;

    static constexpr const size_t minimum_keys_in_root = 1;
    static constexpr const size_t maximum_keys_in_root = 4 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    bool less(tkey a, tkey b) { return compare::operator()(a, b); }
    bool greater(tkey a, tkey b) { return compare::operator()(b, a); }
    bool equal(tkey a, tkey b) {return !less(a, b) && !greater(a, b); }
    bool not_equal(tkey a, tkey b) { return !equal(a, b); }
    bool less_or_equal(tkey a, tkey b) {return less(a, b) || equal(a, b); }
    bool greater_or_equal(tkey a, tkey b) {return greater(a, b) || equal(a, b); }

    // endregion comparators declaration

    struct bstree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_root + 1> _keys;
        boost::container::static_vector<bstree_node*, maximum_keys_in_root + 2> _pointers;
        bstree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    bstree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;


    // region helper functions for five

    bstree_node* clone_node(bstree_node* node);
    void delete_node(bstree_node* node) noexcept;

    // endregion

    // region helper functions for iterators

    template<typename ptr_to_ptr_to_node>
    static void increment_iterator_inner(std::stack<std::pair<ptr_to_ptr_to_node, size_t>>&path, size_t& index);

    template<typename ptr_to_ptr_to_node>
    static void decrement_iterator_inner(std::stack<std::pair<ptr_to_ptr_to_node, size_t>>&path, size_t& index);

    std::stack<std::pair<bstree_node**, size_t>> path_begin();
    std::stack<std::pair<bstree_node**, size_t>> path_reverse_begin();

    // endregion


public:

    // region debug functions declaration

    struct debug_bstree_node
    {
        std::vector<tree_data_type> _keys;
        std::vector<debug_bstree_node*> _pointers;

        explicit debug_bstree_node(const bstree_node* node)
        {
            // Копируем ключи
            _keys.assign(node->_keys.begin(), node->_keys.end());

            // Копируем указатели (рекурсивно конвертируем детей)
            for (auto* child : node->_pointers) {
                _pointers.push_back(new debug_bstree_node(child));
            }
        }

        ~debug_bstree_node() {
            for (auto* ptr : _pointers) {
                delete ptr;
            }
        }
    };

    static std::vector<debug_bstree_node*> vector_pointers(
        const boost::container::static_vector<bstree_node*, maximum_keys_in_node + 2>& _pointers)
    {
        std::vector<debug_bstree_node*> result;
        result.reserve(_pointers.size());

        for (auto* node_ptr : _pointers) {
            if (node_ptr) {
                result.push_back(new debug_bstree_node(node_ptr));
            } else {
                result.push_back(nullptr);
            }
        }

        return result;
    }

    void print_tree();
    void print_node(bstree_node* node, int depth = 0);

    // endregion debug functions declaration

    // region constructors declaration

    explicit BS_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit BS_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BS_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    BS_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    BS_tree(const BS_tree& other);

    BS_tree(BS_tree&& other) noexcept;

    BS_tree& operator=(const BS_tree& other);

    BS_tree& operator=(BS_tree&& other) noexcept;

    ~BS_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class bstree_iterator;
    class bstree_reverse_iterator;
    class bstree_const_iterator;
    class bstree_const_reverse_iterator;

    class bstree_iterator final
    {
        std::stack<std::pair<bstree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bstree_iterator;

        friend class BS_tree;
        friend class bstree_reverse_iterator;
        friend class bstree_const_iterator;
        friend class bstree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        [[nodiscard]] size_t depth() const noexcept;
        [[nodiscard]] size_t current_node_keys_count() const noexcept;
        [[nodiscard]] bool is_terminate_node() const noexcept;
        [[nodiscard]] size_t index() const noexcept;

        explicit bstree_iterator(const std::stack<std::pair<bstree_node**, size_t>>& path = std::stack<std::pair<bstree_node**, size_t>>(), size_t index = 0);

    };

    class bstree_const_iterator final
    {
        std::stack<std::pair<bstree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bstree_const_iterator;

        friend class BS_tree;
        friend class bstree_reverse_iterator;
        friend class bstree_iterator;
        friend class bstree_const_reverse_iterator;

        bstree_const_iterator(const bstree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        [[nodiscard]] size_t depth() const noexcept;
        [[nodiscard]] size_t current_node_keys_count() const noexcept;
        [[nodiscard]] bool is_terminate_node() const noexcept;
        [[nodiscard]] size_t index() const noexcept;

        explicit bstree_const_iterator(const std::stack<std::pair<bstree_node* const*, size_t>>& path = std::stack<std::pair<bstree_node* const*, size_t>>(), size_t index = 0);
    };

    class bstree_reverse_iterator final
    {
        std::stack<std::pair<bstree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bstree_reverse_iterator;

        friend class BS_tree;
        friend class bstree_iterator;
        friend class bstree_const_iterator;
        friend class bstree_const_reverse_iterator;

        bstree_reverse_iterator(const bstree_iterator& it) noexcept;
        operator bstree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        [[nodiscard]] size_t depth() const noexcept;
        [[nodiscard]] size_t current_node_keys_count() const noexcept;
        [[nodiscard]] bool is_terminate_node() const noexcept;
        [[nodiscard]] size_t index() const noexcept;

        explicit bstree_reverse_iterator(const std::stack<std::pair<bstree_node**, size_t>>& path = std::stack<std::pair<bstree_node**, size_t>>(), size_t index = 0);
    };

    class bstree_const_reverse_iterator final
    {
        std::stack<std::pair<bstree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bstree_const_reverse_iterator;

        friend class BS_tree;
        friend class bstree_reverse_iterator;
        friend class bstree_const_iterator;
        friend class bstree_iterator;

        bstree_const_reverse_iterator(const bstree_reverse_iterator& it) noexcept;
        operator bstree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        [[nodiscard]] size_t depth() const noexcept;
        [[nodiscard]] size_t current_node_keys_count() const noexcept;
        [[nodiscard]] bool is_terminate_node() const noexcept;
        [[nodiscard]] size_t index() const noexcept;

        explicit bstree_const_reverse_iterator(const std::stack<std::pair<bstree_node* const*, size_t>>& path = std::stack<std::pair<bstree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class bstree_iterator;
    friend class bstree_const_iterator;
    friend class bstree_reverse_iterator;
    friend class bstree_const_reverse_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

    bstree_iterator begin();
    bstree_iterator end();

    bstree_const_iterator begin() const;
    bstree_const_iterator end() const;

    bstree_const_iterator cbegin() const;
    bstree_const_iterator cend() const;

    bstree_reverse_iterator rbegin();
    bstree_reverse_iterator rend();

    bstree_const_reverse_iterator rbegin() const;
    bstree_const_reverse_iterator rend() const;

    bstree_const_reverse_iterator crbegin() const;
    bstree_const_reverse_iterator crend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    bstree_iterator find(const tkey& key);
    bstree_const_iterator find(const tkey& key) const;

    bstree_iterator lower_bound(const tkey& key);
    bstree_const_iterator lower_bound(const tkey& key) const;

    bstree_iterator upper_bound(const tkey& key);
    bstree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<bstree_iterator, bool> insert(const tree_data_type& data);
    std::pair<bstree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<bstree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    bstree_iterator insert_or_assign(const tree_data_type& data);
    bstree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    bstree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    bstree_iterator erase(bstree_iterator pos);
    bstree_iterator erase(bstree_const_iterator pos);

    bstree_iterator erase(bstree_iterator beg, bstree_iterator en);
    bstree_iterator erase(bstree_const_iterator beg, bstree_const_iterator en);


    bstree_iterator erase(const tkey& key);

    // endregion modifiers declaration

private:
    // region helper functions for insert declaration
    bstree_iterator search_terminate_node_to_insert(tkey key);
    void rebalancing_after_insert(std::stack<std::pair<bstree_node**, size_t>>&path);
    void split(bstree_node**left, bstree_node**right, bstree_node**parent, size_t index_split_parent);
    void handle_rebalancing_root();

    // endregion helper functions for insert declaration

    // helper functions for erase declaration

    void erase_from_internal_node(std::stack<std::pair<bstree_node**, size_t>>& path, size_t index);
    void erase_from_leaf(std::stack<std::pair<bstree_node**, size_t>>& path, size_t index);

    void merge(bstree_node** left, bstree_node** node, bstree_node** right, bstree_node** parent, size_t parent_index);
    void merge_with_root();

    void rebalancing_after_erase(std::stack<std::pair<bstree_node**, size_t>>& path);

    bool is_right_brother_exist(bstree_node** parent, size_t index);
    bool is_left_brother_exist(size_t index);
    bool is_node_has_more_minimum_keys(bstree_node** node);
    bool is_node_miss_elements(bstree_node** node);

    void train_from_right_brother(bstree_node** node, bstree_node** right_brother, bstree_node** parent, size_t parent_index);
    void train_from_left_brother(bstree_node** node, bstree_node** left_brother, bstree_node** parent, size_t parent_index);

    void train_from_right_right_brother(bstree_node** node, bstree_node** right_brother,
                    bstree_node** right_right_brother, bstree_node** parent, size_t parent_index);
    void train_from_left_left_brother(bstree_node** node, bstree_node** left_brother,
                    bstree_node** left_left_brother, bstree_node** parent, size_t parent_index);

    void push_front_element_to_parent(bstree_node** node, bstree_node** parent, size_t parent_index);
    void pull_element_to_back_from_parent(bstree_node** node, bstree_node** parent, size_t parent_index);

    void push_back_element_to_parent(bstree_node** node, bstree_node** parent, size_t parent_index);
    void pull_element_to_front_from_parent(bstree_node** node, bstree_node** parent, size_t parent_index);

    void move_child_from_right_brother(bstree_node** node, bstree_node** right_brother);
    void move_child_from_left_brother(bstree_node** node, bstree_node** left_brother);

    void move_child_from_right_right_brother(bstree_node** node, bstree_node** right_brother, bstree_node** right_right_brother);
    void move_child_from_left_left_brother(bstree_node** node, bstree_node** left_brother, bstree_node** left_left_brother);

    void push_back_child_to_parent(bstree_node* node, bstree_node** parent, size_t parent_index);
    void push_front_child_to_parent(bstree_node* node, bstree_node** parent, size_t parent_index);

    // endregion

};

// region base constructors and getters impl

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
BS_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BS_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BS_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BS_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BS_tree<tkey, tvalue, compare, t>::value_type> BS_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept
{
    return _allocator;
}

// endregion base constructors impl

// region comparators impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::compare_pairs(const BS_tree::tree_data_type &lhs,
                                                     const BS_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}

// endregion comparators impl

// region debug functions impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::print_tree() {
    if (_root) print_node(_root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::print_node(bstree_node* node, int depth) {
    std::cout << std::string(depth, ' ');

    std::cout << '[';
    for (int i = 0; i < node->_keys.size(); ++i) {
        std::cout << (node->_keys[i].first);
        if (i != node->_keys.size() - 1) std::cout << "|";
    }
    std::cout << "]\n" << std::flush;

    if (!node->_pointers.empty()) {
        std::cout << "(" << node->_keys[0].first << ')';
        print_node(node->_pointers[0], depth + 1);

        for (int i = 0; i < node->_keys.size(); ++i) {
            if (node->_pointers[i + 1]) {
                std::cout << '(' << node->_keys[i].first << ')';
                print_node(node->_pointers[i + 1], depth + 1);
            }
        }
    }

}

// endregion debug functions impl

// region bstree_node constructor impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_node::bstree_node() noexcept
{
    _keys = boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1>{};
    _pointers = boost::container::static_vector<bstree_node*, maximum_keys_in_node + 2>{};
}

// endregion bstree_node constructor impl

// region helper functions for iterators impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<class ptr_to_ptr_to_node>
void BS_tree<tkey, tvalue, compare, t>::increment_iterator_inner(
    std::stack<std::pair<ptr_to_ptr_to_node, size_t>>& path, size_t& index)
{
    if (path.empty())
        return;

    auto [node, parent_index] = path.top();
    /* пробуем опуститься в самого левого ребенка */
    if ( !(*node)->_pointers.empty()) {
        while ( !(*node)->_pointers.empty() ) {
            node = &((*node)->_pointers[index + 1]);
            path.emplace(node, index + 1);
            index = -1;
        }
        index = 0;
        return;
    }

    /* мы в листе, просто сдвигаемся вправо */
    if (index < (*node)->_keys.size() - 1) {
        ++index;
        return;
    }

    /* мы в листе, в самом последнем элементе, поднимаемся наверх */
    path.pop();
    if (!path.empty()) {
        auto [parent, parent_parent_index] = path.top();
        while (!path.empty() && parent_index == (*parent)->_keys.size() ) {
            /* поднимаемся, пока не окажемся не в последнем элементе */
            path.pop();
            parent_index = parent_parent_index;
            if (!path.empty()) std::tie(parent, parent_parent_index) = path.top();
        }
        if (!path.empty()) {
            index = parent_index;
            return;
        }
    }

    /* мы поднялись настолько, что оказались в корне, в последнем элементе, значит мы в конце */
    index = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename ptr_to_ptr_to_node>
void BS_tree<tkey, tvalue, compare, t>::decrement_iterator_inner(
    std::stack<std::pair<ptr_to_ptr_to_node, size_t>>& path, size_t& index)
{
    if (path.empty())
        return;

    if (index == (*path.top().first)->_keys.size()) /* если мы в самом конце узла,
                                                     * просто переходим к предыдущему элементу в этой ноде */
    {
        if (!(*path.top().first)->_keys.empty()) index--;
        return;
    }

    if ((*path.top().first)->_pointers.empty()) // если нет детей
    {
        if (index > 0) // мы не дошли еще до первого элемента в узле
        {
            index--;
            return;
        }

        auto path_tmp = path;
        auto child = path_tmp.top();
        path_tmp.pop();

        while (!path_tmp.empty())
        {
            if (child.second > 0) /* child.second - это индекс сына в родителе
                                   * если он не равен 0, то значит еще есть соседние элементы слева
                                   * и мы можем сдвинуться */
            {
                path = path_tmp;
                index = child.second - 1;
                return;
            }

            child = path_tmp.top();
            path_tmp.pop();
        }

        index = 0;
        return;
    }

    auto child = &((*path.top().first)->_pointers[index]);
    path.push({ child, index });
    while (!((*child)->_pointers.empty()))
    {
        size_t child_index = (*child)->_pointers.size() - 1;
        child = &((*child)->_pointers[child_index]);
        path.push({ child, child_index });
    }

    index = (*child)->_keys.size() - 1;
}


// endregion helper functions for iterators impl

// region common iterator impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::bstree_iterator(
        const std::stack<std::pair<bstree_node**, size_t>>& path, size_t index)
            : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::reference
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::pointer
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator++()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator--()
{
    decrement_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator==(const self& other) const noexcept
{
    if (!_path.empty() && !other._path.empty()) {
        return _path.top() == other._path.top() && _index == other._index;
    }
    if (_path.empty() && other._path.empty()) {
        return _index == other._index;
    }
    return false;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;

    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_iterator::is_terminate_node() const noexcept
{
    auto [node, _] = _path.top();
    return (*node)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_iterator::index() const noexcept
{
    return _index;
}

// endregion common iterator impl

// region const iterator impl
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::bstree_const_iterator(
    // ReSharper disable once CppParameterMayBeConst
    const std::stack<std::pair<bstree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) {};

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::bstree_const_iterator(
        const bstree_iterator& it) noexcept
{
    std::stack<std::pair<bstree_node* const*, size_t>> tmp;
    auto copy_path = it._path;

    while (!copy_path.empty()) {
        tmp.push(copy_path.top());
        copy_path.pop();
    }

    while (!tmp.empty()) {
        _path.push(tmp.top());
        tmp.pop();
    }
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::reference
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::pointer
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator++()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator--()
{
    decrement_iterator_inner(_path, _index);
    return *this;

}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator==(const self& other) const noexcept
{
    if (!_path.empty() && !other._path.empty()) {
        return _path.top() == other._path.top() && _index == other._index;
    }
    if (_path.empty() && other._path.empty()) {
        return _index == other._index;
    }
    return false;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;

    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::is_terminate_node() const noexcept
{
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator::index() const noexcept
{
    return _index;
}
// endregion const iterator impl

// region reverse iterator impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::bstree_reverse_iterator(
    // ReSharper disable once CppParameterMayBeConst
    const std::stack<std::pair<bstree_node**, size_t>>& path, size_t index) : _path(path), _index(index) {};

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::bstree_reverse_iterator(
        const bstree_iterator& it) noexcept
{
    _path = it._path;
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator BS_tree<tkey, tvalue, compare, t>::bstree_iterator() const noexcept
{
    return bstree_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::reference
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::pointer
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator++()
{
    decrement_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator--()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator==(const self& other) const noexcept
{
    if (!_path.empty() && !other._path.empty()) {
        return _path.top() == other._path.top() && _index == other._index;
    }
    if (_path.empty() && other._path.empty()) {
        return _index == other._index;
    }
    return false;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::is_terminate_node() const noexcept
{
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion reverse iterator impl

// region reverse const iterator impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::bstree_const_reverse_iterator(
    // ReSharper disable once CppParameterMayBeConst
    const std::stack<std::pair<bstree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) {};

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::bstree_const_reverse_iterator(
        const bstree_reverse_iterator& it) noexcept
{
    std::stack<std::pair<bstree_node* const*, size_t>> tmp;
    auto copy_stack = it._path;

    while (!copy_stack.empty()) {
        tmp.push(copy_stack.top());
        copy_stack.pop();
    }
    while (!tmp.empty()) {
        _path.push(tmp.top());
        tmp.pop();
    }
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator() const noexcept
{
    return bstree_const_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::reference
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::pointer
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator++()
{
    decrement_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator&
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator--()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator==(const self& other) const noexcept
{
    if (!_path.empty() && !other._path.empty()) {
        return _path.top() == other._path.top() && _index == other._index;
    }
    if (_path.empty() && other._path.empty()) {
        return _index == other._index;
    }
    return false;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::is_terminate_node() const noexcept
{
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion reverse const iterator impl

// region constructors impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::BS_tree(
        const compare& cmp,
        pp_allocator<value_type> alloc)
            : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::BS_tree(
        pp_allocator<value_type> alloc,\
        const compare& comp)
            : compare(comp), _allocator(alloc), _root(nullptr), _size(0) {}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BS_tree<tkey, tvalue, compare, t>::BS_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc)
            : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto it = begin; it != end; ++it) {
        insert(*it);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::BS_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc)
            : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto item : data) {
        insert(item);
    }
}

// endregion constructors impl

// region five impl

// region helper functions for five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_node*
BS_tree<tkey, tvalue, compare, t>::clone_node(bstree_node* node) {
    if (!node) return nullptr;

    auto* new_node = new bstree_node();
    new_node->_keys = node->_keys;
    new_node->_pointers.resize(node->_pointers.size());
    for (size_t i = 0; i < node->_pointers.size(); ++i) {
        new_node->_pointers[i] = clone_node(node->_pointers[i]);
    }

    return new_node;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::delete_node(bstree_node *node) noexcept {
    if (!node) return;

    for (auto child : node->_pointers) {
        delete_node(child);
    }
    _allocator.template delete_object<bstree_node>(node);
}

// endregion helper functions for five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::BS_tree(const BS_tree& other) : compare(other), _allocator(other._allocator), _root(nullptr), _size(other._size)
{
    _root = clone_node(other._root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::BS_tree(BS_tree&& other) noexcept
    : compare(std::move(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size)
{
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>& BS_tree<tkey, tvalue, compare, t>::operator=(const BS_tree& other)
{
    if (this != &other) {
        static_cast<compare&>(*this) = static_cast<compare&>(other);
        _root = clone_node(other._root);
        _allocator = other._allocator;
        _size = other._size;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>& BS_tree<tkey, tvalue, compare, t>::operator=(BS_tree&& other) noexcept
{
    if (this != &other) {
        delete_node(_root);

        static_cast<compare&>(*this) = std::move(other);
        _allocator = std::move(other._allocator);
        _size = other._size;
        _root = other._root;

        other._root = nullptr;
        other._size = 0;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::~BS_tree() noexcept
{
    clear();
}

// endregion five impl

// region element access impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BS_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("Такого ключа не существует");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& BS_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("Такого ключа не существует");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BS_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto iterator = find(key);
    if (iterator == end()) iterator = insert({key, tvalue {}});

    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BS_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    return operator[](std::move(key));
}

// endregion element access impl

// region iterator begins impl

// region helper functions for iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
// ReSharper disable once CppRedundantTypenameKeyword
std::stack<std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_node**, size_t>>
BS_tree<tkey, tvalue, compare, t>::path_begin() {
    if (_root == nullptr) return {};

    std::stack<std::pair<bstree_node**, size_t>> path;
    path.emplace(&_root, 0);

    auto** node = &_root;
    while (!(*node)->_pointers.empty()) {
        node = &((*node)->_pointers[0]);
        path.emplace(node, 0);
    }

    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
// ReSharper disable once CppRedundantTypenameKeyword
std::stack<std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_node**, size_t>>
BS_tree<tkey, tvalue, compare, t>::path_reverse_begin() {
    if (_root == nullptr) return {};

    std::stack<std::pair<bstree_node**, size_t>> path;
    path.emplace(&_root, 0);

    auto** node = &_root;
    while (!(*node)->_pointers.empty()) {
        auto index_last_child = (*node)->_pointers.size() - 1;

        node = &((*node)->_pointers[index_last_child]);
        path.emplace(node, index_last_child);
    }

    return path;
}

// endregion helper functions for iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::begin()
{
    if (_root == nullptr) return end();

    return bstree_iterator(path_begin(), 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::end()
{
    return bstree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator BS_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator BS_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator BS_tree<tkey, tvalue, compare, t>::cbegin() const
{
    return bstree_const_iterator(const_cast<BS_tree*>(this)->begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator BS_tree<tkey, tvalue, compare, t>::cend() const
{
    return bstree_const_iterator(const_cast<BS_tree*>(this)->end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator BS_tree<tkey, tvalue, compare, t>::rbegin()
{
    if (!_root) return rend();

    auto path = path_reverse_begin();
    auto [node_ptr_ptr, _] = path.top();
    auto* node_ptr = *node_ptr_ptr;

    return bstree_reverse_iterator(path, node_ptr->_keys.size() - 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_reverse_iterator BS_tree<tkey, tvalue, compare, t>::rend()
{
    return bstree_reverse_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator BS_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return crbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator BS_tree<tkey, tvalue, compare, t>::rend() const
{
    return crend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator BS_tree<tkey, tvalue, compare, t>::crbegin() const
{
    return bstree_const_reverse_iterator(const_cast<BS_tree*>(this)->rbegin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_reverse_iterator BS_tree<tkey, tvalue, compare, t>::crend() const
{
    return bstree_const_reverse_iterator(const_cast<BS_tree*>(this)->rend());
}
// endregion iterator begins impl

// region lookup impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BS_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return size() == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    if (!_root) return end();

    int index = 0;
    bstree_node** node = &_root;
    std::stack<std::pair<bstree_node**, size_t>> path;

    while (node != nullptr) {
        path.emplace(node, index);

        int l = 0;
        int r = (*node)->_keys.size();

        while (l + 1 < r) {
            int m = (l + r) / 2;

            if (less_or_equal((*node)->_keys[m].first, key)) {
                l = m;
            }
            else {
                r = m;
            }
        }

        if (equal((*node)->_keys[l].first, key)) return bstree_iterator(path, l);

        if ((*node)->_pointers.empty() || less(key, (*node)->_keys[0].first)) --l; /* нужно перейти в самого первого сына
                                                                                    * или мы дошли до листа */

        node = (*node)->_pointers.empty() ? nullptr : &((*node)->_pointers[l + 1]);
        index = l + 1;
    }

    node = path.top().first;
    if (equal((*node)->_keys[index].first, key)) return bstree_iterator(path, index);
    return end(); /* дошли до конца и не нашли */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator BS_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    return b_tree_const_iterator(const_cast<BS_tree*>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    if (!_root) return end();

    int index = 0;
    bstree_node** node = &_root;
    std::stack<std::pair<bstree_node**, size_t>> path;

    while (node != nullptr) {
        path.emplace(node, index);

        int l = 0;
        int r = (*node)->_keys.size();

        while (l + 1 < r) {
            int m = (l + r) / 2;

            if (less_or_equal((*node)->_keys[m].first, key)) {
                l = m;
            }
            else {
                r = m;
            }
        }

        if (equal((*node)->_keys[l].first, key)) return bstree_iterator(path, l);

        if ((*node)->_pointers.empty() || less(key, (*node)->_keys[0].first)) --l; /* нужно перейти в самого первого сына
                                                                                    * или мы дошли до листа */

        node = (*node)->_pointers.empty() ? nullptr : &((*node)->_pointers[l + 1]);
        index = l + 1;
    }

    auto iterator = bstree_iterator(path, index);
    /* в случае (index == 0 && key < (*iterator).first) мы уже находимся на элементе, который больше нашего */
    if ((*iterator).first == key || (index == 0 && key < (*iterator).first)) return iterator;
    return ++iterator; /* следующий элемент */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator BS_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    return bstree_const_iterator(const_cast<BS_tree*>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    auto iterator = lower_bound(key);

    if (iterator == end()) return iterator;
    auto node = *iterator;
    if (node.first == key) return ++iterator;
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_const_iterator BS_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    return bstree_const_iterator(const_cast<BS_tree*>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}


// endregion lookup impl

// region modifiers impl

// region helper functions for insert impl

/**
 * Ищет место (лист), в которое надо вставить новый узел
 *
 * @param key Ключ, по которому нужно найти место
 * @return Возвращает итератор на то место, куда надо вставить узел. Возвращает end() в случае, если дерево пустое.
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::search_terminate_node_to_insert(tkey key) {
    auto iterator = lower_bound(key);

    if (iterator == end() && _root != nullptr) {
        /* дерево не пустое, но нужно вставить в последний элемент */
        auto path = path_reverse_begin();
        auto [node, _] = path.top();
        auto index = (*node)->_keys.size();
        return bstree_iterator(path_reverse_begin(), index);
    }
    if (iterator == end()) return iterator; /* дерево пустое */

    /* случай, когда элемент был последним в узле, lower_bound вернул ++iterator, который теперь указывает на внутренний узел,
     * но нам нужно пойти именно назад, причем в индекс самый последний (по аналогии с предыдущим аналогом)!
     */
    if (iterator != begin() && !iterator.is_terminate_node() && key < iterator->first) {
        --iterator;
        auto path = iterator._path;
        auto [node, _] = path.top();
        auto index = (*node)->_keys.size();
        return bstree_iterator(path, index);
    }
    /* теперь нужно спуститься в лист */
    /* итератор всегда пойдет сначала в лист, в самый первый элемент, это нам и надо */
    while (!iterator.is_terminate_node()) ++iterator;

    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::rebalancing_after_insert(std::stack<std::pair<bstree_node**, size_t>>& path) {
    /* уже вставили в узел, который мог переполниться */

    auto [node, parent_index] = path.top();
    path.pop();

    if ( (*node)->_keys.size() <= maximum_keys_in_node ) return; /* балансировка не нужна */

    if ( (*node) == _root && (*node)->_keys.size() <= maximum_keys_in_root) return; /* балансировка не нужна */
    if (*node == _root) {
        /* балансировка нужна корню */
        handle_rebalancing_root();
        return;
    }

    auto [parent, parent_parent_index] = path.top();
    /* хотим отдать правому брату */
    if (parent_index < (*parent)->_pointers.size() - 1) {
        auto right_brother = (*parent)->_pointers[parent_index + 1];

        if (right_brother->_keys.size() < maximum_keys_in_node) {
            /*
             * удаляем последний элемент из node,
             * добавляем его в родителя по индексу, между текущим узлом и его правым братом (parent_index),
             * добавляем старый элемент родителя, как первый элемент в правого брата
             */

            auto last_element_node = (*node)->_keys.back(); /* запомнили элемент из текущего узла */
            (*node)->_keys.pop_back();

            auto element_from_parent = (*parent)->_keys[parent_index]; /* запомнили элемент из родителя */
            (*parent)->_keys[parent_index] = last_element_node; /* заменяем элемент в родителе */

            right_brother->_keys.insert( right_brother->_keys.begin(), element_from_parent ); /* добавили в брата элемент из родителя */

            /* если мы находимся во внутренних узлах */
            if (!((*node)->_pointers.empty())) {
                auto last_child_node = (*node)->_pointers.back();
                (*node)->_pointers.pop_back();

                right_brother->_pointers.insert(right_brother->_pointers.begin(), last_child_node);
            }

            return;
        }
    }

    /* правому брату отдать не получилось, пытаемся отдать левому брату */
    if (parent_index > 0) {
        auto left_brother = (*parent)->_pointers[parent_index - 1];

        if (left_brother->_keys.size() < maximum_keys_in_node) {
            /*
             * удаляем первый элемент из node,
             * добавляем его в родителя по индексу, который разделяем текущий узел и левого брата
             * старый узел из родителя добавляем как последний элемент в левого брата
             *
             * переприсваиваем детей по случаю
             */

            auto first_element_node = (*node)->_keys.front();
            (*node)->_keys.erase((*node)->_keys.begin());

            auto element_from_parent = (*parent)->_keys[parent_index - 1];
            (*parent)->_keys[parent_index - 1] = first_element_node;

            left_brother->_keys.push_back(element_from_parent);

            /* если находимся во внутреннем узле */
            if (!((*node)->_pointers.empty())) {
                auto first_child_node = (*node)->_pointers.front();
                (*node)->_pointers.erase((*node)->_pointers.begin());

                left_brother->_pointers.push_back(first_child_node);
            }

            return;
        }
    }

    /* братья сами переполнены, поэтому пытаемся создать из двух узлов три */
    /* пытаемся соединиться с правым братом */
    if (parent_index < (*parent)->_pointers.size() - 1) {
        auto right_brother = (*parent)->_pointers[parent_index + 1];

        split(node, &right_brother, parent, parent_index);
        rebalancing_after_insert(path);
        return;
    }

    /* пытаемся соединиться с левым братом */
    if (parent_index > 0) {
        auto left_brother = (*parent)->_pointers[parent_index - 1];

        split(&left_brother, node, parent, parent_index - 1);
        rebalancing_after_insert(path);
        return;
    }

    throw std::logic_error("У узла нет соседей и это не корень!");
}


/**
 * В корне произошло переполнение, эта функция делит корень на 3 узла
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::handle_rebalancing_root() {
    auto index_middle_element = _root->_keys.size() / 2;

    auto new_root = get_allocator().template new_object<bstree_node>();
    auto second_node = get_allocator().template new_object<bstree_node>();
    auto first_node = _root;

    new_root->_keys.push_back(_root->_keys[index_middle_element]);
    new_root->_pointers.push_back(_root);
    new_root->_pointers.push_back(second_node);

    _root = new_root;

    second_node->_keys.insert(
        second_node->_keys.begin(), first_node->_keys.begin() + index_middle_element + 1, first_node->_keys.end());
    /* удаляем еще и средний элемент */
    first_node->_keys.erase(first_node->_keys.begin() + index_middle_element, first_node->_keys.end());

    if (!first_node->_pointers.empty()) {
        second_node->_pointers.insert(
            second_node->_pointers.begin(), first_node->_pointers.begin() + index_middle_element + 1, first_node->_pointers.end());
        first_node->_pointers.erase(first_node->_pointers.begin() + index_middle_element + 1, first_node->_pointers.end());
    }
}

/**
 * Разделяет два узла left и right на три узла left, middle, right
 *
 * @param left Узел, который находится слева
 * @param right Узел, который находится справа
 * @param parent Узел-родитель
 * @param index_split_parent Индекс элемента в родителе, который находится между left и right
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::split(bstree_node**left, bstree_node**right, bstree_node**parent, size_t index_split_parent) {
    /* разделяющий элемент в родителе */
    auto parent_split_element = (*parent)->_keys[index_split_parent];
    (*parent)->_keys.erase((*parent)->_keys.begin() + index_split_parent);

    constexpr auto index_split_element_node = 2 * t; /* элемент разделитель с первого узла */
    auto first_element_to_add_parent = (*left)->_keys[index_split_element_node];

    /*
     * в случае, если переполнен левый узел, то тогда правый разделитель имеет индекс t - 1
     * в случае, если переполнен правый узел, то тогда правый разделитель имеет индекс t
     */
    auto index_split_element_right = t; /* элемент разделитель со второго узла */
    if ((*left)->_keys.size() >= maximum_keys_in_node + 1) index_split_element_right = t - 1;

    auto second_element_to_add_parent = (*right)->_keys[index_split_element_right];

    /* удаляем после всего, чтобы не нарушилась проверка на индекс правого элемента */
    (*left)->_keys.erase((*left)->_keys.begin() + index_split_element_node);
    (*right)->_keys.erase((*right)->_keys.begin() + index_split_element_right);

    auto middle_node = get_allocator().template new_object<bstree_node>();

    /* заполняем средний узел */
    int i = index_split_element_node;
    for (int _ = index_split_element_node; _ < (*left)->_keys.size(); ++_) {
        /* не изменяется переменная i, потому что мы делаем erase и все элементы сдвигаются */
        auto element = (*left)->_keys[i];
        (*left)->_keys.erase((*left)->_keys.begin() + i);
        middle_node->_keys.push_back(element);

        if (!((*left)->_pointers.empty())) {
            auto child = (*left)->_pointers[i];
            middle_node->_pointers.push_back(child);
            (*left)->_pointers.erase((*left)->_pointers.begin() + i);
        }
    }
    /* добавляем последнего ребенка узла в средний узел, если он есть */
    if (!((*left)->_pointers.empty())) {
        auto child = (*left)->_pointers.back();
        middle_node->_pointers.push_back(child);
        (*left)->_pointers.pop_back();
    }

    /* добавляем родительский узел */
    middle_node->_keys.push_back(parent_split_element);

    /* добавляем первого ребенка брата в средний узел, если он есть */
    if (!(*right)->_pointers.empty()) {
        auto child = (*right)->_pointers.front();
        middle_node->_pointers.push_back(child);
        (*right)->_pointers.erase((*right)->_pointers.begin());
    }
    i = 0;
    for (int _ = 0; _ < index_split_element_right; ++_) {
        /* аналогично как и выше, переменная i не изменяется, потому что мы выполняем erase и элементы сдвигаются */
        auto element = (*right)->_keys[i];
        (*right)->_keys.erase((*right)->_keys.begin() + i);
        middle_node->_keys.push_back(element);

        if (!((*right)->_pointers.empty())) {
            auto child = (*right)->_pointers[i];
            middle_node->_pointers.push_back(child);
            (*right)->_pointers.erase((*right)->_pointers.begin() + i);
        }
    }

    /* заполняем родителя */
    (*parent)->_keys.insert((*parent)->_keys.begin() + index_split_parent, first_element_to_add_parent);
    (*parent)->_keys.insert((*parent)->_keys.begin() + index_split_parent + 1, second_element_to_add_parent);

    /* распределяем детей */
    (*parent)->_pointers.insert((*parent)->_pointers.begin() + index_split_parent + 1, middle_node);
}

// endregion

// region helper function for erase

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::erase_from_internal_node(std::stack<std::pair<bstree_node**, size_t>>& path, size_t index) {
    auto [node, _] = path.top();

    /* пытаемся найти самый левый элемент в правом поддереве */
    if (index < (*node)->_keys.size()) {
        auto most_left_element_right_subtree = (*node)->_pointers[index + 1];
        path.emplace(&most_left_element_right_subtree, index + 1);
        while (!most_left_element_right_subtree->_pointers.empty()) {
            most_left_element_right_subtree = most_left_element_right_subtree->_pointers[0];
            path.emplace(&most_left_element_right_subtree, 0);
        }

        (*node)->_keys[index] = most_left_element_right_subtree->_keys[0]; /* замена */

        erase_from_leaf(path, 0);
        return;
    }

    /* пытаемся найти самый правый элемент в левом поддереве */
    if (index != 0) {
        auto most_right_element_left_subtree = (*node)->_pointers[index - 1];
        path.emplace(&most_right_element_left_subtree, index - 1);
        while (!most_right_element_left_subtree->_pointers.empty()) {
            auto index_last_child = most_right_element_left_subtree->_pointers.size() - 1;

            most_right_element_left_subtree = most_right_element_left_subtree->_pointers[index_last_child];
            path.emplace(&most_right_element_left_subtree, index_last_child);
        }

        auto index_last_element = most_right_element_left_subtree->_keys.size() - 1;
        (*node)->_keys[index] = most_right_element_left_subtree->_keys[index_last_element]; /* замена */

        erase_from_leaf(path, index_last_element);
        return;
    }

    throw std::logic_error("Проблема при удалении внутреннего узла (нет потомков)");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::erase_from_leaf(std::stack<std::pair<bstree_node**,size_t>>&path, size_t index) {
    auto [node, _] = path.top();

    (*node)->_keys.erase((*node)->_keys.begin() + index);
    rebalancing_after_erase(path);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::rebalancing_after_erase(std::stack<std::pair<bstree_node**, size_t>>& path) {
    auto [node, parent_index] = path.top();
    path.pop();

    if (*node == _root) {
        if ((*node)->_keys.empty()) {
            delete_node(_root);
            _root = nullptr;
        }
        return;
    }

    if ((*node)->_keys.size() >= minimum_keys_in_node) return; /* балансировка не нужна */

    auto [parent, _] = path.top();
    /* пробуем занять у правого брата */
    if (is_right_brother_exist(parent, parent_index)) {
        auto right_brother = &(*parent)->_pointers[parent_index + 1];
        if (is_node_has_more_minimum_keys(right_brother)) {
            /* можно занять у правого брата */
            train_from_right_brother(node, right_brother, parent, parent_index);
            if (!(*node)->_pointers.empty()) {
                /* если есть дети у узла, то они должны быть и у правого брата */
                move_child_from_right_brother(node, right_brother);
            }
            return;
        }


        /* занять у правого брата не получилось, пробуем у правого двоюродного брата */
        if (is_right_brother_exist(parent, parent_index + 1)) {
            auto right_right_brother = &(*parent)->_pointers[parent_index + 1 + 1];
            if (is_node_has_more_minimum_keys(right_right_brother)) {
                /* можно занять у двоюродного брата */
                train_from_right_right_brother(node, right_brother, right_right_brother, parent, parent_index);
                if (!(*node)->_pointers.empty()) {
                    /* если есть дети у узла, то они должны быть и у правого брата */
                    move_child_from_right_right_brother(node, right_brother, right_right_brother);
                }
                return;
            }
        }
    }

    /* занять справа не получилось, занимаем слева */
    if (is_left_brother_exist(parent_index)) {
        auto left_brother = &(*parent)->_pointers[parent_index - 1];
        if (is_node_has_more_minimum_keys(left_brother)) {
            /* можно занять у левого брата */
            train_from_left_brother(node, left_brother, parent, parent_index);
            if (!(*node)->_pointers.empty()) {
                /* если есть дети у узла, то они должны быть и у левого брата */
                move_child_from_left_brother(node, left_brother);
            }
            return;
        }

        /* занять у левого брата не получилось, пробуем занять у левого двоюродного брата */
        if (is_left_brother_exist(parent_index - 1)) {
            auto left_left_brother = &(*parent)->_pointers[parent_index - 1 - 1];
            if (is_node_has_more_minimum_keys(left_left_brother)) {
                /* можно занять у левого двоюродного брата */
                train_from_left_left_brother(node, left_brother, left_left_brother, parent, parent_index);
                if (!(*node)->_pointers.empty()) {
                    /* если есть дети у узла, то они должны быть и у левого брата */
                    move_child_from_left_left_brother(node, left_brother, left_left_brother);
                }
                return;
            }
        }

    }

    /*
     * Ни у кого занять не получилось, поэтому пытаемся выполнить merge
     * у merge бывает три кейса:
     * 1. merge (node, right, right_right)
     * 2. merge (left, node, right)
     * 3. merge (left_left, left, node)
     */

    /* case 1 */
    if (is_right_brother_exist(parent, parent_index)) {
        auto right_brother = &(*parent)->_pointers[parent_index + 1];
        if (is_right_brother_exist(parent, parent_index + 1)) {
            auto right_right_brother = &(*parent)->_pointers[parent_index + 1 + 1];
            merge(node, right_brother, right_right_brother, parent, parent_index + 1);
            rebalancing_after_erase(path);
            return;
        }
    }


    /* case 2 */
    if (is_right_brother_exist(parent, parent_index) && is_left_brother_exist(parent_index)) {
        auto right_brother = &(*parent)->_pointers[parent_index + 1];
        auto left_brother = &(*parent)->_pointers[parent_index - 1];

        merge(left_brother, node, right_brother, parent, parent_index);
        rebalancing_after_erase(path);
        return;
    }

    /* case 3 */
    if (is_left_brother_exist(parent_index)) {
        auto left_brother = &(*parent)->_pointers[parent_index - 1];
        if (is_left_brother_exist(parent_index - 1)) {
            auto left_left_brother = &(*parent)->_pointers[parent_index - 1 - 1];
            merge(left_left_brother, left_brother, node, parent, parent_index - 1);
            rebalancing_after_erase(path);
            return;
        }
    }

    /*
     * остался последний случай
     * в корне один элемент, у него два сына, в каждом сыне по минимум элементов
     * мы удаляем в каком-то из сыновей, и надо смержить все в один большой корень
     */

    merge_with_root();
}

/**
 * Мержит три узла в два узла
 *
 * @param left Левый брат node
 * @param node Узел, который находится посередине
 * @param right Правый брат node
 * @param parent Родитель
 * @param parent_index Индекс родителя, который находится над node
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::merge(bstree_node** left, bstree_node** node, bstree_node** right,
                                                                bstree_node** parent, size_t parent_index) {
    /*
     * Экспериментально доказано, что если:
     * 1. переполнен самый левый узел, то индекс элемента в среднем узле равен t
     * 2. переполнен или средний, правый узел, то индекс элемента в среднем узле равен t - 1
     */
    auto index_split_element = t - 1;
    if (is_node_miss_elements(left)) index_split_element = t;

    auto split_element = (*node)->_keys[index_split_element];
    (*node)->_keys.erase((*node)->_keys.begin() + index_split_element);

    /* элементы-разделители из родителя */
    auto first_element_from_parent = (*parent)->_keys[parent_index - 1];
    auto second_element_from_parent = (*parent)->_keys[parent_index];

    (*parent)->_keys.erase( (*parent)->_keys.begin() + parent_index - 1);
    (*parent)->_keys.erase((*parent)->_keys.begin() + parent_index - 1);

    /* заполняем левый узел */
    /* сначала добавляем элемент из родителя */
    (*left)->_keys.push_back(first_element_from_parent);
    /* переносим элементы из среднего узла в левый */
    for (int _ = 0; _ < index_split_element; ++_) {
        auto element_from_node = (*node)->_keys.front();
        (*left)->_keys.push_back(element_from_node);
        (*node)->_keys.erase((*node)->_keys.begin());

        if (!(*node)->_pointers.empty()) {
            auto child_from_node = (*node)->_pointers.front();
            (*left)->_pointers.push_back(child_from_node);
            (*node)->_pointers.erase((*node)->_pointers.begin());
        }
    }

    if (!(*node)->_pointers.empty()) {
        auto child_from_node = (*node)->_pointers.front();
        (*left)->_pointers.push_back(child_from_node);
        (*node)->_pointers.erase((*node)->_pointers.begin());
    }

    /* заполняем правый узел */
    /* сначала добавляем элемент из родителя */
    (*right)->_keys.insert((*right)->_keys.begin(), second_element_from_parent);
    /* переносим оставшиеся элемент из среднего узла в правый */
    for (int _ = 0; _ < (*node)->_keys.size(); ++_) {
        auto element_from_node = (*node)->_keys.back();
        (*right)->_keys.insert((*right)->_keys.begin(), element_from_node);
        (*node)->_keys.pop_back();

        if (!(*node)->_pointers.empty()) {
            auto child_from_node = (*node)->_pointers.back();
            (*right)->_pointers.insert((*right)->_pointers.begin(), child_from_node);
            (*node)->_pointers.pop_back();
        }
    }
    if (!(*node)->_pointers.empty()) {
        auto child_from_node = (*node)->_pointers.back();
        (*right)->_pointers.insert((*right)->_pointers.begin(), child_from_node);
        (*node)->_pointers.pop_back();
    }

    delete_node(*node);

    (*parent)->_keys.insert((*parent)->_keys.begin() + parent_index - 1, split_element); /* добавляем в родителя */
    (*parent)->_pointers.erase((*parent)->_pointers.begin() + parent_index); /* удаляем ребенка, который указывал на node */
}


/**
 * Функцию следует вызывать только в случае, если остался один корень, два сына,
 * в каждом из сыновей было по минимум элементов и из одного из узлов мы удалили элемент
 *
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::merge_with_root() {
    auto* left = _root->_pointers.front();
    auto* right = _root->_pointers.back();

    _root->_pointers.erase(_root->_pointers.begin(), _root->_pointers.end());

    while (!left->_keys.empty()) {
        push_back_element_to_parent(&left, &_root, 0);

        if (!left->_pointers.empty()) {
            push_back_child_to_parent(left, &_root, 0);
        }
    }
    if (!left->_pointers.empty()) {
        push_back_child_to_parent(left, &_root, 0);
    }

    while (!right->_keys.empty()) {
        push_front_element_to_parent(&right, &_root, _root->_keys.size()); /* вставляем в последний элемент  */

        if (!right->_pointers.empty()) {
            push_front_child_to_parent(right, &_root, _root->_pointers.size()); /* вставляем в последний элемент */
        }
    }

    if (!right->_pointers.empty()) {
        push_front_child_to_parent(right, &_root, _root->_pointers.size()); /* вставляем в последний элемент */
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::is_right_brother_exist(bstree_node** parent, size_t index) {
    return index + 1 < (*parent)->_pointers.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::is_left_brother_exist(size_t index) {
    if (index == 0) return false;
    return index - 1 >= 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::is_node_has_more_minimum_keys(bstree_node** node) {
    return (*node)->_keys.size() > minimum_keys_in_node;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BS_tree<tkey, tvalue, compare, t>::is_node_miss_elements(bstree_node** node) {
    return (*node)->_keys.size() < minimum_keys_in_node;
}

/**
 * Реализация логики паровозика, который сначала забирает элемент у правого брата, пушит его в родителя,
 * а потом добавляет в текущий узел
 *
 * @param node Узел, в который по итогу надо перенести узел
 * @param right_brother Правый брат, откуда мы забираем элемент
 * @param parent Родитель
 * @param parent_index Индекс элемента, который разделяет эти два узла
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::train_from_right_brother(bstree_node** node, bstree_node** right_brother,
                                                            bstree_node** parent, size_t parent_index) {
    push_front_element_to_parent(right_brother, parent, parent_index + 1);
    pull_element_to_back_from_parent(node, parent, parent_index);
}

/**
 * Реализация логики паровозика, который сначала забирает элемент у левого брата, пушит его в родителя,
 * а потом добавляет в текущий узел
 *
 * @param node Узел, в который по итогу надо перенести узел
 * @param left_brother Левый брат, откуда мы забираем элемент
 * @param parent Родитель
 * @param parent_index Индекс элемента, который разделяет эти два узла
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::train_from_left_brother(bstree_node** node, bstree_node** left_brother,
                                                            bstree_node** parent, size_t parent_index) {
    push_back_element_to_parent(left_brother, parent, parent_index - 1);
    pull_element_to_front_from_parent(node, parent, parent_index);
}

/**
 * Реализация логики длинного паровозика, который сначала забирает элемент у правого двоюродного брата, пушит его в родителя,
 * отдает правому брату, пушит из правого брата в родителя и, наконец, отдает узлу
 *
 * @param node Узел, в который по итогу надо перенести узел
 * @param right_brother Правый брат
 * @param right_right_brother Правый двоюродный брат
 * @param parent Родитель
 * @param parent_index Индекс элемента, который разделяет node и right_brother
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::train_from_right_right_brother(bstree_node** node, bstree_node** right_brother,
                    bstree_node** right_right_brother, bstree_node** parent, size_t parent_index) {
    train_from_right_brother(right_brother, right_right_brother, parent, parent_index + 1);
    train_from_right_brother(node, right_brother, parent, parent_index);
}

/**
 * Реализация логики длинного паровозика, который сначала забирает элемент у левого двоюродного брата, пушит его в родителя,
 * отдает левому брату, пушит из левого брата в родителя и, наконец, отдает узлу
 *
 * @param node Узел, в который по итогу надо перенести узел
 * @param left_brother Левый брат
 * @param left_left_brother Левый двоюродный брат
 * @param parent Родитель
 * @param parent_index Индекс элемента, который разделяет node и right_brother
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::train_from_left_left_brother(bstree_node** node, bstree_node** left_brother,
                    bstree_node** left_left_brother, bstree_node** parent, size_t parent_index) {
    train_from_left_brother(left_brother, left_left_brother, parent, parent_index - 1);
    train_from_left_brother(node, left_brother, parent, parent_index);
}

/**
 * Удаляет первый элемент из node и добавляет в родителя по индексу parent_index
 * @param node Узел, первый элемент которого надо добавить в родителя
 * @param parent Родитель
 * @param parent_index Индекс родителя, куда надо вставить новый узел
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::push_front_element_to_parent(bstree_node** node, bstree_node** parent, size_t parent_index) {
    auto element = (*node)->_keys.front();
    (*node)->_keys.erase((*node)->_keys.begin());
    (*parent)->_keys.insert((*parent)->_keys.begin() + parent_index, element);
}

/**
 * Удаляет элемент из родителя по индексу parent_index и добавляет в конец node
 *
 * @param node Узел, в конец которого надо добавить новый элемент
 * @param parent Родитель
 * @param parent_index Индекс родителя, из которого удаляется элемент
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::pull_element_to_back_from_parent(bstree_node** node, bstree_node** parent, size_t parent_index) {
    auto element = (*parent)->_keys[parent_index];
    (*parent)->_keys.erase((*parent)->_keys.begin() + parent_index);
    (*node)->_keys.push_back(element);
}


/**
 * Функция, которая удаляет последний элемент из узла и добавляет его в родителя по индексу parent_index
 *
 * @param node Узел, последний элемент которого должен быть добавлен в родителя
 * @param parent Родитель
 * @param parent_index Индекс в родителе, куда надо вставить новый элемент
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::push_back_element_to_parent(bstree_node** node, bstree_node** parent, size_t parent_index) {
    auto element = (*node)->_keys.back();
    (*node)->_keys.pop_back();
    (*parent)->_keys.insert((*parent)->_keys.begin() + parent_index, element);
}

/**
 * Удаляет из родителя элемент по индексу parent_index и добавляет его в начало node
 *
 * @param node Узел, в начало которого надо добавить элемент из родителя
 * @param parent Родитель
 * @param parent_index Индекс в родителе, откуда надо удалить элемент и вставить в узел
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::pull_element_to_front_from_parent(bstree_node** node, bstree_node** parent, size_t parent_index) {
    auto element = (*parent)->_keys[parent_index];
    (*parent)->_keys.erase((*parent)->_keys.begin() + parent_index);
    (*node)->_keys.insert((*node)->_keys.begin(), element);
}


/**
 * Удаляет последнего сына из ребенка и добавляет его в сыновей родителя по индексу parent_index
 *
 * @param node Узел, откуда будет удален сын
 * @param parent Родитель, куда будет добавлен сын
 * @param parent_index Индекс в родителе
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::push_back_child_to_parent(bstree_node *node, bstree_node **parent, size_t parent_index) {
    auto* child = node->_pointers.back();
    node->_pointers.pop_back();
    (*parent)->_pointers.insert((*parent)->_pointers.begin() + parent_index, child);
}

/**
 * Удаляет первого сына из ребенка и добавляет его в сыновей родителя по индексу parent_index
 *
 * @param node Узел, откуда будет удален сын
 * @param parent Родитель, куда будет добавлен сын
 * @param parent_index Индекс в родителе
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::push_front_child_to_parent(bstree_node *node, bstree_node **parent, size_t parent_index) {
    auto child  = node->_pointers.front();
    node->_pointers.erase(node->_pointers.begin());
    (*parent)->_pointers.insert((*parent)->_pointers.begin() + parent_index, child);
}


/**
 * Забирает первого ребенка у правого брата
 * @param node Узел, в который надо перенести ребенка
 * @param right_brother Правый брат узла, у которого надо забрать ребенка
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::move_child_from_right_brother(bstree_node **node, bstree_node **right_brother) {
    auto child = (*right_brother)->_pointers.front();
    (*right_brother)->_pointers.erase((*right_brother)->_pointers.begin());
    (*node)->_pointers.push_back(child);
}

/**
 * Забирает последнего ребенка у левого брата
 * @param node Узел, в который надо перенести ребенка
 * @param left_brother Левый брат узла, у которого надо забрать ребенка
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::move_child_from_left_brother(bstree_node **node, bstree_node **left_brother) {
    auto child = (*left_brother)->_pointers.back();
    (*left_brother)->_pointers.pop_back();
    (*node)->_pointers.insert((*node)->_pointers.begin(), child);
}


/**
 * Переносит сначала ребенка у двоюродного брата в брата, а затем из брата в узел
 * @param node Узел
 * @param right_brother Правый брат
 * @param right_right_brother Двоюродный правый брат
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::move_child_from_right_right_brother(bstree_node **node, bstree_node **right_brother, bstree_node **right_right_brother) {
    move_child_from_right_brother(right_brother, right_right_brother);
    move_child_from_right_brother(node, right_brother);
}

/**
 * Переносит сначала ребенка у двоюродного брата в брата, а затем из брата в узел
 * @param node Узел
 * @param left_brother Левый брат
 * @param left_left_brother Двоюродный левый брат
 */
template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::move_child_from_left_left_brother(bstree_node **node, bstree_node **left_brother, bstree_node **left_left_brother) {
    move_child_from_left_brother(left_brother, left_left_brother);
    move_child_from_left_brother(node, left_brother);
}




// endregion

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    delete_node(_root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return emplace(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    return emplace(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);

    auto found_key = find(data.first);
    if (found_key != end()) return {found_key, false};

    auto iterator = search_terminate_node_to_insert(data.first);

    if (iterator == end()) {
        /* дерево было пустым, надо создать новый элемент */
        auto node = get_allocator().template new_object<bstree_node>();
        _root = node;
        std::stack<std::pair<bstree_node**, size_t>> path;
        path.emplace(&node, 0);
        iterator = bstree_iterator(path, 0);
    }

    auto path = iterator._path;
    auto index = iterator._index;

    auto [node, _] = path.top();
    (*node)->_keys.insert((*node)->_keys.begin() + index, data);

    rebalancing_after_insert(path);

    ++_size;
    return {find(data.first), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    return emplace_or_assign(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    return emplace_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator  BS_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);

    auto found_key = find(data.first);
    if (found_key != end()) {
        /* такой ключ уже существовал */
        auto* node = found_key->first;
        node->_keys[found_key._index] = data.second;
        return found_key;
    }

    return insert(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_iterator pos) {
    if (pos == end()) return pos;

    auto path = pos._path;
    auto index = pos._index;

    auto [node, _] = path.top();
    auto key = (*node)->_keys[index].first;

    if (!pos.is_terminate_node()) erase_from_internal_node(path, index);
    else erase_from_leaf(path, index);

    --_size;
    return upper_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_const_iterator pos)
{
    return erase(bstree_iterator(pos));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_iterator beg, bstree_iterator en)
{
    auto res = end();
    while (beg != en) {
        res = erase(beg);
        ++beg;
    }
    return res;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_const_iterator beg, bstree_const_iterator en)
{
    return erase(btree_iterator(beg), btree_iterator(en));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    auto iterator = find(key);
    if (iterator == end()) return iterator;

    return erase(iterator);
}

// endregion modifiers impl

#endif