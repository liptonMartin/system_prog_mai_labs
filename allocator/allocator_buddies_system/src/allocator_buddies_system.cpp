#include <not_implemented.h>
#include <cstddef>
#include "../include/allocator_buddies_system.h"

allocator_buddies_system::~allocator_buddies_system()
{
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    size_t size = (1 << metadata_allocator->k) + allocator_metadata_size;
    metadata_allocator->parent_allocator->deallocate(_trusted_memory, size);
    _trusted_memory = nullptr;
}

allocator_buddies_system::allocator_buddies_system(
    allocator_buddies_system &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_buddies_system &allocator_buddies_system::operator=(
    allocator_buddies_system &&other) noexcept
{
    if (*this != other) {
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }

    return *this;
}

allocator_buddies_system::allocator_buddies_system(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    size_t min_size = 1 << min_k;
    if (space_size < min_size) throw std::logic_error("Space size too small");

    size_t space_k = __detail::nearest_greater_k_of_2(space_size);
    size_t space_size_of_2 = 1 << space_k; // точная степень двойки

    if (parent_allocator == nullptr) parent_allocator = std::pmr::get_default_resource();

    size_t need_size = space_size_of_2 + allocator_metadata_size;
    void* memory = parent_allocator->allocate(need_size);

    _trusted_memory = memory;

    auto* metadata_allocator = static_cast<allocator_metadata*>(memory);

    metadata_allocator->parent_allocator = parent_allocator;
    metadata_allocator->fit_mode = allocate_fit_mode;
    metadata_allocator->k = space_k;
    new (&metadata_allocator->mutex) std::mutex();

    // инициализировать первый свободный блок

    void* ptr_to_first_free_block = static_cast<char*>(_trusted_memory) + allocator_metadata_size;
    auto* metadata_first_free_block = static_cast<common_block_metadata*>(ptr_to_first_free_block);

    metadata_first_free_block->occupied = false;
    metadata_first_free_block->size = space_k;
}

[[nodiscard]] void *allocator_buddies_system::do_allocate_sm(size_t size)
{
    if (_trusted_memory == nullptr) return nullptr;

    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata->mutex);

    void* result = nullptr;

    switch (metadata->fit_mode) {
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

void allocator_buddies_system::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;

    void* ptr_to_metadata_occupied_block = static_cast<char*>(at) - occupied_block_metadata_size;
    auto* metadata_occupied_block = static_cast<occupied_block_metadata*>(ptr_to_metadata_occupied_block);
    if (metadata_occupied_block->trusted_memory != _trusted_memory) return;

    metadata_occupied_block->metadata.occupied = false;
    _join_free_blocks(ptr_to_metadata_occupied_block);
}

allocator_buddies_system::allocator_buddies_system(const allocator_buddies_system &other)
{
    _trusted_memory = other._trusted_memory;
}

allocator_buddies_system &allocator_buddies_system::operator=(const allocator_buddies_system &other)
{
    if (*this != other) {
        _trusted_memory = other._trusted_memory;
    }
    return *this;
}

bool allocator_buddies_system::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto* allocator = dynamic_cast<const allocator_buddies_system*>(&other);
    if (allocator != nullptr) {
        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
        auto* metadata_other_allocator = static_cast<allocator_metadata*>(allocator->_trusted_memory);

        return metadata_allocator->parent_allocator == metadata_other_allocator->parent_allocator &&
            metadata_allocator->fit_mode == metadata_other_allocator->fit_mode &&
                metadata_allocator->k == metadata_other_allocator->k;

    }
    return false;
}

inline void allocator_buddies_system::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    metadata_allocator->fit_mode = mode;
}


std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata_allocator->mutex);

    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info_inner() const
{
    auto blocks = std::vector<allocator_test_utils::block_info>();
    for (auto it = begin(); it != end(); ++it) {
        block_info block;
        block.block_size = it.size();
        block.is_block_occupied = it.occupied();

        blocks.push_back(block);
    }
    return blocks;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::begin() const noexcept
{
    void* start = static_cast<char*>(_trusted_memory) + allocator_metadata_size;
    return allocator_buddies_system::buddy_iterator(start);
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::end() const noexcept
{
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    void* end_of_all_memory = static_cast<char*>(_trusted_memory) + (1 << metadata_allocator->k) + allocator_metadata_size;
    return allocator_buddies_system::buddy_iterator(end_of_all_memory);
}

bool allocator_buddies_system::buddy_iterator::operator==(const allocator_buddies_system::buddy_iterator &other) const noexcept
{
    return _block == other._block;
}

bool allocator_buddies_system::buddy_iterator::operator!=(const allocator_buddies_system::buddy_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_buddies_system::buddy_iterator &allocator_buddies_system::buddy_iterator::operator++() & noexcept
{
    auto* metadata_block = static_cast<common_block_metadata*>(_block);
    size_t current_size = 1 << metadata_block->size;

    _block = static_cast<char*>(_block) + current_size;

    return *this;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::buddy_iterator::operator++(int n)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

size_t allocator_buddies_system::buddy_iterator::size() const noexcept
{
    // возвращает реальный размер, как 2^k
    auto* common_metadata_block = static_cast<common_block_metadata*>(_block);
    return (1 << common_metadata_block->size);
}

bool allocator_buddies_system::buddy_iterator::occupied() const noexcept
{
    auto* common_metadata_block = static_cast<common_block_metadata*>(_block);
    return common_metadata_block->occupied;
}

void *allocator_buddies_system::buddy_iterator::operator*() const noexcept
{
    if (occupied()) {
        return static_cast<char*>(_block) + occupied_block_metadata_size;
    }
    return static_cast<char*>(_block) + free_block_metadata_size;

}

allocator_buddies_system::buddy_iterator::buddy_iterator()
{
    _block = nullptr;
}

allocator_buddies_system::buddy_iterator::buddy_iterator(void *start)
{
    _block = start;
}

void *allocator_buddies_system::allocate_first_fit(size_t size) {
    // возвращает указатель на метаданные нового занятого блока

    size_t need_size = size + occupied_block_metadata_size;
    unsigned char need_k = __detail::nearest_greater_k_of_2(need_size);
    need_size = 1 << need_k;
    for (auto it = begin(); it != end(); ++it) {;
        if (it.occupied() || it.size() < need_size) continue;

        void* ptr_to_metadata_free_block = static_cast<char*>(*it) - free_block_metadata_size;
        auto* metadata_free_block = static_cast<common_block_metadata*>(ptr_to_metadata_free_block);

        void* ptr_to_metadata_new_occupied_block = _split_free_block(metadata_free_block, need_k);

        auto* metadata_new_occupuied_block = static_cast<occupied_block_metadata*>(ptr_to_metadata_new_occupied_block);
        metadata_new_occupuied_block->metadata.occupied = true;
        metadata_new_occupuied_block->metadata.size =  need_k;
        metadata_new_occupuied_block->trusted_memory = _trusted_memory;

        return ptr_to_metadata_new_occupied_block;
    }

    return nullptr;
}

void *allocator_buddies_system::allocate_best_fit(size_t size) {
    // возвращает указатель на метеданные нового занятого блока

    size_t need_size = size + occupied_block_metadata_size;
    unsigned char need_k = __detail::nearest_greater_k_of_2(need_size);
    need_size = 1 << need_k;

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    size_t best_size = 1 << metadata_allocator->k;
    void* ptr_to_metadata_best_free_block = nullptr;

    for (auto it = begin(); it != end(); ++it) {
        if (it.occupied() || it.size() < need_size) continue;

        if (it.size() < best_size) {
            best_size = it.size();
            ptr_to_metadata_best_free_block = static_cast<char*>(*it) - free_block_metadata_size;
        }
    }

    if (ptr_to_metadata_best_free_block == nullptr) return nullptr;
    ptr_to_metadata_best_free_block = _split_free_block(ptr_to_metadata_best_free_block, need_k);

    auto* metadata_new_occupuied_block = static_cast<occupied_block_metadata*>(ptr_to_metadata_best_free_block);
    metadata_new_occupuied_block->metadata.occupied = true;
    metadata_new_occupuied_block->metadata.size =  need_k;
    metadata_new_occupuied_block->trusted_memory = _trusted_memory;

    return ptr_to_metadata_best_free_block;

}

void *allocator_buddies_system::allocate_worst_fit(size_t size) {
    // возвращает указатель на метеданные нового занятого блока

    size_t need_size = size + occupied_block_metadata_size;
    unsigned char need_k = __detail::nearest_greater_k_of_2(need_size);
    need_size = 1 << need_k;

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    size_t worst_size = 1 << metadata_allocator->k;
    void* ptr_to_metadata_worst_free_block = nullptr;

    for (auto it = begin(); it != end(); ++it) {
        if (it.occupied() || it.size() < need_size) continue;

        if (it.size() > worst_size) {
            worst_size = it.size();
            ptr_to_metadata_worst_free_block = static_cast<char*>(*it) - free_block_metadata_size;
        }
    }

    if (ptr_to_metadata_worst_free_block == nullptr) return nullptr;
    ptr_to_metadata_worst_free_block = _split_free_block(ptr_to_metadata_worst_free_block, need_k);

    auto* metadata_new_occupuied_block = static_cast<occupied_block_metadata*>(ptr_to_metadata_worst_free_block);
    metadata_new_occupuied_block->metadata.occupied = true;
    metadata_new_occupuied_block->metadata.size =  need_k;
    metadata_new_occupuied_block->trusted_memory = _trusted_memory;

    return ptr_to_metadata_worst_free_block;
}

void *allocator_buddies_system::_split_free_block(void* ptr, unsigned int k) {
    auto* metadata_block = static_cast<common_block_metadata*>(ptr);

    if (metadata_block->size == k) return metadata_block;

    // иначе нужно разделить на два блока, меньшего размера
    // чтобы найти своего двойника -> ptr xor 2^k

    void* ptr_to_start_blocks = static_cast<char*>(_trusted_memory) + allocator_metadata_size;
    size_t offset = static_cast<char*>(ptr) - static_cast<char*>(ptr_to_start_blocks); /* смещение текущего блока
                                                                                        * относительно начала всех блоков */

    --metadata_block->size;

    size_t offset_buddy_block = offset ^ (1 << metadata_block->size);
    void* ptr_to_metadata_buddy_block = static_cast<char*>(ptr_to_start_blocks) + offset_buddy_block;
    auto* metadata_buddy_block = static_cast<common_block_metadata*>(ptr_to_metadata_buddy_block);

    metadata_buddy_block->occupied = false;
    metadata_buddy_block->size = metadata_block->size;

    return _split_free_block(metadata_block, k);
}

void allocator_buddies_system::_join_free_blocks(void *ptr) {
    // рекурсивно объединяет двойники

    auto* metadata_block = static_cast<common_block_metadata*>(ptr);
    if (metadata_block->occupied == true) return; // если текущий блок занят, то соседний нет смысла рассматривать

    void* ptr_to_start_blocks = static_cast<char*>(_trusted_memory) + allocator_metadata_size;
    size_t offset = static_cast<char*>(ptr) - static_cast<char*>(ptr_to_start_blocks); /* смещение текущего блока
                                                                                        * относительно начала всех блоков */

    size_t offset_buddy_block = offset ^ (1 << metadata_block->size);

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    if (offset_buddy_block == (1 << metadata_allocator->k)) return; // вышли за границы памяти

    void* ptr_to_metadata_buddy_block = static_cast<char*>(ptr_to_start_blocks) + offset_buddy_block;
    auto* metadata_buddy_block = static_cast<common_block_metadata*>(ptr_to_metadata_buddy_block);

    if (metadata_buddy_block->occupied == true || metadata_buddy_block->size != metadata_block->size) return; /* соседний блок занят или
                                                                                                               * у соседнего блока не тот размер */

    // нужно определить какой из них левее и туда записать новые данные и от него запустить алгос
    auto* metadata_left_block = metadata_block < metadata_buddy_block ? metadata_block : metadata_block;
    metadata_left_block->occupied = false;
    metadata_left_block->size = ++metadata_block->size;

    _join_free_blocks(metadata_left_block);
}
