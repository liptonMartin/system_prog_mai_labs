#ifndef SYS_PROG_B_TREE_H
#define SYS_PROG_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private compare // EBCO
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration


    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    btree_node* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

public:

    // region constructors declaration

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    // region helper functions for five declaration
    btree_node* clone_node(btree_node* node);

    void delete_node(btree_node* node) noexcept;
    // endregion helper functions for five declaration

    B_tree(const B_tree& other);

    B_tree(B_tree&& other) noexcept;

    B_tree& operator=(const B_tree& other);

    B_tree& operator=(B_tree&& other) noexcept;

    ~B_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);

    };

    class btree_const_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_reverse_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

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

    // region helper functions for iterator begins declaration

    std::stack<std::pair<btree_node**, size_t>> path_begin();
    std::stack<std::pair<btree_node**, size_t>> path_reverse_begin();

    // endregion helper functions for iterator begins declaration

    btree_iterator begin();
    btree_iterator end();

    btree_const_iterator begin() const;
    btree_const_iterator end() const;

    btree_const_iterator cbegin() const;
    btree_const_iterator cend() const;

    btree_reverse_iterator rbegin();
    btree_reverse_iterator rend();

    btree_const_reverse_iterator rbegin() const;
    btree_const_reverse_iterator rend() const;

    btree_const_reverse_iterator crbegin() const;
    btree_const_reverse_iterator crend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next to removed or end() if key not exists
     */
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);


    btree_iterator erase(const tkey& key);

    // endregion modifiers declaration

    // region print functions declaration
    void print_tree();
    void print_node(btree_node* node, int depth = 0);
    // endregion print functions declaration

private:
    // region helper function for iterators declaration

    template <class ptr_to_ptr_to_node>
    static void increment_iterator_inner(std::stack<std::pair<ptr_to_ptr_to_node, size_t>> &path, size_t &index);

    template <class ptr_to_ptr_to_node>
    static void decrement_iterator_inner(std::stack<std::pair<ptr_to_ptr_to_node, size_t>> &path, size_t &index);


    // endregion helper function for iterators declaration

    // region helper functions for modifiers declaration

    void search_terminate_node_to_emplace(std::stack<std::pair<btree_node*, size_t>>& path, tkey key);

    void rebalancing_after_insert(std::stack<std::pair<btree_node*, size_t>> &path);

    btree_node* split(btree_node* node, btree_node* parent, size_t index);

    bool is_terminate_node(btree_node* node);
    std::pair<btree_node*, size_t> try_top_path(std::stack<std::pair<btree_node*, size_t>> &path);
    void try_pop_path(std::stack<std::pair<btree_node*, size_t>>& path);


    void erase_from_leaf(std::stack<std::pair<btree_node**, size_t>>& path, size_t index);
    void erase_from_internal_node(std::stack<std::pair<btree_node**, size_t>>& path, size_t index);
    void rebalancing_after_erase(std::stack<std::pair<btree_node**, size_t>> &path);

    // endregion

    // region helper functions for comparator

    bool less(tkey a, tkey b) { return compare::operator()(a, b); }
    bool greater(tkey a, tkey b) { return compare::operator()(b, a); }
    bool equal(tkey a, tkey b) {return !less(a, b) && !greater(a, b); }
    bool not_equal(tkey a, tkey b) { return !equal(a, b); }
    bool less_or_equal(tkey a, tkey b) {return less(a, b) || equal(a, b); }
    bool greater_or_equal(tkey a, tkey b) {return greater(a, b) || equal(a, b); }

    // endregion helper functions for comparator

    // region debug functions
    struct debug_btree_node
    {
        std::vector<tree_data_type> _keys;
        std::vector<debug_btree_node*> _pointers;

        debug_btree_node(const btree_node* node)
        {
            // Копируем ключи
            _keys.assign(node->_keys.begin(), node->_keys.end());

            // Копируем указатели (рекурсивно конвертируем детей)
            for (auto* child : node->_pointers) {
                if (child) {
                    _pointers.push_back(new debug_btree_node(child));
                } else {
                    _pointers.push_back(nullptr);
                }
            }
        }

        ~debug_btree_node() {
            for (auto* ptr : _pointers) {
                delete ptr;
            }
        }
    };

    static std::vector<debug_btree_node*> vector_pointers(
        const boost::container::static_vector<btree_node*, maximum_keys_in_node + 2>& _pointers)
    {
        std::vector<debug_btree_node*> result;
        result.reserve(_pointers.size());

        for (auto* node_ptr : _pointers) {
            if (node_ptr) {
                result.push_back(new debug_btree_node(node_ptr));
            } else {
                result.push_back(nullptr);
            }
        }

        return result;
    }

    // endregion
};

// region print functions impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::print_tree() {
    if (_root) print_node(_root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::print_node(btree_node* node, int depth) {
    std::vector<tree_data_type> keys(node->_keys.begin(), node->_keys.end());
    std::vector<btree_node*> pointers(node->_pointers.begin(), node->_pointers.end());

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

// endregion print functions impl

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs,
                                                     const B_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node() noexcept
{
    _keys = boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1>{};
    _pointers = boost::container::static_vector<btree_node*, maximum_keys_in_node + 2>{};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type> B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

// region constructors implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        const compare& cmp,
        pp_allocator<value_type> alloc)
            : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        pp_allocator<value_type> alloc,\
        const compare& comp)
            : compare(comp), _allocator(alloc), _root(nullptr), _size(0) {}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc)
            : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto it = begin; it != end; ++it) {
        // TODO: после реализации итераторов, проверить правильно ли разыменовывается
        insert(*it);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc)
            : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto item : data) {
        insert(item);
    }
}

// endregion constructors implementation

// region five implementation

// region helper functions for five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node*
B_tree<tkey, tvalue, compare, t>::clone_node(btree_node* node) {
    if (!node) return nullptr;

    auto* new_node = new btree_node();
    new_node->_keys = node->_keys;
    new_node->_pointers.resize(node->_pointers.size());
    for (size_t i = 0; i < node->_pointers.size(); ++i) {
        new_node->_pointers[i] = clone_node(node->_pointers[i]);
    }

    return new_node;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::delete_node(btree_node *node) noexcept {
    if (!node) return;

    auto node_vector = new debug_btree_node(node);

    for (auto child : node->_pointers) {
        delete_node(child);
    }
    _allocator.template delete_object<btree_node>(node);
}

// endregion helper functions for five implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other)
    : compare(other), _allocator(other._allocator), _root(nullptr), _size(other._size)
{
    _root = clone_node(other._root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other)
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
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept
    : compare(std::move(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size)
{
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept
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

// endregion five implementation

// region iterators implementation

// region helper functions for iterators impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<class ptr_to_ptr_to_node>
void B_tree<tkey, tvalue, compare, t>::increment_iterator_inner(
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
void B_tree<tkey, tvalue, compare, t>::decrement_iterator_inner(
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

// region common iterator

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index)
            : _path(path), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--()
{
    decrement_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept
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
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;

    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept
{
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept
{
    return _index;
}

// endregion common iterator

// region const iterator

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) {};

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const btree_iterator& it) noexcept
{
    std::stack<std::pair<btree_node* const*, size_t>> tmp;
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
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--()
{
    decrement_iterator_inner(_path, _index);
    return *this;

}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept
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
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;

    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept
{
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept
{
    return _index;
}

// endregion const iterator

// region reverse iterator

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index) : _path(path), _index(index) {};

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const btree_iterator& it) noexcept
{
    _path = it._path;
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_iterator() const noexcept
{
    return btree_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()
{
    decrement_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept
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
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept
{
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion reverse iterator

// region const reverse iterator

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index) : _path(path), _index(index) {};

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const btree_reverse_iterator& it) noexcept
{
    std::stack<std::pair<btree_node* const*, size_t>> tmp;
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
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_const_iterator() const noexcept
{
    return btree_const_iterator(_path, _index);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept
{
    auto [node, _] = _path.top();

    return reinterpret_cast<reference>((*node)->_keys[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept
{
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()
{
    decrement_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()
{
    increment_iterator_inner(_path, _index);
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)
{
    auto iterator = *this;
    --(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept
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
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept
{
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.size() == 0) return 0;
    return (*_path.top().first)->_keys.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept
{
    return (*_path.top().first)->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion const reverse iterator

// endregion iterators implementation

// region element access implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("Такого ключа не существует");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("Такого ключа не существует");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto iterator = find(key);
    if (iterator == end()) iterator = insert({key, tvalue {}});

    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    return operator[](std::move(key));
}

// endregion element access implementation

// region iterator begins implementation

// region helper functions for iterator begins implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::stack<std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_node**, size_t>>
B_tree<tkey, tvalue, compare, t>::path_begin() {
    if (_root == nullptr) return {};

    std::stack<std::pair<btree_node**, size_t>> path;
    path.emplace(&_root, 0);

    auto** node = &_root;
    while (!(*node)->_pointers.empty()) {
        node = &((*node)->_pointers[0]);
        path.emplace(node, 0);
    }

    return path;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::stack<std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_node**, size_t>>
B_tree<tkey, tvalue, compare, t>::path_reverse_begin() {
    if (_root == nullptr) return {};

    std::stack<std::pair<btree_node**, size_t>> path;
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
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    if (_root == nullptr) return end();

    return btree_iterator(path_begin(), 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    return btree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->end());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    if (!_root) return rend();

    auto path = path_reverse_begin();
    auto [node_ptr_ptr, _] = path.top();
    auto* node_ptr = *node_ptr_ptr;

    return btree_reverse_iterator(path, node_ptr->_keys.size() - 1);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    return btree_reverse_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return crbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    return crend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    return btree_const_reverse_iterator(const_cast<B_tree*>(this)->rbegin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    return btree_const_reverse_iterator(const_cast<B_tree*>(this)->rend());
}

// endregion iterator begins implementation

// region lookup implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept
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
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return size() == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    if (!_root) return end();

    int index = 0;
    btree_node** node = &_root;
    std::stack<std::pair<btree_node**, size_t>> path;

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

        if (equal((*node)->_keys[l].first, key)) return btree_iterator(path, l);

        if ((*node)->_pointers.empty() || less(key, (*node)->_keys[0].first)) --l; /* нужно перейти в самого первого сына
                                                                                    * или мы дошли до листа */

        node = (*node)->_pointers.empty() ? nullptr : &((*node)->_pointers[l + 1]);
        index = l + 1;
    }

    node = path.top().first;
    if (equal((*node)->_keys[index].first, key)) return btree_iterator(path, index);
    return end(); /* дошли до конца и не нашли */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    return b_tree_const_iterator(const_cast<B_tree*>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    if (!_root) return end();

    int index = 0;
    btree_node** node = &_root;
    std::stack<std::pair<btree_node**, size_t>> path;

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

        if (equal((*node)->_keys[l].first, key)) return btree_iterator(path, l);

        if ((*node)->_pointers.empty() || less(key, (*node)->_keys[0].first)) --l; /* нужно перейти в самого первого сына
                                                                                    * или мы дошли до листа */

        node = (*node)->_pointers.empty() ? nullptr : &((*node)->_pointers[l + 1]);
        index = l + 1;
    }

    auto iterator = btree_iterator(path, index);
    if ((*iterator).first == key) return iterator;
    return ++iterator; /* следующий элемент */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    auto iterator = lower_bound(key);

    if (iterator == end()) return iterator;
    auto node = *iterator;
    if (node.first == key) return ++iterator;
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    return btree_const_iterator(const_cast<B_tree*>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

// endregion lookup implementation

// region modifiers implementation

// region helper functions for modifiers impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::search_terminate_node_to_emplace(
                std::stack<std::pair<btree_node*, size_t>>& path, tkey key) {
    /* после нахождения, наверху path должен лежать узел, куда надо вставить новый узел */
    auto node = _root;
    while (node) {
        /*
         * В l - первый элемент меньше или равен искомого узла
         * поэтому в конце для листа: добавляем по индексу на один больше (l + 1).
         * Поэтому в конце для внутреннего узла: идем в следующего сына (l + 1).
         *
         * Нужно обработать случаи первого элемента
         *
         */

        int l = 0;
        int r = node->_keys.size() + 1;

        while (l + 1 < r) {
            int m = (l + r) / 2;
            if (m == node->_keys.size()) {
                if (greater(key, node->_keys[m - 1].first)) { /* искомый ключ слишком большой, поэтому вставляем в самый конец */
                    l = m - 1;
                    break;
                }
                else {
                    r = m;
                    continue;
                }
            }

            if (less_or_equal(node->_keys[m].first, key)) { /* node->_keys[m].first <= key */
                l = m;
            }
            else {
                r = m;
            }
        }

        if (node->_pointers.empty()) {
            if (! (l == 0 && greater(node->_keys[0].first, key))) l++;
                                        /* условие - нужно добавить перед первым элементов (индекс = 0) */

            path.emplace(node, l);
            node = nullptr;
        }
        else {
            if (l == 0 && greater(node->_keys[0].first, key)) --l; // нужно идти в первого сына

            path.emplace(node, l + 1);
            node = node->_pointers[l + 1];
        }
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::is_terminate_node(btree_node* node) {
    return node->_pointers.empty();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::rebalancing_after_insert(std::stack<std::pair<btree_node*, size_t>> &path) {
    /* функцию следует вызывать после вставки, она рекурсивно делит узлы, поднимаясь к корню */

    auto [node, index] = path.top();
    btree_node* parent = nullptr;
    path.pop();
    while (node->_keys.size() > maximum_keys_in_node) {
        std::tie(parent, index) = try_top_path(path);
        node = split(node, parent, index);
        try_pop_path(path);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node* B_tree<tkey, tvalue, compare, t>::split(btree_node* node, btree_node* parent, size_t index) {
    /*
     * уже добавили в node узел, и он переполнился
     * делим этот узел на два, добавляем в родителя
     *
     * index:: индекс сына в родителе (parent->_pointers[index] = node)
     */

    int middle = node->_keys.size() / 2;
    tree_data_type data = node->_keys[middle];

    auto* second_child = new btree_node();
    second_child->_keys.insert(second_child->_keys.end(), node->_keys.begin() + middle + 1, node->_keys.end());
    if (!node->_pointers.empty()) second_child->_pointers.insert(
                            second_child->_pointers.end(), node->_pointers.begin() + middle + 1, node->_pointers.end());

    node->_keys.erase(node->_keys.begin() + middle, node->_keys.end());
    if (!node->_pointers.empty()) node->_pointers.erase(node->_pointers.begin() + middle + 1, node->_pointers.end());

    if (parent == nullptr) {
        parent = _allocator.template new_object<btree_node>();
        index = 0;
        _root = parent;
    }

    parent->_keys.insert(parent->_keys.begin() + index, data);

    if (parent->_pointers.empty()) parent->_pointers.push_back(node); /* если родителя не было, там и детей нет */
    parent->_pointers.insert(parent->_pointers.begin() + index + 1, second_child);

    return parent;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_node*, size_t> B_tree<tkey, tvalue, compare, t>::try_top_path(std::stack<std::pair<btree_node*, size_t>>& path) {
    if (path.empty()) return std::pair<btree_node*, size_t> {nullptr, 0};
    return path.top();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::try_pop_path(std::stack<std::pair<btree_node*, size_t>>& path) {
    if (!path.empty()) path.pop();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::erase_from_leaf(std::stack<std::pair<btree_node**, size_t>>& path, size_t index) {
    /*
     * path:: путь от корня до текущего узла
     * path::first:: узел
     * path::second:: индекс сына в родителе, как мы попали в текущий узел (parent->_pointers[path.second] == path.first)
     *
     * index:: индекс элемента в текущем узле
     */

    auto [node, _] = path.top();

    (*node)->_keys.erase((*node)->_keys.begin() + index);

    rebalancing_after_erase(path);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::erase_from_internal_node(std::stack<std::pair<btree_node**, size_t>>& path, size_t index) {
    /*
     * path:: путь от корня до текущего узла
     * path::first:: узел
     * path::second:: индекс сына в родителе, как мы попали в текущий узел (parent->_pointers[path.second] == path.first)
     *
     * index:: индекс элемента в текущем узле
     */

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
void B_tree<tkey, tvalue, compare, t>::rebalancing_after_erase(std::stack<std::pair<btree_node**, size_t>> &path) {
    /* Запускать функцию только после удаления элемента из path.top(). Не вызывать path.pop() */

    auto [node, parent_index] = path.top();
    path.pop();

    if ((*node)->_keys.size() >= minimum_keys_in_node) return;

    if (path.empty()) {
        if ( *node == _root) return;
        throw std::logic_error("Балансировка от пустого узла! И это не корень!");
    }

    /* здесь в node лежит слишком мало элементов */

    auto [parent, parent_parent_index] = path.top();

    /* пробуем занять у левого соседа */
    /* если левый сосед существует, то наш элемент не первый (parent_index != 0) */
    if (parent_index != 0 && (*parent)->_pointers[parent_index - 1]->_keys.size() > minimum_keys_in_node) {
        /*
         * у левого соседа занимаем самый правый элемент, добавляем его как первый элемент в родителя
         * добавляем в текущий узел последний узел родителя
         */

        auto left_brother = (*parent)->_pointers[parent_index - 1];
        auto most_right_element = left_brother->_keys.back();
        left_brother->_keys.pop_back();

        auto most_left_element_parent = (*parent)->_keys.front();
        (*parent)->_keys.erase( (*parent)->_keys.begin() );

        (*parent)->_keys.insert( (*parent)->_keys.begin(), most_right_element );

        (*node)->_keys.insert( (*node)->_keys.begin(), most_left_element_parent );

        if (!left_brother->_pointers.empty()) {
            auto most_right_child_in_left_brother = left_brother->_pointers.back();
            left_brother->_pointers.pop_back();

            (*node)->_pointers.insert( (*node)->_pointers.begin(), most_right_child_in_left_brother );
        }
        return;
    }

    /* у левого соседа занять не получилось, пробуем занять у правого соседа */
    /* если правый сосед существует, то наш элемент не последний (parent_index < (*parent)->_pointers.size() - 1) */
    if (parent_index < (*parent)->_pointers.size() - 1 && (*parent)->_pointers[parent_index + 1]->_keys.size() > minimum_keys_in_node) {
        /*
         * у правого соседа занимаем самый левый элемент, добавляем его как последний элемент в родителя
         * добавляем в текущий узел первый узел родителя
         */

        auto right_brother = (*parent)->_pointers[parent_index + 1];
        auto most_left_element = right_brother->_keys.front();
        right_brother->_keys.erase( right_brother->_keys.begin() );

        auto most_right_element_parent = (*parent)->_keys.back();
        (*parent)->_keys.pop_back();

        (*parent)->_keys.push_back( most_left_element );

        (*node)->_keys.push_back(most_right_element_parent);

        if (!right_brother->_pointers.empty()) {
            auto most_left_child_in_right_brother = right_brother->_pointers.front();
            right_brother->_pointers.erase( right_brother->_pointers.begin() );

            (*node)->_pointers.push_back(most_left_child_in_right_brother);
        }
        return;
    }

    /* не получилось занять у соседей, поэтому объединяем узлы */

    /*
     * логика объединения:
     * - взять два соседних элемента
     * - вставить туда разделяющий узел из родителя
     * - запустить балансировку от родителя
     */

    /* пытаемся объединиться с левым соседом */
    if (parent_index != 0) {
        /* добавляем ключ-разделитель с родителя */
        auto node_split_parent = (*parent)->_keys[parent_index - 1];
        (*node)->_keys.insert( (*node)->_keys.begin(), node_split_parent );
        (*parent)->_keys.erase( (*parent)->_keys.begin() + parent_index - 1);

        /* добавляем весь соседний левый узел */
        auto left_brother = (*parent)->_pointers[parent_index - 1];
        (*node)->_keys.insert( (*node)->_keys.begin(), left_brother->_keys.begin(), left_brother->_keys.end() );

        /* добавляем детей */
        (*node)->_pointers.insert( (*node)->_pointers.begin(), left_brother->_pointers.begin(), left_brother->_pointers.end() );

        /* удаляем одного ребенка из родительского узла (левого соседа) */
        (*parent)->_pointers.erase((*parent)->_pointers.begin() + parent_index - 1);

        if (*parent == _root && (*parent)->_keys.empty()) {
            /* если мы заняли у корня, в котором не осталось ключей, то новый корень это наш node */
            _root = *node;
            return;
        }

        rebalancing_after_erase(path);
    }

    /* пытаемся объединиться с правым соседом */
    else if (parent_index < (*parent)->_pointers.size()) {
        /* добавляем ключ-разделить с родителя */
        auto node_split_parent = (*parent)->_keys[parent_index];
        (*node)->_keys.push_back(node_split_parent);
        (*parent)->_keys.erase((*parent)->_keys.begin() + parent_index);

        /* добавляем весь соседний правый узел */
        auto right_brother = (*parent)->_pointers[parent_index + 1];
        (*node)->_keys.insert( (*node)->_keys.end(), right_brother->_keys.begin(), right_brother->_keys.end() );

        /* добавляем детей */
        (*node)->_pointers.insert( (*node)->_pointers.end(), right_brother->_pointers.begin(), right_brother->_pointers.end() );

        /* удаляем одного ребенка из родительского узла (правого соседа) */
        (*parent)->_pointers.erase((*parent)->_pointers.begin() + parent_index + 1);

        if (*parent == _root && (*parent)->_keys.empty()) {
            /* если мы заняли у корня, в котором не осталось ключей, то новый корень это наш node */
            _root = *node;
            return;
        }

        rebalancing_after_erase(path);
    }

    /* какая-то ошибка в построении дерева, хотя бы один сосед должен быть */
    else throw std::logic_error("Ошибка в структуре дерева!!! Произошла во время удаления");
}

// endregion helper functions for modifiers impl


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    return delete_node(_root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return emplace(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    return emplace(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);

    std::stack<std::pair<btree_node*, size_t>> path;
    search_terminate_node_to_emplace(path, data.first);

    auto [node, index] = try_top_path(path);
    if (node && index < node->_keys.size() && node->_keys[index].first == data.first) /* такой ключ существует */
                return std::pair<btree_iterator, bool> {end(), false};

    if (node == nullptr) {
        node = _allocator.template new_object<btree_node>();
        index = 0;
        _root = node;
        path.emplace(node, index);
    }

    node->_keys.insert(node->_keys.begin() + index, data);
    rebalancing_after_insert(path);

    return std::pair<btree_iterator, bool> {find(data.first), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    return emplace_or_assign(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    return emplace_or_assign(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type data(std::forward<Args>(args)...);

    std::stack<std::pair<btree_node*, size_t>> path;
    search_terminate_node_to_emplace(path, data.first);

    auto [node, index] = try_top_path(path);
    if (node && index < node->_keys.size() && node->_keys[index].first == data.first) {
        /* такой ключ существует */
        node->_keys[index] = data.second;
        return find(data.first);
    }

    return insert(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    if (pos == end()) return end();

    auto path = pos._path;
    auto index = pos._index;

    auto [node, _] = path.top();
    auto key = (*node)->_keys[index].first;

    if ((*node)->_pointers.empty()) erase_from_leaf(path, index);
    else erase_from_internal_node(path, index);

    return upper_bound(key);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    return erase(btree_iterator(pos));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    auto res = end();
    while (beg != en) {
        res = erase(beg);
        ++beg;
    }
    return res;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    return erase(btree_iterator(beg), btree_iterator(en));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    auto node = find(key);
    return erase(node);
}

// endregion modifiers implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_pairs(const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &lhs,
                   const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &rhs)
{
    return compare_keys(lhs, rhs);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool compare_keys(const tkey &lhs, const tkey &rhs)
{
    return compare::operator()(lhs, rhs);
}


#endif