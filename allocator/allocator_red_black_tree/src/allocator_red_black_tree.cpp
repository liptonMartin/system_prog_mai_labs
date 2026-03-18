#include <not_implemented.h>

#include "../include/allocator_red_black_tree.h"

allocator_red_black_tree::~allocator_red_black_tree()
{
    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);
    metadata->parent_allocator->deallocate(_trusted_memory, metadata->size);
}

allocator_red_black_tree::allocator_red_black_tree(
    allocator_red_black_tree &&other) noexcept
    : _trusted_memory(other._trusted_memory) {
    other._trusted_memory = nullptr;
}

allocator_red_black_tree &allocator_red_black_tree::operator=(
    allocator_red_black_tree &&other) noexcept
{
    if (this != &other) {
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }

    return *this;
}

allocator_red_black_tree::allocator_red_black_tree(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (parent_allocator == nullptr) parent_allocator = std::pmr::get_default_resource();

    size_t need_size = space_size + allocator_metadata_size;

    void* memory = parent_allocator->allocate(need_size);

    auto* metadata = static_cast<allocator_metadata*>(memory);
    metadata->size = need_size;
    metadata->parent_allocator = parent_allocator;
    metadata->mode = allocate_fit_mode;
    metadata->root = static_cast<char*>(memory) + allocator_metadata_size;

    // инициализируем свободный блок
    auto* root = static_cast<free_block_metadata*>(metadata->root);
    root->common_metadata.data.color = block_color::BLACK;
    root->common_metadata.data.occupied = false;

    root->left = nullptr;
    root->right = nullptr;
    root->common_metadata.next = nullptr;
    root->common_metadata.prev = nullptr;
    root->parent = nullptr;

    new (&metadata->mutex) std::mutex();

    _trusted_memory = metadata;
}

allocator_red_black_tree::allocator_red_black_tree(const allocator_red_black_tree &other)
    : _trusted_memory(other._trusted_memory) {}

allocator_red_black_tree &allocator_red_black_tree::operator=(const allocator_red_black_tree &other)
{
    if (this != &other)
        _trusted_memory = other._trusted_memory;

    return *this;
}

bool allocator_red_black_tree::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto* other_allocator = dynamic_cast<const allocator_red_black_tree*>(&other);
    if (other_allocator != nullptr) {
        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
        auto* metadata_other_allocator = static_cast<allocator_metadata*>(other_allocator->_trusted_memory);
        return _trusted_memory == other_allocator->_trusted_memory &&
        metadata_allocator->parent_allocator == metadata_other_allocator->parent_allocator &&
            metadata_allocator->size == metadata_other_allocator->size &&
                metadata_allocator->mode == metadata_other_allocator->mode &&
                    metadata_allocator->root == metadata_other_allocator->root;
    }
    return false;
}

[[nodiscard]] void *allocator_red_black_tree::do_allocate_sm(size_t size)
{
    if (_trusted_memory == nullptr) return nullptr;

    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata->mutex);

    void* result = nullptr;

    switch (metadata->mode) {
        case (fit_mode::first_fit) : {
            result = allocate_first_fit(size);
            break;
        }

        case (fit_mode::the_best_fit) : {
            result = allocate_best_fit(size);
            break;
        }

        case (fit_mode::the_worst_fit) : {
            result = allocate_worst_fit(size);
            break;
        }
    }

    if (result == nullptr) throw std::bad_alloc();

    return static_cast<char*>(result) + occupied_block_metadata_size;
}


void allocator_red_black_tree::do_deallocate_sm(void *at)
{
    if (at == nullptr) throw std::logic_error("Попытка освободить несуществующую память!");

    void* ptr_metadata_deleted_block = static_cast<char*>(at) - occupied_block_metadata_size;

    auto* metadata_deleted_block = static_cast<occupied_block_metadata*>(ptr_metadata_deleted_block);
    if (metadata_deleted_block->trusted_memory != _trusted_memory)
        throw std::logic_error("Попытка освободить чужую память!");

    auto* metadata_new_free_block = static_cast<free_block_metadata *>(ptr_metadata_deleted_block);
    metadata_new_free_block->common_metadata.data.occupied = false;

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    std::lock_guard<std::mutex> lock(metadata_allocator->mutex);

    // если следующий блок - свободный
    auto* common_metadata_next_block = static_cast<common_block_metadata*>(metadata_new_free_block->common_metadata.next);
    if (common_metadata_next_block && common_metadata_next_block->data.occupied == false) {
        // удаляем следующий блок, объединяем с текущим
        auto* metadata_next_block = static_cast<free_block_metadata*>(metadata_new_free_block->common_metadata.next);
        remove(metadata_next_block);

        metadata_new_free_block->common_metadata.next = metadata_next_block->common_metadata.next;
    }

    // если предыдущий блок - свободный
    auto* common_metadata_prev_block = static_cast<common_block_metadata*>(metadata_new_free_block->common_metadata.prev);
    if (common_metadata_prev_block && common_metadata_prev_block->data.occupied == false) {
        // удаляем предыдущий блок, объедияем с текущим
        auto* metadata_prev_block = static_cast<free_block_metadata*>(metadata_new_free_block->common_metadata.prev);
        remove(metadata_prev_block);

        metadata_prev_block->common_metadata.next = metadata_new_free_block->common_metadata.next;
        metadata_new_free_block = metadata_prev_block;
    }

    metadata_new_free_block->left = nullptr;
    metadata_new_free_block->right = nullptr;
    metadata_new_free_block->parent = nullptr;

    add(metadata_new_free_block);
}

void allocator_red_black_tree::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    if (_trusted_memory == nullptr) return;
    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);
    metadata->mode = mode;
}


std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const
{
    if (_trusted_memory == nullptr) return std::vector<allocator_test_utils::block_info>();

    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata->mutex);

    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> blocks;

    for (auto it = begin(); it != end(); ++it) {
        allocator_test_utils::block_info info_block{};
        info_block.block_size = it.size();
        info_block.is_block_occupied = it.occupied();

        blocks.push_back(info_block);
    }

    return blocks;
}


allocator_red_black_tree::rb_iterator allocator_red_black_tree::begin() const noexcept
{
    return allocator_red_black_tree::rb_iterator(_trusted_memory);
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::end() const noexcept
{
    return allocator_red_black_tree::rb_iterator();
}


bool allocator_red_black_tree::rb_iterator::operator==(const allocator_red_black_tree::rb_iterator &other) const noexcept
{
    return _block_ptr == other._block_ptr && _trusted == other._trusted;
}

bool allocator_red_black_tree::rb_iterator::operator!=(const allocator_red_black_tree::rb_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_red_black_tree::rb_iterator &allocator_red_black_tree::rb_iterator::operator++() & noexcept
{
    if (_block_ptr == nullptr) {
        _trusted = nullptr; // чтобы прошла проверка на end()
        return *this;
    }

    auto* common_metadata = static_cast<common_block_metadata*>(_block_ptr);
    _block_ptr = common_metadata->next;
    if (_block_ptr == nullptr) _trusted = nullptr;
    return *this;
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::rb_iterator::operator++(int n)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

size_t allocator_red_black_tree::rb_iterator::size() const noexcept
{
    // возвращает весь размер, вместе с метаданными

    if (_block_ptr == nullptr) return 0;

    auto* common_metadata = static_cast<common_block_metadata*>(_block_ptr);
    void* ptr_to_next_block = common_metadata->next;
    if (ptr_to_next_block == nullptr) {
        // следующего нет, тогда используем указатель на конец всей памяти
        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted);
        ptr_to_next_block = static_cast<char*>(_trusted) + metadata_allocator->size;
    }

    return static_cast<char*>(ptr_to_next_block) - static_cast<char*>(_block_ptr);
}

void *allocator_red_black_tree::rb_iterator::operator*() const noexcept
{
    return _block_ptr;
}

allocator_red_black_tree::rb_iterator::rb_iterator()
    : _block_ptr(nullptr), _trusted(nullptr) {}

allocator_red_black_tree::rb_iterator::rb_iterator(void *trusted) : _trusted(trusted)
{
    _block_ptr = static_cast<char*>(trusted) + allocator_metadata_size;
}

bool allocator_red_black_tree::rb_iterator::occupied() const noexcept
{
    if (_block_ptr == nullptr) return true;

    const auto* common_data = static_cast<common_block_metadata*>(_block_ptr);
    return common_data->data.occupied;
}

/* ----------------------- ALLOCATE LOGIC -------------------------------- */

void *allocator_red_black_tree::allocate_first_fit(size_t size) {
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    const size_t need_size = size + occupied_block_metadata_size;

    auto* current_node = static_cast<free_block_metadata*>(metadata_allocator->root);
    while (current_node != nullptr) {
        auto* right_child = static_cast<free_block_metadata*>(current_node->right);

        size_t current_size = current_node->get_size(_trusted_memory);
        if (current_size < need_size) {
            current_node = right_child;
            continue;
        }

        void* result = on_block_allocate(current_node, size);
        return result;
    }

    return nullptr;
}

void *allocator_red_black_tree::allocate_best_fit(size_t size) {
    return nullptr;
}

void *allocator_red_black_tree::allocate_worst_fit(size_t size) {
    return nullptr;
}

void *allocator_red_black_tree::on_block_allocate(free_block_metadata *free_block, const size_t size) {
    /* free_block - указатель на свободный блок, в котором достаточно места
     * size - размер, сколько нужно выделить для конечного пользователя
     * return:: указатель на метаданные блока, который надо отддать пользователю
     *
     * реализуется вся логика после аллокации:
     * - удаляется узел из кчд
     * - при необходимости он делится (добавляется в кчд)
     * - заполняются поля у нового блока
     */

    remove(free_block);

    const size_t need_size = size + occupied_block_metadata_size;
    const size_t block_size = free_block->get_size(_trusted_memory);

    // проверка, нужно ли разделить два блока:
    if (need_size + free_block_metadata_size < block_size) {
        // если вмещаются еще метаданные свободного блока, то делим

        // находим указатель на метаданные нового блока
        void* ptr_to_new_free_block = reinterpret_cast<char*>(free_block) + need_size;
        auto* metadata_new_free_block = static_cast<free_block_metadata*>(ptr_to_new_free_block);

        metadata_new_free_block->common_metadata.data.occupied = false;
        metadata_new_free_block->common_metadata.next = free_block->common_metadata.next;
        metadata_new_free_block->common_metadata.prev = free_block;
        metadata_new_free_block->right = nullptr;
        metadata_new_free_block->left = nullptr;
        metadata_new_free_block->parent = nullptr;

        free_block->common_metadata.next = metadata_new_free_block;

        add(metadata_new_free_block);
    }

    auto* new_block = static_cast<occupied_block_metadata*>(static_cast<void*>(free_block));

    new_block->trusted_memory = _trusted_memory;
    new_block->common_metadata.data.occupied = true;
    return new_block;
}

/* ----------------------- RED BLACK TREE -------------------------------- */

size_t allocator_red_black_tree::free_block_metadata::get_size(void* trusted) const {
    void* ptr_to_next_block = this->common_metadata.next;
    if (ptr_to_next_block == nullptr) {
        // следующего нет, тогда используем указатель на конец всей памяти
        auto* metadata_allocator = static_cast<allocator_metadata*>(trusted);
        ptr_to_next_block = static_cast<char*>(trusted) + metadata_allocator->size;
    }

    return static_cast<char*>(ptr_to_next_block) - static_cast<const char*>(static_cast<const void*>(this));
}

int allocator_red_black_tree::compare_free_blocks(const free_block_metadata *left, const free_block_metadata *right, void *trusted) {
    size_t left_size = left ? left->get_size(trusted) : 0;
    size_t right_size = right ? right->get_size(trusted) : 0;

    if (left_size == right_size) return 0;
    if (left_size < right_size) return -1;
    return 1;
}

bool allocator_red_black_tree::is_red_parent(free_block_metadata *node) {
    return node->parent != nullptr && static_cast<free_block_metadata*>(node->parent)->common_metadata.data.color == block_color::RED;
}

bool allocator_red_black_tree::is_red_left_child(free_block_metadata *node) {
    return node->right != nullptr && static_cast<free_block_metadata*>(node->right)->common_metadata.data.color == block_color::RED;
}

bool allocator_red_black_tree::is_red_right_child(free_block_metadata *node) {
    return node->left != nullptr && static_cast<free_block_metadata*>(node->left)->common_metadata.data.color == block_color::RED;
}

bool allocator_red_black_tree::free_block_metadata::is_left_child() const {
    if (parent == nullptr) return false;
    auto* parent_metadata = static_cast<free_block_metadata*>(parent);
    return parent_metadata->left == this;
}

bool allocator_red_black_tree::free_block_metadata::is_right_child() const {
    if (parent == nullptr) return false;
    auto* parent_metadata = static_cast<free_block_metadata*>(parent);
    return parent_metadata->right == this;
}

void allocator_red_black_tree::rotate_left(void *ptr) {
    auto* node = static_cast<free_block_metadata*>(ptr);

    // если корень, то ничего не делаем
    if (node->parent == nullptr) return;

    auto* parent = static_cast<free_block_metadata*>(node->parent);

    // меняем у родителя родителя ссылку на ребенка
    auto* parent_parent = static_cast<free_block_metadata*>(parent->parent);
    if (parent->is_left_child()) parent_parent->left = ptr;
    if (parent->is_right_child()) parent_parent->right = ptr;

    // меняем у основного узла ссылку на родителя
    node->parent = parent->parent;

    // меняем левого потомка с правым потомком родителя
    auto* temp = static_cast<free_block_metadata*>(node->left);

    // основная логика
    node->left = parent;
    parent->parent = node;

    parent->right = temp;
    if (temp != nullptr) temp->parent = parent;

    // если мы свапнули с корнем, то корень изменился
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    if (parent == metadata_allocator->root) metadata_allocator->root = node;
}

void allocator_red_black_tree::rotate_right(void *ptr) {
    auto* node = static_cast<free_block_metadata*>(ptr);

    // если корень, то ничего не делаем
    if (node->parent == nullptr) return;

    auto* parent = static_cast<free_block_metadata*>(node->parent);

    // меняем у родителя родителя ссылку на ребенка
    auto* parent_parent = static_cast<free_block_metadata*>(parent->parent);
    if (parent->is_left_child()) parent_parent->left = ptr;
    if (parent->is_right_child()) parent_parent->right = ptr;

    // меняем у основного узла ссылку на родителя
    node->parent = parent->parent;

    // меняем правого потомка с левым потомком родителя
    auto* temp = static_cast<free_block_metadata*>(node->right);

    // основная логика
    node->right = parent;
    parent->parent = node;

    parent->left = temp;
    if (temp != nullptr) temp->parent = parent;

    // если мы свапнули с корнем, то корень изменился
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    if (parent == metadata_allocator->root) metadata_allocator->root = node;
}

void allocator_red_black_tree::transplant(free_block_metadata *u, free_block_metadata *v) {
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    auto* parent = static_cast<free_block_metadata*>(u->parent);
    if (parent == nullptr)
    {
        metadata_allocator->root = v;
    }
    else if (u->is_left_child())
    {
        parent->left = v;
    }
    else
    {
        parent->right = v;
    }
    v->parent = parent;
}

void allocator_red_black_tree::add(free_block_metadata *new_node) {
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    auto* current_node = static_cast<free_block_metadata*>(metadata_allocator->root); // начинаем перебирать с корня
    auto* left_or_right_child = static_cast<free_block_metadata*>(metadata_allocator->root); // левый или правый потомок

    // в этом подходе учтено, если дерево пустое

    // вставляем в лист
    while (left_or_right_child != nullptr)
    {
        // если меньше или равно, то продолдаем поиск в левом поддереве
        // иначе продолджаем поиск, куда вставить узел, в правом поддереве
        current_node = left_or_right_child;

        int cmp = compare_free_blocks(new_node, left_or_right_child, _trusted_memory);
        if (cmp <= 0) left_or_right_child = static_cast<free_block_metadata *>(left_or_right_child->left);
        else left_or_right_child = static_cast<free_block_metadata *>(left_or_right_child->right);
    }

    // после этого цикла currentNode - родитель нового узла

    // определяем родителя у нового узла
    new_node->parent = current_node;
    // у родителя нового узла определяем сына (новый узел)
    int cmp = compare_free_blocks(new_node, current_node, _trusted_memory);
    if (current_node != nullptr && cmp <= 0) current_node->left = new_node;
    else if (current_node != nullptr && cmp > 0) current_node->right = new_node;


    if (current_node == nullptr) metadata_allocator->root = new_node;

    on_node_added(new_node);
}

void allocator_red_black_tree::on_node_added(free_block_metadata *new_node) {
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    if (new_node == metadata_allocator->root)
    {
        new_node->common_metadata.data.color = block_color::BLACK;
        return;
    }

    auto* current_node = new_node;
    while (is_red_parent(current_node))
    {
        auto* father = static_cast<free_block_metadata*>(current_node->parent);
        auto* grandfather = static_cast<free_block_metadata*>(father->parent);

        if (father->is_left_child())
        {
            if (is_red_right_child(grandfather))
            {
                // если дядя тоже красный
                auto* uncle = static_cast<free_block_metadata*>(grandfather->right);
                uncle->common_metadata.data.color = block_color::BLACK;
                father->common_metadata.data.color = block_color::BLACK;
                grandfather->common_metadata.data.color = block_color::RED;
                current_node = grandfather;
            }
            else
            // дядя черный или его нет
            {
                if (current_node->is_right_child())
                {
                    rotate_left(current_node);
                    // Теперь father это левый потомок currentNode.
                    // Теперь смотрим относительного него
                    auto* temp = father;
                    father = current_node;
                    current_node = temp;
                }

                grandfather->common_metadata.data.color = block_color::RED;
                father->common_metadata.data.color =  block_color::BLACK;

                rotate_right(father);

                // так как теперь отец стал "корнем"
                current_node = father;
            }
        }
        else // father.is_right_child()
        {
            if (is_red_left_child(grandfather))
            {
                // если дядя тоже красный
                auto* uncle = static_cast<free_block_metadata*>(grandfather->left);
                uncle->common_metadata.data.color = block_color::BLACK;
                father->common_metadata.data.color = block_color::BLACK;
                grandfather->common_metadata.data.color = block_color::RED;
                current_node = grandfather;
            }
            else
            {
                // если дяди нет, или он черный
                if (current_node->is_left_child())
                {
                    rotate_right(current_node);
                    auto* temp = father;
                    father = current_node;
                    current_node = temp;
                }

                grandfather->common_metadata.data.color = block_color::RED;
                father->common_metadata.data.color = block_color::BLACK;

                rotate_left(father);

                // так как теперь отец стал "корнем"
                current_node = father;
            }
        }
    }
    if (metadata_allocator->root != nullptr) {
        auto* root = static_cast<free_block_metadata *>(metadata_allocator->root);
        root->common_metadata.data.color = block_color::BLACK;
    }
}

void allocator_red_black_tree::remove(free_block_metadata *node) {
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    auto* root = static_cast<free_block_metadata *>(metadata_allocator->root);

    // у узла нет детей
    if (node->left == nullptr && node->right == nullptr)
    {
        if (node->common_metadata.data.color == block_color::BLACK) on_node_removed(node);

        if (node->is_left_child()) {
            auto* parent = static_cast<free_block_metadata*>(node->parent);
            parent->left = nullptr;
        }
        else if (node->is_right_child()) {
            auto* parent = static_cast<free_block_metadata*>(node->parent);
            parent->right = nullptr;
        }
        else metadata_allocator->root = nullptr; // удалили корень

        if (metadata_allocator->root != nullptr) {
            root->common_metadata.data.color = block_color::BLACK;
        }

        node->parent = nullptr;
    }

    // у узла есть только правый ребенок
    else if (node->left == nullptr && node->right != nullptr)
    {
        auto* right = static_cast<free_block_metadata *>(node->right);
        transplant(node, right);

        if (node->common_metadata.data.color == block_color::BLACK) on_node_removed(right);

        root->common_metadata.data.color = block_color::BLACK;

        node->right = nullptr;
        node->parent = nullptr;
    }

    // у узла есть только левый ребенок
    else if (node->left != nullptr && node->right == nullptr)
    {
        auto* left = static_cast<free_block_metadata *>(node->left);
        transplant(node, left);

        if (node->common_metadata.data.color == block_color::BLACK) on_node_removed(left);

        root->common_metadata.data.color = block_color::BLACK;

        node->left = nullptr;
        node->parent = nullptr;
    }

    // два ребенка у узла
    else
    {
        auto* most_left_node_in_right_subtree = static_cast<free_block_metadata*>(node->right);

        while (most_left_node_in_right_subtree->left != nullptr)
        {
            most_left_node_in_right_subtree = static_cast<free_block_metadata *>(most_left_node_in_right_subtree->left);
        }

        auto* parent_delete_node =  static_cast<free_block_metadata*>(most_left_node_in_right_subtree->parent);
        auto* child_most_left_node_in_right_subtree = static_cast<free_block_metadata *>(most_left_node_in_right_subtree->right);
        if (most_left_node_in_right_subtree == node->right)
        {
            parent_delete_node = most_left_node_in_right_subtree;
        }
        else // mostLeftNodeInRightSubtree != node.Right
        {
            parent_delete_node->left = child_most_left_node_in_right_subtree;
            if (child_most_left_node_in_right_subtree) child_most_left_node_in_right_subtree->parent = parent_delete_node;
        }


        transplant(node, most_left_node_in_right_subtree);

        auto* node_left = static_cast<free_block_metadata *>(node->left);
        most_left_node_in_right_subtree->left = node_left;
        if (node_left) node_left->parent = most_left_node_in_right_subtree;


        if (most_left_node_in_right_subtree == node->right)
        {
            auto* node_right = static_cast<free_block_metadata *>(node->right);
            most_left_node_in_right_subtree->right = node_right->right;
        }
        else
        {
            most_left_node_in_right_subtree->right = node->right;
        }
        auto* new_right_node = static_cast<free_block_metadata *>(most_left_node_in_right_subtree->right);
        if (new_right_node) new_right_node->parent = most_left_node_in_right_subtree;

        // заполнение для того, чтобы вызвать метод on_node_deleted
        if (child_most_left_node_in_right_subtree == nullptr)
        {
            child_most_left_node_in_right_subtree = node;
            child_most_left_node_in_right_subtree->parent = parent_delete_node;

            if (parent_delete_node == most_left_node_in_right_subtree)
            {
                parent_delete_node->right = child_most_left_node_in_right_subtree;
            }
            else
            {
                parent_delete_node->left = child_most_left_node_in_right_subtree;
            }
        }

        if (most_left_node_in_right_subtree->common_metadata.data.color == block_color::BLACK)
            on_node_removed(child_most_left_node_in_right_subtree);

        if (child_most_left_node_in_right_subtree == node)
        {
            child_most_left_node_in_right_subtree->right = nullptr;
            auto* child_parent = static_cast<free_block_metadata *>(child_most_left_node_in_right_subtree->parent);
            if (child_most_left_node_in_right_subtree->is_left_child()) child_parent->left = nullptr;
            else if (child_most_left_node_in_right_subtree->is_right_child()) child_parent->right = nullptr;

            child_most_left_node_in_right_subtree->parent = nullptr;
        }

        node->right = nullptr;
        node->left = nullptr;
        node->parent = nullptr;

        root->common_metadata.data.color = block_color::BLACK;
    }
}

void allocator_red_black_tree::on_node_removed(free_block_metadata *child) {
    if (child == nullptr) return; // балансировка не нужна

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    auto* root = static_cast<free_block_metadata *>(metadata_allocator->root);

    while (child->common_metadata.data.color == block_color::BLACK && child != root)
    {
        if (child->is_left_child())
        {
            auto* father = static_cast<free_block_metadata *>(child->parent);
            auto* brother = static_cast<free_block_metadata *>(father->right);

            if (brother != nullptr && brother->common_metadata.data.color == block_color::RED)
                // случай 1: есть красный брат, надо повернуть дерево
            {
                father->common_metadata.data.color = block_color::RED;
                brother->common_metadata.data.color = block_color::BLACK;
                rotate_left(brother);

                // после поворота изменился брат
                brother = static_cast<free_block_metadata *>(father->right);
            }


            if (brother == nullptr)
            {
                father->common_metadata.data.color = block_color::BLACK;
                child = father;
                continue;
            }

            // случай 2: брат черный
            // случай 2.1: у брата оба черных ребенка (в том числе если их нет, ведь лист черного цвета)
            auto* left_child_brother = static_cast<free_block_metadata *>(brother->left); // левый ребенок брата
            auto* right_child_brother = static_cast<free_block_metadata *>(brother->right); // правый ребенок брата

            if ((left_child_brother == nullptr || left_child_brother->common_metadata.data.color == block_color::BLACK) &&
                (right_child_brother == nullptr || right_child_brother->common_metadata.data.color == block_color::BLACK))
            {
                brother->common_metadata.data.color = block_color::RED;

                child = father;
            }

            else
            // у брата есть хотя бы один не черный ребенок
            {
                // случай 2.2: у брата правый ребенок черный, а левый красный
                if (left_child_brother != nullptr && left_child_brother->common_metadata.data.color == block_color::RED &&
                    (right_child_brother == nullptr || right_child_brother->common_metadata.data.color == block_color::BLACK))
                {
                    brother->common_metadata.data.color = block_color::RED;
                    left_child_brother->common_metadata.data.color = block_color::BLACK;

                    rotate_right(left_child_brother);

                    brother = left_child_brother;
                    right_child_brother =  static_cast<free_block_metadata *>(brother->right);
                    left_child_brother = static_cast<free_block_metadata *>(brother->left);
                }

                // случай 2.3: у брата правый ребенок красный, а левый черный
                brother->common_metadata.data.color = father->common_metadata.data.color;
                father->common_metadata.data.color = block_color::BLACK;
                right_child_brother->common_metadata.data.color = block_color::BLACK;

                rotate_left(brother);

                child = root;
            }
        }
        else // child.IsRightChild
        {
            auto* father = static_cast<free_block_metadata *>(child->parent);
            auto* brother = static_cast<free_block_metadata *>(father->left);

            if (brother != nullptr && brother->common_metadata.data.color == block_color::RED)
                // случай 1: есть красный брат, надо повернуть дерево
            {
                father->common_metadata.data.color = block_color::RED;
                brother->common_metadata.data.color = block_color::BLACK;
                rotate_right(brother);

                // после поворота изменился брат
                brother = static_cast<free_block_metadata *>(father->left);
            }

            if (brother == nullptr)
            {
                child = father;
                continue;
            }

            // случай 2: брат черный
            // случай 2.1: у брата оба черных ребенка (в том числе если их нет, ведь лист черного цвета)
            auto* left_child_brother = static_cast<free_block_metadata*>(brother->left); // левый ребенок брата
            auto* right_child_brother = static_cast<free_block_metadata*>(brother->right); // правый ребенок брата

            if ((left_child_brother == nullptr || left_child_brother->common_metadata.data.color == block_color::BLACK) &&
                (right_child_brother == nullptr || right_child_brother->common_metadata.data.color == block_color::BLACK))
            {
                brother->common_metadata.data.color = block_color::RED;

                child = father;
            }

            else
                // у брата есть хотя бы один не черный ребенок
            {
                // случай 2.2: у брата левый ребенок черный, а правый красный
                if (right_child_brother != nullptr && right_child_brother->common_metadata.data.color == block_color::RED &&
                    (left_child_brother == nullptr || left_child_brother->common_metadata.data.color == block_color::BLACK))
                {
                    brother->common_metadata.data.color = block_color::RED;
                    right_child_brother->common_metadata.data.color = block_color::BLACK;

                    rotate_left(right_child_brother);

                    brother = right_child_brother;
                    left_child_brother =  static_cast<free_block_metadata *>(brother->left);
                    right_child_brother = static_cast<free_block_metadata *>(brother->right);
                }

                // случай 2.3: у брата левый ребенок красный, а правый черный
                brother->common_metadata.data.color = father->common_metadata.data.color;
                father->common_metadata.data.color = block_color::BLACK;
                left_child_brother->common_metadata.data.color = block_color::BLACK;

                rotate_right(brother);

                child = root;
            }

        }
    }

    child->common_metadata.data.color = block_color::BLACK;
    root->common_metadata.data.color = block_color::BLACK;
}

std::vector<std::pair<allocator_red_black_tree::free_block_metadata, size_t>> allocator_red_black_tree::free_blocks() {
    std::vector<std::pair<free_block_metadata, size_t>> result;
    auto* metadata = static_cast<allocator_metadata *>(_trusted_memory);

    if (metadata->root != nullptr) free_blocks(static_cast<free_block_metadata *>(metadata->root), result);
    return result;
}

void allocator_red_black_tree::free_blocks(free_block_metadata* current_node, std::vector<std::pair<free_block_metadata, size_t>> &result) {
    if (current_node->right != nullptr) free_blocks(static_cast<free_block_metadata *>(current_node->right), result);
    result.emplace_back(*current_node, current_node->get_size(_trusted_memory));
    if (current_node->left != nullptr) free_blocks(static_cast<free_block_metadata *>(current_node->left), result);
}
