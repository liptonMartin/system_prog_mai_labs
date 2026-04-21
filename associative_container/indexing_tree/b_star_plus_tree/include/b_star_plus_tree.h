#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <initializer_list>
#include <not_implemented.h>

#ifndef SYS_PROG_BS_PLUS_TREE_H
#define SYS_PROG_BS_PLUS_TREE_H

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BSP_tree final : private compare {
public:
    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:
    static constexpr size_t minimum_keys_in_root = 1;
    static constexpr size_t maximum_keys_in_root = 4 * t - 1;

    static constexpr size_t minimum_keys_in_node = 2 * t - 1;
    static constexpr size_t maximum_keys_in_node = 3 * t - 1;

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

    struct bsptree_node_base {
        bool _is_terminated;

        bsptree_node_base() noexcept;

        virtual size_t keys_size() = 0;

        virtual boost::container::static_vector<tkey, maximum_keys_in_node + 1> keys() = 0;

        virtual ~bsptree_node_base() = default;
    };

    struct bsptree_node_term : public bsptree_node_base {
        bsptree_node_term *_next;
        boost::container::static_vector<tree_data_type, maximum_keys_in_root + 1> _data;

        bsptree_node_term() noexcept;

        size_t keys_size() override { return _data.size(); }

        boost::container::static_vector<tkey, maximum_keys_in_node + 1> keys() override {
            auto keys = boost::container::static_vector<tkey, maximum_keys_in_node + 1>{};
            for (auto &elem: _data) {
                keys.push_back(elem.first);
            }
            return keys;
        };
    };

    struct bsptree_node_middle : public bsptree_node_base {
        boost::container::static_vector<tkey, maximum_keys_in_root + 1> _keys;
        boost::container::static_vector<bsptree_node_base *, maximum_keys_in_root + 2> _pointers;

        bsptree_node_middle() noexcept;

        size_t keys_size() override { return _keys.size(); }
        boost::container::static_vector<tkey, maximum_keys_in_node + 1> keys() override { return _keys; };
    };

    pp_allocator<value_type> _allocator;
    bsptree_node_base *_root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

public:
    // region constructors declaration

    explicit BSP_tree(const compare &cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit BSP_tree(pp_allocator<value_type> alloc, const compare &comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BSP_tree(iterator begin, iterator end, const compare &cmp = compare(),
                      pp_allocator<value_type> = pp_allocator<value_type>());

    BSP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp = compare(),
             pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    BSP_tree(const BSP_tree &other);

    BSP_tree(BSP_tree &&other) noexcept;

    BSP_tree &operator=(const BSP_tree &other);

    BSP_tree &operator=(BSP_tree &&other) noexcept;

    ~BSP_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class bsptree_iterator;
    class bsptree_const_iterator;

    class bsptree_iterator final {
        bsptree_node_term *_node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type &;
        using pointer = value_type *;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bsptree_iterator;

        friend class BSP_tree;
        friend class bsptree_const_iterator;

        reference operator*() const noexcept;

        pointer operator->() const noexcept;

        self &operator++();

        self operator++(int);

        bool operator==(const self &other) const noexcept;

        bool operator!=(const self &other) const noexcept;

        size_t current_node_keys_count() const noexcept;

        size_t index() const noexcept;

        explicit bsptree_iterator(bsptree_node_term *node = nullptr, size_t index = 0);
    };

    class bsptree_const_iterator final {
        const bsptree_node_term *_node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = const value_type &;
        using pointer = const value_type *;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bsptree_const_iterator;

        friend class BSP_tree;
        friend class bsptree_iterator;

        bsptree_const_iterator(const bsptree_iterator &it) noexcept;

        reference operator*() const noexcept;

        pointer operator->() const noexcept;

        self &operator++();

        self operator++(int);

        bool operator==(const self &other) const noexcept;

        bool operator!=(const self &other) const noexcept;

        size_t current_node_keys_count() const noexcept;

        size_t index() const noexcept;

        explicit bsptree_const_iterator(const bsptree_node_term *node = nullptr, size_t index = 0);
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

    bsptree_iterator begin();

    bsptree_iterator end();

    bsptree_const_iterator begin() const;

    bsptree_const_iterator end() const;

    bsptree_const_iterator cbegin() const;

    bsptree_const_iterator cend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;

    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    bsptree_iterator find(const tkey &key);

    bsptree_const_iterator find(const tkey &key) const;

    bsptree_iterator lower_bound(const tkey &key);

    bsptree_const_iterator lower_bound(const tkey &key) const;

    bsptree_iterator upper_bound(const tkey &key);

    bsptree_const_iterator upper_bound(const tkey &key) const;

    bool contains(const tkey &key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<bsptree_iterator, bool> insert(const tree_data_type &data);

    std::pair<bsptree_iterator, bool> insert(tree_data_type &&data);

    template<typename... Args>
    std::pair<bsptree_iterator, bool> emplace(Args &&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    bsptree_iterator insert_or_assign(const tree_data_type &data);

    bsptree_iterator insert_or_assign(tree_data_type &&data);

    template<typename... Args>
    bsptree_iterator emplace_or_assign(Args &&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    bsptree_iterator erase(bsptree_iterator pos);

    bsptree_iterator erase(bsptree_const_iterator pos);

    bsptree_iterator erase(bsptree_iterator beg, bsptree_iterator en);

    bsptree_iterator erase(bsptree_const_iterator beg, bsptree_const_iterator en);


    bsptree_iterator erase(const tkey &key);

    // endregion modifiers declaration

    // region debug functions

    void print_tree();

    void print_node(bsptree_node_base *node, int depth = 0);

    // endregion

private:
    // region helper functions

    bsptree_node_term *first_terminate_node();

    // endregion
};


// region debug impl

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::print_tree() {
    if (_root) print_node(_root);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::print_node(bsptree_node_base *node, int depth) {
    std::cout << std::string(depth, ' ');

    auto keys = node->keys();
    std::cout << '[';
    for (int i = 0; i < node->keys_size(); ++i) {
        std::cout << (keys[i]);
        if (i != node->keys_size() - 1) std::cout << "|";
    }
    std::cout << "]\n" << std::flush;

    if (auto node_middle = dynamic_cast<bsptree_node_middle *>(node)) {
        std::cout << "(" << keys[0] << ')';
        print_node(node_middle->_pointers[0], depth + 1);

        for (int i = 0; i < node->keys_size(); ++i) {
            if (node_middle->_pointers[i + 1]) {
                std::cout << '(' << keys[i] << ')';
                print_node(node_middle->_pointers[i + 1], depth + 1);
            }
        }
    }
}

// endregion debug impl


// region comparators impl

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type>
    compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
    std::size_t t = 5, typename U>
BSP_tree(iterator begin, iterator end, const compare &cmp = compare(),
         pp_allocator<U> = pp_allocator<U>()) -> BSP_tree<typename std::iterator_traits<
    iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BSP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp = compare(),
         pp_allocator<U> = pp_allocator<U>()) -> BSP_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::compare_pairs(const BSP_tree::tree_data_type &lhs,
                                                       const BSP_tree::tree_data_type &rhs) const {
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const {
    return compare::operator()(lhs, rhs);
}

// endregion comparators impl

// region bsptree_node_base implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_base::bsptree_node_base() noexcept : _is_terminated(false) {
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_term::bsptree_node_term() noexcept {
    this->_is_terminate = true;
    _next = nullptr;
    _data = boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1>{};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_middle::bsptree_node_middle() noexcept {
    this->_is_terminate = false;
    _keys = boost::container::static_vector<tkey, maximum_keys_in_node + 1>{};
    _pointers = boost::container::static_vector<bsptree_node_base *, maximum_keys_in_node + 2>{};
}

// endregion bsptree_node_base implementation

// region BSP_tree constructor implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BSP_tree<tkey, tvalue, compare, t>::value_type> BSP_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept {
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(const compare &cmp, pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(pp_allocator<value_type> alloc, const compare &cmp)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(iterator begin, iterator end, const compare &cmp,
                                             pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
    while (begin != end) {
        insert(*begin);
        ++begin;
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(std::initializer_list<std::pair<tkey, tvalue> > data, const compare &cmp,
                                             pp_allocator<value_type> alloc)
    : compare(cmp), _allocator(alloc), _root(nullptr), _size(0) {
    for (auto item: data) insert_or_assign(item);
}

// endregion BSP_tree constructor implementations

// region BSP_tree copy and move constructors

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(const BSP_tree &other)
    : compare(other), _allocator(other._allocator), _root(nullptr), _size(0) {
    for (auto it = other->begin(); it != other->end(); ++it) {
        insert(it);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(BSP_tree &&other) noexcept
    : compare(std::move(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size) {
    other._root = nullptr;
    other._size = 0;
}

// endregion BSP_tree copy and move constructors

// region BSP_tree copy and move assignment operators

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t> &BSP_tree<tkey, tvalue, compare, t>::operator=(const BSP_tree &other) {
    if (this != &other) {
        auto tmp = BP_tree(other);
        clear();
        *this = tmp;
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t> &BSP_tree<tkey, tvalue, compare, t>::operator=(BSP_tree &&other) noexcept {
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

// endregion BSP_tree copy and move assignment operators

// region BSP_tree desctructor

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::~BSP_tree() noexcept {
    clear();
}

// endregion BSP_tree desctructor

// region BSP_tree iterators implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::bsptree_iterator(bsptree_node_term *node, size_t index) {
    _node = node;
    _index = index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::reference BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator*() const noexcept {
    return reinterpret_cast<reference>(_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::pointer BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator->() const noexcept {
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator &BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator++() {
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
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::bsptree_iterator::operator++(int) {
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator==(const self &other) const noexcept {
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator!=(const self &other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::current_node_keys_count() const noexcept {
    return _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::index() const noexcept {
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::bsptree_const_iterator(const bsptree_node_term *node,
    size_t index) {
    _node = node;
    _index = index;
}


template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::bsptree_const_iterator(const bsptree_iterator &it) noexcept {
    _node = it._node;
    _index = it._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::reference BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator*() const noexcept {
    return reinterpret_cast<reference>(_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::pointer BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator->() const noexcept {
    return &operator*();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator &BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator++() {
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
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare,
    t>::bsptree_const_iterator::operator++(int) {
    auto iterator = *this;
    ++(*this);
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator==(const self &other) const noexcept {
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator!=(const self &other) const noexcept {
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::current_node_keys_count() const noexcept {
    return _node->_data.size();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::index() const noexcept {
    return _index;
}

// endregion BSP_tree iterators implementations

// region BSP_tree element access implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BSP_tree<tkey, tvalue, compare, t>::at(const tkey &key) {
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("This key doesn't exist!");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue &BSP_tree<tkey, tvalue, compare, t>::at(const tkey &key) const {
    auto iterator = find(key);
    if (iterator == end()) throw std::out_of_range("This key doesn't exist!");
    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BSP_tree<tkey, tvalue, compare, t>::operator[](const tkey &key) {
    auto iterator = find(key);
    if (iterator == end()) {
        /* добавляем элемент по умолчанию */
        iterator = insert(key, tvalue{});
    }

    return iterator->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue &BSP_tree<tkey, tvalue, compare, t>::operator[](tkey &&key) {
    return operator[](std::move(key));
}

// endregion BSP_tree element access implementations

// region BSP_tree iterator begins implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::begin() {
    auto node = first_terminate_node();
    return bsptree_iterator(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::end() {
    return bsptree_iterator();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::begin() const {
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::end() const {
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::cbegin() const {
    auto *node = (const_cast<BSP_tree *>(this))->first_terminate_node();
    return bsptree_const_iterator(node);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::cend() const {
    return bsptree_const_iterator();
}

// endregion BSP_tree iterator begins implementations

// region BSP_tree lookup implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::size() const noexcept {
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::empty() const noexcept {
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::find(const tkey &key) {
    if (_root == nullptr) return end();

    auto *node = _root;
    while (node) {
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
        if (node->_is_terminate && equal(keys[l], key))
            return bsptree_iterator(
                dynamic_cast<bsptree_node_term *>(node), l);

        /* мы уже в листовых узлах или нужно перейти в самого левого ребенка, значит l увеличивать не нужно */
        if (!(node->_is_terminate || less(key, keys[l]))) ++l;

        auto middle_node = dynamic_cast<bsptree_node_middle *>(node);
        if (middle_node != nullptr) node = middle_node->_pointers[l];
        else node = nullptr;
    }

    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::find(
    const tkey &key) const {
    return bptree_const_iterator(const_cast<BSP_tree *>(this)->find(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::lower_bound(const tkey &key) {
    if (_root == nullptr) return end();

    auto *node = _root;
    auto *last_node = node;
    size_t index = 0;
    while (node != nullptr) {
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
        if (node->_is_terminate && equal(keys[l], key))
            return bsptree_iterator(
                dynamic_cast<bsptree_node_term *>(node), l);

        /* уже дошли до листового узла или нужно перейти в самого левого ребенка, значит l увеличивать не нужно */
        if (!(node->_is_terminate || less(key, keys[l]))) ++l;

        auto middle_node = dynamic_cast<bsptree_node_middle *>(node);
        if (middle_node != nullptr) {
            node = middle_node->_pointers[l];
            last_node = node;
        } else node = nullptr;
        index = l;
    }

    auto iterator = bsptree_iterator(dynamic_cast<bsptree_node_term *>(last_node), index);
    /* в случае (index == 0 && key < (*iterator).first) мы уже находимся на элементе, который больше нашего */
    if (iterator->first == key || (index == 0 && key < iterator->first)) return iterator;
    return ++iterator; /* следующий элемент */
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::lower_bound(
    const tkey &key) const {
    return bstree_const_iterator(const_cast<BSP_tree *>(this)->lower_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::upper_bound(const tkey &key) {
    auto iterator = lower_bound(key);

    if (iterator == end()) return iterator;
    if (iterator->first == key) return ++iterator;
    return iterator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::upper_bound(
    const tkey &key) const {
    return bstree_const_iterator(const_cast<BSP_tree *>(this)->upper_bound(key));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::contains(const tkey &key) const {
    return find(key) != end();
}

// endregion BSP_tree lookup implementations

// region BSP_tree modifiers implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::clear() noexcept {
    while (!empty()) erase(begin());
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare,
    t>::insert(const tree_data_type &data) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare,
    t>::insert(tree_data_type &&data) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare,
    t>::emplace(Args &&... args) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> template<typename ...Args> std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(
    const tree_data_type &data) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(
    tree_data_type &&data) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename... Args>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::emplace_or_assign(Args &&... args) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> template<typename ...Args> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_iterator pos) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_iterator pos)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_const_iterator pos) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_const_iterator pos)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_iterator beg, bsptree_iterator en) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_iterator beg, bsptree_iterator en)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(
    bsptree_const_iterator beg, bsptree_const_iterator en) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_const_iterator beg, bsptree_const_iterator en)",
        "your code should be here...");
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare,
    t>::erase(const tkey &key) {
    throw not_implemented(
        "template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t> typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(const tkey& key)",
        "your code should be here...");
}

// endregion BSP_tree modifiers implementations

// region helper functions

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_node_term *BSP_tree<tkey, tvalue, compare,
    t>::first_terminate_node() {
    bsptree_node_base *node = _root;
    while (auto node_middle = dynamic_cast<bsptree_node_middle *>(node)) {
        node = node_middle->_pointers[0];
    }
    return dynamic_cast<bsptree_node_term *>(node);
}

// endregion


#endif
