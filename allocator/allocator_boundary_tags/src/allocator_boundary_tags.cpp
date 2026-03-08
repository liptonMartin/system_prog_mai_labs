#include <not_implemented.h>
#include "../include/allocator_boundary_tags.h"

allocator_boundary_tags::~allocator_boundary_tags()
{
    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);
    metadata->parent_allocator->deallocate(_trusted_memory, metadata->size);
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept :
    _trusted_memory(other._trusted_memory) {}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (this != &other) {
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }

    return *this;
}


/** If parent_allocator* == nullptr you should use std::pmr::get_default_resource()
 */
allocator_boundary_tags::allocator_boundary_tags(
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
    metadata->allocate_fit_mode = allocate_fit_mode;
    metadata->first_occupied_block = nullptr;
    new (&metadata->mutex) std::mutex();

    _trusted_memory = metadata;
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(size_t size)
{
    if (_trusted_memory == nullptr) return nullptr;

    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata->mutex);

    void* result = nullptr;

    switch (metadata->allocate_fit_mode) {
        case (fit_mode::first_fit) : {
            result = allocate_first_fit(size);
            if (result == nullptr) throw std::bad_alloc();
        }

        case (fit_mode::the_best_fit) : {
            result = allocate_best_fit(size);
            if (result == nullptr) throw std::bad_alloc();
        }

        case (fit_mode::the_worst_fit) : {
            result = allocate_worst_fit(size);
            if (result == nullptr) throw std::bad_alloc();
        }
    }

    // auto* metadata_new_occupied_ptr = static_cast<occupied_block_metadata*>(result);
    // void* ptr_to_next_block_or_end = nullptr;

    return result;
}

void allocator_boundary_tags::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;

    void* ptr_metadata_deleted_block = static_cast<char*>(at) - occupied_block_metadata_size;

    auto* metadata_deleted_block = static_cast<occupied_block_metadata*>(ptr_metadata_deleted_block);
    if (metadata_deleted_block->trusted_memory != _trusted_memory) return;

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    std::lock_guard<std::mutex> lock(metadata_allocator->mutex);

    auto* metadata_prev_block = static_cast<occupied_block_metadata*>(metadata_deleted_block->back_occupied_ptr);
    auto* metadata_next_block = static_cast<occupied_block_metadata*>(metadata_deleted_block->next_occupied_ptr);

    if (metadata_prev_block != nullptr) metadata_prev_block->next_occupied_ptr = metadata_next_block;
    if (metadata_next_block != nullptr) metadata_next_block->back_occupied_ptr = metadata_prev_block;

    if (metadata_allocator->first_occupied_block == metadata_deleted_block) {
        metadata_allocator->first_occupied_block = metadata_next_block;
    }
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    if (_trusted_memory == nullptr) return;
    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);
    metadata->allocate_fit_mode = mode;
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    if (_trusted_memory == nullptr) return std::vector<allocator_test_utils::block_info>();

    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata->mutex);

    return get_blocks_info_inner();
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    return boundary_iterator(_trusted_memory);
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);
    if (_trusted_memory != nullptr) {
        void* end_ptr = static_cast<char*>(_trusted_memory) + metadata->size;
        return boundary_iterator(end_ptr, _trusted_memory);
    }
    return boundary_iterator();
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
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

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other) :
    _trusted_memory(other._trusted_memory) {}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    if (this != &other)
        _trusted_memory = other._trusted_memory;

    return *this;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto* other_allocator = dynamic_cast<const allocator_boundary_tags*>(&other);
    if (other_allocator != nullptr) {
        return _trusted_memory == other_allocator->_trusted_memory;
    }
    return false;
}

bool allocator_boundary_tags::boundary_iterator::operator==(
        const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
     return _occupied_ptr == other._occupied_ptr && _occupied == other._occupied
        && _trusted_memory == other._trusted_memory;
}

bool allocator_boundary_tags::boundary_iterator::operator!=(
        const allocator_boundary_tags::boundary_iterator & other) const noexcept
{
    return !(*this == other);
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    if (_occupied_ptr == _get_ptr_to_end_allocator()) {
        // дошли до конца
        return *this;
    }

    if (_occupied_ptr == nullptr) {
        // свободный блок перед занятыми
        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
        if (metadata_allocator->first_occupied_block == nullptr) {
            // один единственный свободный блок
            _occupied_ptr = _get_ptr_to_end_allocator();
        }

        else {
            _occupied_ptr = metadata_allocator->first_occupied_block;
        }

        _occupied = true;
        return *this;
    }

    const occupied_block_metadata* block_metadata = static_cast<occupied_block_metadata*>(_occupied_ptr);
    if (_occupied) {
        void* ptr_to_metadata_next_occupied_block = block_metadata->next_occupied_ptr;
        if (static_cast<char*>(_occupied_ptr) + block_metadata->size == ptr_to_metadata_next_occupied_block) {
            // свободного блока между двумя занятыми нет
            _occupied_ptr = ptr_to_metadata_next_occupied_block;
            _occupied = true;
        }
        else {
            // есть свободный блок между двумя занятыми => значит интерпретируем текущую память как свободный блок
            _occupied = false;
        }
    }

    else {
        _occupied = true;
        if (block_metadata->next_occupied_ptr != nullptr) _occupied_ptr = block_metadata->next_occupied_ptr;
        else _occupied_ptr = _get_ptr_to_end_allocator();
    }

    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    if (_occupied_ptr == nullptr) {
        // _occupied_ptr == nullptr, только если это свободный блок перед всеми занятыми -> дошли до начала
        return *this;
    }
    if (_occupied == true && _occupied_ptr == _get_ptr_to_begin_allocator()) {
        // уже в самом начале
        return *this;
    }

    const occupied_block_metadata* block_metadata = static_cast<occupied_block_metadata*>(_occupied_ptr);
    if (_occupied) {
        void* ptr_to_occupied_old_block = _occupied_ptr;
        if (block_metadata->back_occupied_ptr != nullptr)
            _occupied_ptr = block_metadata->back_occupied_ptr;
        else _occupied_ptr = _get_ptr_to_begin_allocator();

        // в _occupied_ptr уже предыдщий блок
        auto* metadata_prev_occupied_block = static_cast<occupied_block_metadata*>(_occupied_ptr);
        if (static_cast<char*>(_occupied_ptr) + metadata_prev_occupied_block->size == ptr_to_occupied_old_block) {
            // тогда свободного блока между занятыми нет
            _occupied = true;
        }
        else {
            // тогда есть свободный блок между двумя занятыми
            _occupied = false;
        }
    }
    else _occupied = true; // если мы были на свободном, то по любому есть занятый предыдущий

    return *this;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    const boundary_iterator iterator = *this;
    ++(*this);
    return iterator;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    const boundary_iterator iterator = *this;
    --(*this);
    return iterator;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    if (_occupied_ptr != nullptr) {
        const occupied_block_metadata* block_metadata = static_cast<occupied_block_metadata*>(_occupied_ptr);
        if (_occupied) {
            return block_metadata->size;
        }
        else {
            void* free_block_ptr = static_cast<char*>(_occupied_ptr) + block_metadata->size;
            void* next_occupied_block_ptr = block_metadata->next_occupied_ptr;
            if (next_occupied_block_ptr == nullptr) {
                const auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
                next_occupied_block_ptr = static_cast<char*>(_trusted_memory) + metadata_allocator->size;
            }
            return static_cast<char*>(next_occupied_block_ptr) - static_cast<char*>(free_block_ptr);
        }
    }

    else {
        // тогда в аллокаторе первый свободный блок
        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
        if (metadata_allocator->first_occupied_block == nullptr) return metadata_allocator->size - allocator_metadata_size;

        auto* ptr_to_first_occupied_block = static_cast<char*>(metadata_allocator->first_occupied_block);
        char* ptr_to_first_free_block = static_cast<char*>(_trusted_memory) + allocator_metadata_size;
        return ptr_to_first_occupied_block - ptr_to_first_free_block;
    }
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    return _occupied;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    return get_ptr();
}

allocator_boundary_tags::boundary_iterator::boundary_iterator() :
    _occupied_ptr(nullptr),
    _occupied(false),
    _trusted_memory(nullptr) {}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted) :
    _occupied_ptr(nullptr),
    _occupied(false),
    _trusted_memory(trusted)
{
    if (trusted != nullptr) {
        const allocator_metadata* metadata = static_cast<allocator_metadata*>(trusted);
        void* ptr_to_first_block_after_allocator_metadata = static_cast<char*>(trusted) + metadata->size;
        if (metadata->first_occupied_block == ptr_to_first_block_after_allocator_metadata) {
            _occupied = true;
        }
        else {
            _occupied_ptr = nullptr; // первый блок свободный
            _occupied = false;
        }
    }
}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void* occupied_ptr, void* trusted) :
    _occupied_ptr(occupied_ptr),
    _occupied(true),
    _trusted_memory(trusted) {}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    if (_occupied_ptr == nullptr) {
        // свободный блок перед всеми занятыми
        return static_cast<char*>(_trusted_memory) + allocator_metadata_size;
    }

    const auto* block_metadata = static_cast<occupied_block_metadata*>(_occupied_ptr);

    if (_occupied) {
        return static_cast<char*>(_occupied_ptr) + occupied_block_metadata_size;
    }
    else {
        return static_cast<char*>(_occupied_ptr) + block_metadata->size;
    }
}

void *allocator_boundary_tags::allocate_first_fit(const size_t useful_size) {
    const size_t need_size = useful_size + occupied_block_metadata_size;
    for (auto it = begin(); it != end(); ++it) {
        if (it.occupied() == true || it.size() < need_size) continue;

        void* metadata_new_occupied_block = nullptr;
        if (it == begin()) {
            // тогда свободный блок перед всеми занятыми
            metadata_new_occupied_block = _link_new_occupied_block(nullptr, need_size);
        }
        else {
            // тогда нужно получить указатель на занятый блок
            --it;
            void* ptr_occupied_block = *it;
            void* ptr_to_metadata_occupied_block = static_cast<char*>(ptr_occupied_block) - occupied_block_metadata_size;
            metadata_new_occupied_block = _link_new_occupied_block(ptr_to_metadata_occupied_block, need_size);
        }

        return static_cast<char*>(metadata_new_occupied_block) + occupied_block_metadata_size;
    }

    return nullptr;
}

void *allocator_boundary_tags::allocate_best_fit(const size_t useful_size) {
    const size_t need_size = useful_size + occupied_block_metadata_size;

    const auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    size_t best_size = metadata_allocator->size; // больше этого значения не выделить
    void* ptr_occupied_block_before_best_free_block = nullptr; // причем если свободный блок перед всеми занятыми, то nullptr
    bool is_find = false;

    for (auto it = begin(); it != end(); ++it) {
        if (it.occupied() == true || it.size() < need_size) continue;

        if (it.size() < best_size) {
            best_size = it.size();
            is_find = true;

            if (it == begin()) ptr_occupied_block_before_best_free_block = nullptr;
            else {
                --it;
                ptr_occupied_block_before_best_free_block = *it;
                ++it;
            }
        }
    }

    if (is_find == false) return nullptr;

    void* ptr_to_metadata_occupied_block_before_best_free_block = ptr_occupied_block_before_best_free_block;
    if (ptr_occupied_block_before_best_free_block != nullptr) {
        ptr_to_metadata_occupied_block_before_best_free_block = static_cast<char*>(ptr_occupied_block_before_best_free_block) - occupied_block_metadata_size;
    }

    void* ptr_to_new_occupied_block = _link_new_occupied_block(ptr_to_metadata_occupied_block_before_best_free_block, need_size);
    return static_cast<char*>(ptr_to_new_occupied_block) + occupied_block_metadata_size;
}

void *allocator_boundary_tags::allocate_worst_fit(const size_t useful_size) {
    const size_t need_size = useful_size + occupied_block_metadata_size;

    const auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    size_t worst_size = 0; // меньше этого значения не выделить
    void* ptr_occupied_block_before_worst_free_block = nullptr; // причем если свободный блок перед всеми занятыми, то nullptr
    bool is_find = false;

    for (auto it = begin(); it != end(); ++it) {
        if (it.occupied() == true || it.size() < need_size) continue;

        if (it.size() > worst_size) {
            worst_size = it.size();
            is_find = true;

            if (it == begin()) ptr_occupied_block_before_worst_free_block = nullptr;
            else {
                --it;
                ptr_occupied_block_before_worst_free_block = *it;
                ++it;
            }
        }
    }

    if (is_find == false) return nullptr;

    void* ptr_to_metadata_occupied_block_before_worst_free_block = ptr_occupied_block_before_worst_free_block;
    if (ptr_occupied_block_before_worst_free_block != nullptr) {
        ptr_to_metadata_occupied_block_before_worst_free_block = static_cast<char*>(ptr_occupied_block_before_worst_free_block) - occupied_block_metadata_size;
    }

    void* ptr_to_new_occupied_block = _link_new_occupied_block(ptr_to_metadata_occupied_block_before_worst_free_block, need_size);
    return static_cast<char*>(ptr_to_new_occupied_block) + occupied_block_metadata_size;
}

void* allocator_boundary_tags::boundary_iterator::_get_ptr_to_end_allocator() const {
    auto* metadata_allocator = static_cast<class allocator_metadata*>(_trusted_memory);
    return static_cast<char*>(_trusted_memory) + metadata_allocator->size;
}

void* allocator_boundary_tags::boundary_iterator::_get_ptr_to_begin_allocator() const {
    return static_cast<char*>(_trusted_memory) + allocator_metadata_size;
}

void* allocator_boundary_tags::_link_new_occupied_block(void* occupied_ptr, const size_t size) {
    // переопределить старые связи между блоками
    // принимает указатель на занятый блок! возвращает именно указатель на метаданные нового занятого блока
    // if occupied_ptr = nullptr, то тогда это свободный блок сразу после метаданных аллокатора

    occupied_block_metadata* metadata_new_occupied_block = nullptr;
    if (occupied_ptr != nullptr) {
        auto* metadata_old_occupied_block = static_cast<occupied_block_metadata*>(occupied_ptr);

        void* ptr_to_metadata_new_occupied_block = static_cast<char*>(occupied_ptr) + metadata_old_occupied_block->size;
        metadata_new_occupied_block = static_cast<occupied_block_metadata*>(ptr_to_metadata_new_occupied_block) ;

        occupied_block_metadata* metadata_next_occupied_block = nullptr;
        if (metadata_old_occupied_block->next_occupied_ptr != nullptr) {
            metadata_next_occupied_block = static_cast<occupied_block_metadata*>(metadata_old_occupied_block->next_occupied_ptr);
        }

        metadata_new_occupied_block->next_occupied_ptr = metadata_next_occupied_block;
        metadata_old_occupied_block->next_occupied_ptr = metadata_new_occupied_block;

        metadata_new_occupied_block->back_occupied_ptr = metadata_old_occupied_block;
        if (metadata_next_occupied_block != nullptr) {
            metadata_next_occupied_block->back_occupied_ptr = metadata_new_occupied_block;
        }
    }
    else {
        void* ptr_to_metadata_new_occupied_block = static_cast<char*>(_trusted_memory) + allocator_metadata_size;
        metadata_new_occupied_block = static_cast<occupied_block_metadata*>(ptr_to_metadata_new_occupied_block);

        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

        metadata_new_occupied_block->next_occupied_ptr = metadata_allocator->first_occupied_block;
        metadata_new_occupied_block->back_occupied_ptr = nullptr;

        metadata_allocator->first_occupied_block = metadata_new_occupied_block;
    }

    metadata_new_occupied_block->size = size;
    metadata_new_occupied_block->trusted_memory = _trusted_memory;

    return metadata_new_occupied_block;
}