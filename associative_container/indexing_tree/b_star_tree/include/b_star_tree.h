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
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<bstree_node*, maximum_keys_in_node + 2> _pointers;
        bstree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    bstree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

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
                if (child) {
                    _pointers.push_back(new debug_bstree_node(child));
                } else {
                    _pointers.push_back(nullptr);
                }
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
    void print_node(bstree_node* node, int depth);

    // endregion

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
    std::vector<tree_data_type> keys(node->_keys.begin(), node->_keys.end());
    std::vector<bstree_node*> pointers(node->_pointers.begin(), node->_pointers.end());

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
    while ( !(*node)->_pointers.empty() ) {
        node = &((*node)->_pointers[index + 1]);
        path.emplace(node, index + 1);
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
    return (*_path.top().first)->_pointers.empty();
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

    auto node_vector = new debug_bstree_node(node);

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
    size_t count = 0;
    auto iterator = begin();
    while ( iterator != end() ) {
        ++count;
        ++iterator;
    }
    return count;
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
    if ((*iterator).first == key) return iterator;
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

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BS_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    delete_node(_root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
std::pair<typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator, bool> BS_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);

    auto iterator = lower_bound(data.first);

    return std::pair<bstree_iterator, bool> {begin(), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template <typename ...Args>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator  BS_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> template <typename ...Args> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_iterator pos)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_iterator pos)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_const_iterator pos)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_const_iterator pos)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_iterator beg, bstree_iterator en)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_iterator beg, bstree_iterator en)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_const_iterator beg, bstree_const_iterator en)
{
    throw not_implemented("template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(bstree_const_iterator beg, bstree_const_iterator en)", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    throw not_implemented(
            "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BS_tree<tkey, tvalue, compare, t>::bstree_iterator BS_tree<tkey, tvalue, compare, t>::erase(const tkey& key)",
            "your code should be here...");
}

// endregion modifiers impl

#endif