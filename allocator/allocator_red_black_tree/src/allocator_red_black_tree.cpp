#include <not_implemented.h>

#include "../include/allocator_red_black_tree.h"

allocator_red_black_tree::~allocator_red_black_tree()
{
    auto* parent_allocator = parent_allocator_ref(_trusted_memory);
    auto& total_size = total_size_ref(_trusted_memory);
    parent_allocator->deallocate(_trusted_memory, total_size);
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

    total_size_ref(memory) = need_size;
    parent_allocator_ref(memory) = parent_allocator;
    fit_mode_ref(memory) = allocate_fit_mode;
    root_ref(memory) = static_cast<char*>(memory) + allocator_metadata_size;

    // инициализируем свободный блок
    auto* root = root_ref(memory);
    auto& block_data_root = block_data_ref(root);
    block_data_root.color = block_color::BLACK;
    block_data_root.occupied = false;

    left_ref(root) = nullptr;
    right_ref(root) = nullptr;
    next_ref(root) = nullptr;
    prev_ref(root) = nullptr;
    parent_ref(root) = nullptr;

    new (&mutex_ref(memory)) std::mutex();

    _trusted_memory = memory;
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
        return _trusted_memory == other_allocator->_trusted_memory;
    }
    return false;
}

[[nodiscard]] void *allocator_red_black_tree::do_allocate_sm(size_t size)
{
    if (_trusted_memory == nullptr) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_ref(_trusted_memory));

    void* result = nullptr;

    switch (fit_mode_ref(_trusted_memory)) {
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

    void* deleted_block = static_cast<char*>(at) - occupied_block_metadata_size;

    if (trusted_memory_ref(deleted_block) != _trusted_memory)
        throw std::logic_error("Попытка освободить чужую память!");

    block_data_ref(deleted_block).occupied = false;

    std::lock_guard<std::mutex> lock(mutex_ref(_trusted_memory));

    // если следующий блок - свободный
    auto* next_block = next_ref(deleted_block);
    if (next_block && block_data_ref(next_block).occupied == false) {
        // удаляем следующий блок, объединяем с текущим
        remove(next_block);

        next_ref(deleted_block) = next_ref(next_block);
    }

    // если предыдущий блок - свободный
    auto* prev_block = prev_ref(deleted_block);
    if (prev_block && block_data_ref(prev_block).occupied == false) {
        // удаляем предыдущий блок, объедияем с текущим
        remove(prev_block);

        next_ref(prev_block) = next_ref(deleted_block);
        deleted_block = prev_block;
    }

    left_ref(deleted_block) = nullptr;
    right_ref(deleted_block) = nullptr;
    parent_ref(deleted_block) = nullptr;

    add(deleted_block);
}

void allocator_red_black_tree::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    if (_trusted_memory == nullptr) return;
    fit_mode_ref(_trusted_memory) = mode;
}


std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const
{
    std::lock_guard<std::mutex> lock(mutex_ref(_trusted_memory));
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

    _block_ptr = *reinterpret_cast<void**>(static_cast<char*>(_block_ptr) + sizeof(block_data) + sizeof(void*));;
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

    auto* ptr_to_next_block = *reinterpret_cast<void**>(static_cast<char*>(_block_ptr) + sizeof(block_data) + sizeof(void*));;
    if (ptr_to_next_block == nullptr) {
        // следующего нет, тогда используем указатель на конец всей памяти
        size_t total_size = *reinterpret_cast<size_t*>(
                    static_cast<char*>(_trusted) +
                    sizeof(std::pmr::memory_resource*) +
                    sizeof(fit_mode));
        ptr_to_next_block = static_cast<char*>(_trusted) + total_size;
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
    auto [occupied, color] = *static_cast<block_data*>(_block_ptr);
    return occupied;
}

/* ----------------------- ALLOCATE LOGIC -------------------------------- */

void *allocator_red_black_tree::allocate_first_fit(size_t size) {
    const size_t need_size = size + occupied_block_metadata_size;

    auto* current_node = root_ref(_trusted_memory);
    while (current_node != nullptr) {
        auto* right_child = right_ref(current_node);

        size_t current_size = get_size_block(current_node);
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

void *allocator_red_black_tree::on_block_allocate(void *free_block, const size_t size) {
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
    const size_t block_size = get_size_block(free_block);

    // проверка, нужно ли разделить два блока:
    if (need_size + free_block_metadata_size < block_size) {
        // если вмещаются еще метаданные свободного блока, то делим

        // находим указатель на метаданные нового блока
        void* new_free_block = static_cast<char*>(free_block) + need_size;

        block_data_ref(new_free_block).occupied = false;
        block_data_ref(new_free_block).color = block_color::RED;
        next_ref(new_free_block) = next_ref(free_block);
        prev_ref(new_free_block) = free_block;
        right_ref(new_free_block) = nullptr;
        left_ref(new_free_block) = nullptr;
        parent_ref(new_free_block) = nullptr;

        next_ref(free_block) = new_free_block;

        add(new_free_block);
    }

    auto* new_block = free_block;

    trusted_memory_ref(new_block) = _trusted_memory;
    block_data_ref(new_block).occupied = true;
    return new_block;
}

/* ----------------------- RED BLACK TREE -------------------------------- */

int allocator_red_black_tree::compare_free_blocks(void *left, void *right, void *trusted) {
    size_t left_size = left ? get_size_block(left) : 0;
    size_t right_size = right ? get_size_block(right) : 0;

    if (left_size == right_size) return 0;
    if (left_size < right_size) return -1;
    return 1;
}

bool allocator_red_black_tree::is_red_parent(void *node) {
    auto* parent = parent_ref(node);
    return parent != nullptr && block_data_ref(parent).color == block_color::RED;
}

bool allocator_red_black_tree::is_red_left_child(void *node) {
    auto* left = left_ref(node);
    return left != nullptr && block_data_ref(left).color == block_color::RED;
}

bool allocator_red_black_tree::is_red_right_child(void *node) {
    auto* right = right_ref(node);
    return right != nullptr && block_data_ref(right).color == block_color::RED;
}

bool allocator_red_black_tree::is_left_child(void* node) {
    if (parent_ref(node) == nullptr) return false;

    return left_ref(parent_ref(node)) == node;
}

bool allocator_red_black_tree::is_right_child(void* node) {
    if (parent_ref(node) == nullptr) return false;

    return right_ref(parent_ref(node)) == node;
}

void allocator_red_black_tree::rotate_left(void *node) {
    // если корень, то ничего не делаем
    if (parent_ref(node) == nullptr) return;

    auto* parent = parent_ref(node);

    // меняем у родителя родителя ссылку на ребенка
    auto* parent_parent = parent_ref(parent);
    if (is_left_child(parent)) left_ref(parent_parent) = node;
    if (is_right_child(parent)) right_ref(parent_parent) = node;

    // меняем у основного узла ссылку на родителя
    parent_ref(node) = parent_ref(parent);

    // меняем левого потомка с правым потомком родителя
    auto* temp = left_ref(node);

    // основная логика
    left_ref(node) = parent;
    parent_ref(parent) = node;

    right_ref(parent) = temp;
    if (temp != nullptr) parent_ref(temp) = parent;

    // если мы свапнули с корнем, то корень изменился
    if (parent == root_ref(_trusted_memory)) root_ref(_trusted_memory) = node;
}

void allocator_red_black_tree::rotate_right(void *node) {
    // если корень, то ничего не делаем
    if (parent_ref(node) == nullptr) return;

    auto* parent = parent_ref(node);

    // меняем у родителя родителя ссылку на ребенка
    auto* parent_parent = parent_ref(parent);
    if (is_left_child(parent)) left_ref(parent_parent) = node;
    if (is_right_child(parent)) right_ref(parent_parent) = node;

    // меняем у основного узла ссылку на родителя
    parent_ref(node) = parent_ref(parent);

    // меняем правого потомка с левым потомком родителя
    auto* temp = right_ref(node);

    // основная логика
    right_ref(node)= parent;
    parent_ref(parent) = node;

    left_ref(parent) = temp;
    if (temp != nullptr) parent_ref(temp) = parent;

    // если мы свапнули с корнем, то корень изменился
    if (parent == root_ref(_trusted_memory)) root_ref(_trusted_memory) = node;
}

void allocator_red_black_tree::transplant(void *u, void *v) {
    auto* parent = parent_ref(u);
    if (parent == nullptr)
    {
        root_ref(_trusted_memory) = v;
    }
    else if (is_left_child(u))
    {
        left_ref(parent) = v;
    }
    else
    {
        right_ref(parent) = v;
    }
    parent_ref(v) = parent;
}

void allocator_red_black_tree::add(void *new_node) {
    auto* current_node = root_ref(_trusted_memory); // начинаем перебирать с корня
    auto* left_or_right_child = root_ref(_trusted_memory); // левый или правый потомок

    // в этом подходе учтено, если дерево пустое

    // вставляем в лист
    while (left_or_right_child != nullptr)
    {
        // если меньше или равно, то продолдаем поиск в левом поддереве
        // иначе продолджаем поиск, куда вставить узел, в правом поддереве
        current_node = left_or_right_child;

        int cmp = compare_free_blocks(new_node, left_or_right_child, _trusted_memory);
        if (cmp <= 0) left_or_right_child = left_ref(left_or_right_child);
        else left_or_right_child = right_ref(left_or_right_child);
    }

    // после этого цикла currentNode - родитель нового узла

    // определяем родителя у нового узла
    parent_ref(new_node) = current_node;
    // у родителя нового узла определяем сына (новый узел)
    int cmp = compare_free_blocks(new_node, current_node, _trusted_memory);
    if (current_node != nullptr && cmp <= 0) left_ref(current_node) = new_node;
    else if (current_node != nullptr && cmp > 0) right_ref(current_node) = new_node;


    if (current_node == nullptr) root_ref(_trusted_memory) = new_node;

    on_node_added(new_node);
}

void allocator_red_black_tree::on_node_added(void *new_node) {
    if (new_node == root_ref(_trusted_memory))
    {
        block_data_ref(new_node).color = block_color::BLACK;
        return;
    }

    auto* current_node = new_node;
    while (is_red_parent(current_node))
    {
        auto* father = parent_ref(current_node);
        auto* grandfather = parent_ref(father);

        if (is_left_child(father))
        {
            if (is_red_right_child(grandfather))
            {
                // если дядя тоже красный
                auto* uncle = right_ref(grandfather);
                block_data_ref(uncle).color = block_color::BLACK;
                block_data_ref(father).color = block_color::BLACK;
                block_data_ref(grandfather).color = block_color::RED;
                current_node = grandfather;
            }
            else
            // дядя черный или его нет
            {
                if (is_right_child(current_node))
                {
                    rotate_left(current_node);
                    // Теперь father это левый потомок currentNode.
                    // Теперь смотрим относительного него
                    auto* temp = father;
                    father = current_node;
                    current_node = temp;
                }

                block_data_ref(grandfather).color = block_color::RED;
                block_data_ref(father).color =  block_color::BLACK;

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
                auto* uncle = left_ref(grandfather);
                block_data_ref(uncle).color = block_color::BLACK;
                block_data_ref(father).color = block_color::BLACK;
                block_data_ref(grandfather).color = block_color::RED;
                current_node = grandfather;
            }
            else
            {
                // если дяди нет, или он черный
                if (is_left_child(current_node))
                {
                    rotate_right(current_node);
                    auto* temp = father;
                    father = current_node;
                    current_node = temp;
                }

                block_data_ref(grandfather).color = block_color::RED;
                block_data_ref(father).color = block_color::BLACK;

                rotate_left(father);

                // так как теперь отец стал "корнем"
                current_node = father;
            }
        }
    }
    if (root_ref(_trusted_memory) != nullptr) {
        auto* root = root_ref(_trusted_memory);
        block_data_ref(root).color = block_color::BLACK;
    }
}

void allocator_red_black_tree::remove(void *node) {
    auto* root = root_ref(_trusted_memory);

    // у узла нет детей
    if (left_ref(node) == nullptr && right_ref(node) == nullptr)
    {
        if (block_data_ref(node).color == block_color::BLACK) on_node_removed(node);

        if (is_left_child(node)) {
            auto* parent = parent_ref(node);
            left_ref(parent) = nullptr;
        }
        else if (is_right_child(node)) {
            auto* parent = parent_ref(node);
            right_ref(parent) = nullptr;
        }
        else root_ref(_trusted_memory) = nullptr; // удалили корень

        if (root_ref(_trusted_memory) != nullptr) {
            block_data_ref(root).color = block_color::BLACK;
        }

        parent_ref(node) = nullptr;
    }

    // у узла есть только правый ребенок
    else if (left_ref(node) == nullptr && right_ref(node) != nullptr)
    {
        auto* right = right_ref(node);
        transplant(node, right);

        if (block_data_ref(node).color == block_color::BLACK) on_node_removed(right);

        block_data_ref(root).color = block_color::BLACK;

        right_ref(node) = nullptr;
        parent_ref(node) = nullptr;
    }

    // у узла есть только левый ребенок
    else if (left_ref(node) != nullptr && right_ref(node) == nullptr)
    {
        auto* left = left_ref(node);
        transplant(node, left);

        if (block_data_ref(node).color == block_color::BLACK) on_node_removed(left);

        block_data_ref(root).color = block_color::BLACK;

        left_ref(node) = nullptr;
        parent_ref(node) = nullptr;
    }

    // два ребенка у узла
    else
    {
        auto* most_left_node_in_right_subtree = right_ref(node);

        while (left_ref(most_left_node_in_right_subtree) != nullptr)
        {
            most_left_node_in_right_subtree = left_ref(most_left_node_in_right_subtree);
        }

        auto* parent_delete_node = parent_ref(most_left_node_in_right_subtree);
        auto* child_most_left_node_in_right_subtree = right_ref(most_left_node_in_right_subtree);
        if (most_left_node_in_right_subtree == right_ref(node))
        {
            parent_delete_node = most_left_node_in_right_subtree;
        }
        else // mostLeftNodeInRightSubtree != node.Right
        {
            left_ref(parent_delete_node) = child_most_left_node_in_right_subtree;
            if (child_most_left_node_in_right_subtree) parent_ref(child_most_left_node_in_right_subtree) = parent_delete_node;
        }


        transplant(node, most_left_node_in_right_subtree);

        auto* node_left = left_ref(node);
        left_ref(most_left_node_in_right_subtree) = node_left;
        if (node_left) parent_ref(node_left) = most_left_node_in_right_subtree;


        if (most_left_node_in_right_subtree == right_ref(node))
        {
            auto* node_right = right_ref(node);
            right_ref(most_left_node_in_right_subtree) = right_ref(node_right);
        }
        else
        {
            right_ref(most_left_node_in_right_subtree) = right_ref(node);
        }
        auto* new_right_node = right_ref(most_left_node_in_right_subtree);
        if (new_right_node) parent_ref(new_right_node) = most_left_node_in_right_subtree;

        // заполнение для того, чтобы вызвать метод on_node_deleted
        if (child_most_left_node_in_right_subtree == nullptr)
        {
            child_most_left_node_in_right_subtree = node;
            parent_ref(child_most_left_node_in_right_subtree) = parent_delete_node;

            if (parent_delete_node == most_left_node_in_right_subtree)
            {
                right_ref(parent_delete_node) = child_most_left_node_in_right_subtree;
            }
            else
            {
                left_ref(parent_delete_node) = child_most_left_node_in_right_subtree;
            }
        }

        if (block_data_ref(most_left_node_in_right_subtree).color == block_color::BLACK)
            on_node_removed(child_most_left_node_in_right_subtree);

        if (child_most_left_node_in_right_subtree == node)
        {
            right_ref(child_most_left_node_in_right_subtree) = nullptr;
            auto* child_parent = parent_ref(child_most_left_node_in_right_subtree);
            if (is_left_child(child_most_left_node_in_right_subtree)) left_ref(child_parent) = nullptr;
            else if (is_right_child(child_most_left_node_in_right_subtree)) right_ref(child_parent) = nullptr;

            parent_ref(child_most_left_node_in_right_subtree) = nullptr;
        }

        right_ref(node) = nullptr;
        left_ref(node) = nullptr;
        parent_ref(node) = nullptr;

        block_data_ref(root).color = block_color::BLACK;
    }
}

void allocator_red_black_tree::on_node_removed(void *child) {
    if (child == nullptr) return; // балансировка не нужна

    auto* root = root_ref(_trusted_memory);

    while (block_data_ref(child).color == block_color::BLACK && child != root)
    {
        if (is_left_child(child))
        {
            auto* father = parent_ref(child);
            auto* brother = right_ref(father);

            if (brother != nullptr && block_data_ref(brother).color == block_color::RED)
                // случай 1: есть красный брат, надо повернуть дерево
            {
                block_data_ref(father).color = block_color::RED;
                block_data_ref(brother).color = block_color::BLACK;
                rotate_left(brother);

                // после поворота изменился брат
                brother = right_ref(father);
            }


            if (brother == nullptr)
            {
                block_data_ref(father).color = block_color::BLACK;
                child = father;
                continue;
            }

            // случай 2: брат черный
            // случай 2.1: у брата оба черных ребенка (в том числе если их нет, ведь лист черного цвета)
            auto* left_child_brother = left_ref(brother); // левый ребенок брата
            auto* right_child_brother = right_ref(brother); // правый ребенок брата

            if ((left_child_brother == nullptr || block_data_ref(left_child_brother).color == block_color::BLACK) &&
                (right_child_brother == nullptr || block_data_ref(right_child_brother).color == block_color::BLACK))
            {
                block_data_ref(brother).color = block_color::RED;

                child = father;
            }

            else
            // у брата есть хотя бы один не черный ребенок
            {
                // случай 2.2: у брата правый ребенок черный, а левый красный
                if (left_child_brother != nullptr && block_data_ref(left_child_brother).color == block_color::RED &&
                    (right_child_brother == nullptr || block_data_ref(right_child_brother).color == block_color::BLACK))
                {
                    block_data_ref(brother).color = block_color::RED;
                    block_data_ref(left_child_brother).color = block_color::BLACK;

                    rotate_right(left_child_brother);

                    brother = left_child_brother;
                    right_child_brother =  right_ref(brother);
                    left_child_brother = left_ref(brother);
                }

                // случай 2.3: у брата правый ребенок красный, а левый черный
                block_data_ref(brother).color = block_data_ref(father).color;
                block_data_ref(father).color = block_color::BLACK;
                block_data_ref(right_child_brother).color = block_color::BLACK;

                rotate_left(brother);

                child = root;
            }
        }
        else // child.IsRightChild
        {
            auto* father = parent_ref(child);
            auto* brother = left_ref(father);

            if (brother != nullptr && block_data_ref(brother).color == block_color::RED)
                // случай 1: есть красный брат, надо повернуть дерево
            {
                block_data_ref(father).color = block_color::RED;
                block_data_ref(brother).color = block_color::BLACK;
                rotate_right(brother);

                // после поворота изменился брат
                brother = left_ref(father);
            }

            if (brother == nullptr)
            {
                child = father;
                continue;
            }

            // случай 2: брат черный
            // случай 2.1: у брата оба черных ребенка (в том числе если их нет, ведь лист черного цвета)
            auto* left_child_brother = left_ref(brother); // левый ребенок брата
            auto* right_child_brother = right_ref(brother); // правый ребенок брата

            if ((left_child_brother == nullptr || block_data_ref(left_child_brother).color == block_color::BLACK) &&
                (right_child_brother == nullptr || block_data_ref(right_child_brother).color == block_color::BLACK))
            {
                block_data_ref(brother).color = block_color::RED;

                child = father;
            }

            else
                // у брата есть хотя бы один не черный ребенок
            {
                // случай 2.2: у брата левый ребенок черный, а правый красный
                if (right_child_brother != nullptr && block_data_ref(right_child_brother).color == block_color::RED &&
                    (left_child_brother == nullptr || block_data_ref(left_child_brother).color == block_color::BLACK))
                {
                    block_data_ref(brother).color = block_color::RED;
                    block_data_ref(right_child_brother).color = block_color::BLACK;

                    rotate_left(right_child_brother);

                    brother = right_child_brother;
                    left_child_brother = left_ref(brother);
                    right_child_brother = right_ref(brother);
                }

                // случай 2.3: у брата левый ребенок красный, а правый черный
                block_data_ref(brother).color = block_data_ref(father).color;
                block_data_ref(father).color = block_color::BLACK;
                block_data_ref(left_child_brother).color = block_color::BLACK;

                rotate_right(brother);

                child = root;
            }

        }
    }

    block_data_ref(child).color = block_color::BLACK;
    block_data_ref(root).color = block_color::BLACK;
}

std::vector<allocator_red_black_tree::free_block_debug_struct> allocator_red_black_tree::free_blocks() {
    std::vector<free_block_debug_struct> result;

    if (root_ref(_trusted_memory) != nullptr) free_blocks(root_ref(_trusted_memory), result);
    return result;
}

void allocator_red_black_tree::free_blocks(void* current_node, std::vector<free_block_debug_struct> &result) {
    if (left_ref(current_node) != nullptr) free_blocks(left_ref(current_node), result);
    free_block_debug_struct free_block{};
    free_block.color = block_data_ref(current_node).color;
    free_block.left = left_ref(current_node);
    free_block.right = right_ref(current_node);
    free_block.parent = parent_ref(current_node);
    free_block.next = next_ref(current_node);
    free_block.prev = prev_ref(current_node);
    free_block.size = get_size_block(current_node);
    result.push_back(free_block);
    if (right_ref(current_node) != nullptr) free_blocks(right_ref(current_node), result);
}


/* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO ALLOCATOR FIELDS ------------------------------ */
std::pmr::memory_resource*& allocator_red_black_tree::parent_allocator_ref(void* trusted) {
    return *reinterpret_cast<std::pmr::memory_resource**>(trusted);
}

allocator_with_fit_mode::fit_mode& allocator_red_black_tree::fit_mode_ref(void* trusted) {
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(
        static_cast<char*>(trusted) + sizeof(std::pmr::memory_resource*));
}

size_t& allocator_red_black_tree::total_size_ref(void* trusted) {
    return *reinterpret_cast<size_t*>(
        static_cast<char*>(trusted) +
        sizeof(std::pmr::memory_resource*) +
        sizeof(fit_mode));
}

std::mutex& allocator_red_black_tree::mutex_ref(void* trusted) const {
    return *reinterpret_cast<std::mutex*>(
        static_cast<char*>(trusted) +
        sizeof(std::pmr::memory_resource*) +
        sizeof(fit_mode) +
        sizeof(size_t));
}

void*& allocator_red_black_tree::root_ref(void* trusted) {
    return *reinterpret_cast<void**>(
        static_cast<char*>(trusted)
        + sizeof(std::pmr::memory_resource*)
        + sizeof(allocator_with_fit_mode::fit_mode)
        + sizeof(size_t)
        + sizeof(std::mutex));
}


/* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO COMMON BLOCK FIELDS ------------------------------ */

allocator_red_black_tree::block_data& allocator_red_black_tree::block_data_ref(void* block) {
    return *reinterpret_cast<block_data*>(block);
}

void*& allocator_red_black_tree::prev_ref(void* block) {
    return *reinterpret_cast<void**>(static_cast<char*>(block) + sizeof(block_data));
}

void*& allocator_red_black_tree::next_ref(void* block) {
    return *reinterpret_cast<void**>(static_cast<char*>(block) + sizeof(block_data) + sizeof(void*));
}

// void* const& allocator_red_black_tree::next_ref(const void* block) const {
//     return *reinterpret_cast<void* const*>(static_cast<const char*>(block) + sizeof(block_data) + sizeof(void*));
// }

size_t allocator_red_black_tree::get_size_block(void* block) {
    auto* ptr_to_next_block = next_ref(block);
    if (ptr_to_next_block == nullptr) {
        // следующего нет, тогда используем указатель на конец всей памяти
        ptr_to_next_block = static_cast<char*>(_trusted_memory) + total_size_ref(_trusted_memory);
    }

    return static_cast<const char*>(ptr_to_next_block) - static_cast<char*>(block);
}

/* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO OCCUPIED BLOCK FIELDS ------------------------------ */

void*& allocator_red_black_tree::trusted_memory_ref(void* block) {
    return *reinterpret_cast<void**>(static_cast<char*>(block) + sizeof(block_data) + 2 * sizeof(void*));
}

/* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO FREE BLOCK FIELDS ------------------------------ */

void*& allocator_red_black_tree::right_ref(void* block) {
    return *reinterpret_cast<void**>(static_cast<char*>(block) + sizeof(block_data) + 4 * sizeof(void*));
}

void*& allocator_red_black_tree::left_ref(void* block) {
    return *reinterpret_cast<void**>(static_cast<char*>(block) + sizeof(block_data) + 3 * sizeof(void*));
}

void*& allocator_red_black_tree::parent_ref(void* block) {
    return *reinterpret_cast<void**>(static_cast<char*>(block) + sizeof(block_data) + 2 * sizeof(void*));
}
