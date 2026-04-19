#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <not_implemented.h>
#include <initializer_list>

#ifndef SYS_PROG_B_PLUS_TREE_H
#define SYS_PROG_B_PLUS_TREE_H

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BP_tree final : private compare //EBCO
{
public:
    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:
    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey &lhs, const tkey &rhs) const;

    inline bool compare_pairs(const tree_data_type &lhs, const tree_data_type &rhs) const;

    bool less(tkey a, tkey b) { return compare::operator()(a, b); }
    bool greater(tkey a, tkey b) { return compare::operator()(b, a); }
    bool equal(tkey a, tkey b) { return !less(a, b) && !greater(a, b); }
    bool not_equal(tkey a, tkey b) { return !equal(a, b); }
    bool less_or_equal(tkey a, tkey b) { return less(a, b) || equal(a, b); }
    bool greater_or_equal(tkey a, tkey b) { return greater(a, b) || equal(a, b); }

    // endregion comparators declaration

    struct bptree_node_base {
        bool _is_terminate;

        bptree_node_base() noexcept;

        virtual boost::container::static_vector<tkey, maximum_keys_in_node + 1> &keys() = 0;

        virtual ~bptree_node_base() = default;
    };

    struct bptree_node_term : public bptree_node_base {
        bptree_node_term *_next;

        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _data;

        bptree_node_term() noexcept;

        boost::container::static_vector<tkey, maximum_keys_in_node + 1> &keys() override;
    };

    struct bptree_node_middle : public bptree_node_base {
        boost::container::static_vector<tkey, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<bptree_node_base *, maximum_keys_in_node + 2> _pointers;

        bptree_node_middle() noexcept;

        boost::container::static_vector<tkey, maximum_keys_in_node + 1> &keys() override;
    };

    pp_allocator<value_type> _allocator;
    bptree_node_base *_root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

public:
    // region constructors declaration

    explicit BP_tree(const compare &cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit BP_tree(pp_allocator<value_type> alloc, const compare &comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BP_tree(iterator begin, iterator end, const compare &cmp = compare(),
                     pp_allocator<value_type> = pp_allocator<value_type>());

    BP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp = compare(),
            pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    BP_tree(const BP_tree &other);

    BP_tree(BP_tree &&other) noexcept;

    BP_tree &operator=(const BP_tree &other);

    BP_tree &operator=(BP_tree &&other) noexcept;

    ~BP_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class bptree_iterator;
    class bptree_const_iterator;

    class bptree_iterator final {
        bptree_node_term *_node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type &;
        using pointer = value_type *;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_iterator;

        friend class BP_tree;
        friend class bptree_const_iterator;

        reference operator*() const noexcept;

        pointer operator->() const noexcept;

        self &operator++();

        self operator++(int);

        bool operator==(const self &other) const noexcept;

        bool operator!=(const self &other) const noexcept;

        size_t current_node_keys_count() const noexcept;

        size_t index() const noexcept;

        explicit bptree_iterator(bptree_node_term *node = nullptr, size_t index = 0);
    };

    class bptree_const_iterator final {
        const bptree_node_term *_node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = const value_type &;
        using pointer = const value_type *;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_const_iterator;

        friend class BP_tree;
        friend class bptree_iterator;

        bptree_const_iterator(const bptree_iterator &it) noexcept;

        reference operator*() const noexcept;

        pointer operator->() const noexcept;

        self &operator++();

        self operator++(int);

        bool operator==(const self &other) const noexcept;

        bool operator!=(const self &other) const noexcept;

        size_t current_node_keys_count() const noexcept;

        size_t index() const noexcept;

        explicit bptree_const_iterator(const bptree_node_term *node = nullptr, size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue &at(const tkey &);

    const tvalue &at(const tkey &) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue &operator[](const tkey &key);

    tvalue &operator[](tkey &&key);

    // endregion element access declaration
    // region iterator begins declaration

    bptree_iterator begin();

    bptree_iterator end();

    bptree_const_iterator begin() const;

    bptree_const_iterator end() const;

    bptree_const_iterator cbegin() const;

    bptree_const_iterator cend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;

    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    bptree_iterator find(const tkey &key);

    bptree_const_iterator find(const tkey &key) const;

    bptree_iterator lower_bound(const tkey &key);

    bptree_const_iterator lower_bound(const tkey &key) const;

    bptree_iterator upper_bound(const tkey &key);

    bptree_const_iterator upper_bound(const tkey &key) const;

    bool contains(const tkey &key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<bptree_iterator, bool> insert(const tree_data_type &data);

    std::pair<bptree_iterator, bool> insert(tree_data_type &&data);

    template<typename... Args>
    std::pair<bptree_iterator, bool> emplace(Args &&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    bptree_iterator insert_or_assign(const tree_data_type &data);

    bptree_iterator insert_or_assign(tree_data_type &&data);

    template<typename... Args>
    bptree_iterator emplace_or_assign(Args &&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    bptree_iterator erase(bptree_iterator pos);

    bptree_iterator erase(bptree_const_iterator pos);

    bptree_iterator erase(bptree_iterator beg, bptree_iterator en);

    bptree_iterator erase(bptree_const_iterator beg, bptree_const_iterator en);


    bptree_iterator erase(const tkey &key);

    // endregion modifiers declaration

private:
    // region helper functions for begins

    bptree_node_term *first_terminate_node();

    // endregion
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type>
    compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
    std::size_t t = 5, typename U>
BP_tree(iterator begin, iterator end, const compare &cmp = compare(),
        pp_allocator<U> = pp_allocator<U>()) -> BP_tree<typename std::iterator_traits<iterator>::value_type::first_type,
    typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp = compare(),
        pp_allocator<U> = pp_allocator<U>()) -> BP_tree<tkey, tvalue, compare, t>;

// region comparators impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_pairs(const BP_tree::tree_data_type &lhs,
                                                      const BP_tree::tree_data_type &rhs) const {
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const {
    return compare::operator()(lhs, rhs);
}

// endregion comparators impl

// region node constructors impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_base::bptree_node_base() noexcept : _is_terminate(false) {
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_term::bptree_node_term() noexcept {
    this->_is_terminate = true;
    _next = nullptr;
    _data = boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1>{};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_middle::bptree_node_middle() noexcept {
    this->_is_terminate = false;
    _keys = boost::container::static_vector<tkey, maximum_keys_in_node + 1>{};
    _pointers = boost::container::static_vector<bptree_node_base *, maximum_keys_in_node + 2>{};
}

// endregion node constructors impl

// region node virtual functions impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
boost::container::static_vector<tkey, BP_tree<tkey, tvalue, compare, t>::maximum_keys_in_node + 1> &BP_tree<tkey, tvalue
    , compare,
    t>::bptree_node_term::keys() {
    auto keys = boost::container::static_vector<tkey, maximum_keys_in_node + 1>{};
    for (auto &elem: _data) {
        keys.push_back(elem.first);
    }
    return keys;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
boost::container::static_vector<tkey, BP_tree<tkey, tvalue, compare, t>::maximum_keys_in_node + 1> &BP_tree<tkey, tvalue
    , compare,
    t>::bptree_node_middle::keys() {
    return _keys;
}


// endregion

// region getters

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BP_tree<tkey, tvalue, compare, t>::value_type> BP_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept {
    return _allocator;
}

// endregion

// region common iterator impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::reference BP_tree<tkey, tvalue, compare, t>::
bptree_iterator::operator*() const noexcept {
    return _node->_data[_index];
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::pointer BP_tree<tkey, tvalue, compare, t>::bptree_iterator
::operator->() const noexcept {
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self &BP_tree<tkey, tvalue, compare, t>::bptree_iterator::
operator++() {
    if (_node == nullptr) return *this; /* уже в конце */

    /* если мы в конце текущего узла */
    if (_index + 1 >= current_node_keys_count()) {
        _index = 0;
        _node = _node->_next;
    }
    /* сдвигаемся в текущем узле на следующий элемент */
    else {
        ++_index;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self BP_tree<tkey, tvalue, compare, t>::bptree_iterator::
operator++(int) {
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator==(const self &other) const noexcept {
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator!=(const self &other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::current_node_keys_count() const noexcept {
    return _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::index() const noexcept {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::bptree_iterator(bptree_node_term *node, size_t index) {
    _node = node;
    _index = index;
}

// endregion common iterator impl

// region const iterator impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_iterator &it) noexcept {
    _node = it._node;
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::reference BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator*() const noexcept {
    return _node->_data[_index];
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::pointer BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator->() const noexcept {
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self &BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator++() {
    if (_node == nullptr) return *this; /* уже в конце */

    /* если мы в конце текущего узла */
    if (_index + 1 >= current_node_keys_count()) {
        _index = 0;
        _node = _node->_next;
    }
    /* сдвигаемся в текущем узле на следующий элемент */
    else {
        ++_index;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator++(int) {
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator==(const self &other) const noexcept {
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator!=(const self &other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::current_node_keys_count() const noexcept {
    return _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::index() const noexcept {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(
    const bptree_node_term *node, size_t index) {
    _node = node;
    _index = index;
}

// endregion const iterator impl

// region element access impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BP_tree<tkey, tvalue, compare, t>::at(const tkey &) {
    throw not_implemented("too laazyy", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue &BP_tree<tkey, tvalue, compare, t>::at(const tkey &) const {
    throw not_implemented("too laazyy", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BP_tree<tkey, tvalue, compare, t>::operator[](const tkey &key) {
    throw not_implemented("too laazyy", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BP_tree<tkey, tvalue, compare, t>::operator[](tkey &&key) {
    throw not_implemented("too laazyy", "your code should be here...");
}

// endregion element access impl

// region constructors impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const compare &cmp, pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(pp_allocator<value_type> alloc, const compare &cmp)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BP_tree<tkey, tvalue, compare, t>::BP_tree(iterator begin, iterator end, const compare &cmp,
                                           pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
    while (begin != end) {
        insert(*begin);
        ++begin;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp,
                                           pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
    for (auto item: data) insert_or_assign(item);
}

// endregion

// region five impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const BP_tree &other)
    : compare(other), _allocator(other._allocator), _root(nullptr), _size(0) {
    for (auto it = other->begin(); it != other->end(); ++it) {
        insert(it);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(BP_tree &&other) noexcept
    : compare(std::move(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size) {
    other._root = nullptr;
    other._size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t> &BP_tree<tkey, tvalue, compare, t>::operator=(const BP_tree &other) {
    if (this != &other) {
        clear();
        *this = BP_tree(other);
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t> &BP_tree<tkey, tvalue, compare, t>::operator=(BP_tree &&other) noexcept {
    if (this != &other) {
        clear();

        static_cast<compare &>(*this) = std::move(other);
        _allocator = std::move(other._allocator);
        _size = other._size;
        _root = other._root;

        other._root = nullptr;
        other._size = 0;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::~BP_tree() noexcept {
    clear();
}

// endregion five impl

// region begins impl

// region helper functions for begins

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_term *BP_tree<tkey, tvalue, compare, t>::first_terminate_node() {
    bptree_node_base *node = _root;
    while (dynamic_cast<bptree_node_middle *>(node)) {
        node = dynamic_cast<bptree_node_middle *>(node);
        node = node->_pointers[0];
    }
    return node;
}

// endregion

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::begin() {
    auto node = first_terminate_node();
    return bptree_iterator(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::end() {
    return bptree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::begin() const {
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::end() const {
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cbegin() const {
    const auto *node = const_cast<bptree_node_term *>(first_terminate_node());
    return bptree_const_iterator(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cend() const {
    return bptree_const_iterator();
}

// endregion begins impl

// region lookup impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::size() const noexcept {
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::empty() const noexcept {
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey &key) {
    if (_root == nullptr) return end();

    auto *node = _root;
    do {
        auto keys = node->keys();
        /* бинарный поиск по ключам */
        size_t l = 0;
        size_t r = keys.size();
        while (l + 1 < r) {
            size_t m = (l + r) / 2;

            if (less_or_equal(keys[m], key)) {
                /* keys[m] <= key */
                l = m;
            } else {
                r = m;
            }
        }

        /* такой ключ существует в листе */
        if (node->_is_terminate && equal(keys[l], key)) return bptree_iterator(node, l);

        /* нужно перейти в самого левого ребенка */
        if (less(key, keys[l])) --l;

        auto middle_node = dynamic_cast<bptree_node_middle *>(node);
        if (middle_node != nullptr) node = middle_node->_pointers[l + 1];
    } while (!node->_is_terminate);

    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::find(
    const tkey &key) const {
    return bptree_const_iterator(const_cast<BP_tree *>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare,
    t>::lower_bound(const tkey &key) {
    if (_root == nullptr) return end();

    auto *node = _root;
    size_t index = 0;
    do {
        auto keys = node->keys();
        /* бинарный поиск по ключам */
        size_t l = 0;
        size_t r = keys.size();
        while (l + 1 < r) {
            size_t m = (l + r) / 2;

            if (less_or_equal(keys[m], key)) {
                /* keys[m] <= key */
                l = m;
            } else {
                r = m;
            }
        }

        /* такой ключ существует в листе */
        if (node->_is_terminate && equal(keys[l], key)) return bptree_iterator(node, l);

        /* нужно перейти в самого левого ребенка */
        if (less(key, keys[l])) --l;

        auto middle_node = dynamic_cast<bptree_node_middle *>(node);
        if (middle_node != nullptr) {
            ++l;
            node = middle_node->_pointers[l];
        }
        index = l;
    } while (!node->_is_terminate);

    auto iterator = bptree_iterator(node, index);
    /* в случае (index == 0 && key < (*iterator).first) мы уже находимся на элементе, который больше нашего */
    if (iterator->first == key || (index == 0 && key < iterator->first)) return iterator;
    return ++iterator; /* следующий элемент */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(
    const tkey &key) const {
    return bstree_const_iterator(const_cast<BP_tree *>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare,
    t>::upper_bound(const tkey &key) {
    auto iterator = lower_bound(key);

    if (iterator == end()) return iterator;
    if (iterator->first == key) return ++iterator;
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(
    const tkey &key) const {
    return bstree_const_iterator(const_cast<BP_tree*>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::contains(const tkey &key) const {
    return find(key) != end();
}

// endregion lookup impl

// region modifiers impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::clear() noexcept {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> void BP_tree<tkey, tvalue, compare, t>::clear() noexcept",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::insert(
    const tree_data_type &data) {
    throw not_implemented("too laazyy", "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::insert(
    tree_data_type &&data) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare,
    t>::emplace(Args &&... args) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> template <typename ...Args> std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(
    const tree_data_type &data) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(
    tree_data_type &&data) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare,
    t>::emplace_or_assign(Args &&... args) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> template <typename ...Args> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(
    bptree_iterator pos) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator pos)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(
    bptree_const_iterator pos) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator pos)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(
    bptree_iterator beg, bptree_iterator en) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator beg, bptree_iterator en)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(
    bptree_const_iterator beg, bptree_const_iterator en) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator beg, bptree_const_iterator en)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(const tkey &key) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(const tkey& key)",
        "your code should be here...");
}

// endregion modifiers impl

#endif
