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

    void* memory = parent_allocator->allocate(space_size);

    auto* metadata = static_cast<allocator_metadata*>(memory);
    metadata->size = space_size;
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

    switch (metadata->allocate_fit_mode) {
        case (fit_mode::first_fit) : {
            return allocate_first_fit(size);
        }

        case (fit_mode::the_best_fit) : {
            return allocate_best_fit(size);
        }

        case (fit_mode::the_worst_fit) : {
            return allocate_worst_fit(size);
        }
    }

    return nullptr;
}

void allocator_boundary_tags::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;

    auto* metadata_deleted_block = static_cast<occupied_block_metadata*>(at);
    if (metadata_deleted_block->trusted_memory != _trusted_memory) return;

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    std::lock_guard<std::mutex> lock(metadata_allocator->mutex);

    auto* metadata_prev_block = static_cast<occupied_block_metadata*>(metadata_deleted_block->back_occupied_ptr);
    auto* metadata_next_block = static_cast<occupied_block_metadata*>(metadata_deleted_block->next_occupied_ptr);

    metadata_prev_block->next_occupied_ptr = metadata_next_block;
    metadata_next_block->back_occupied_ptr = metadata_prev_block;
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
    const occupied_block_metadata* block_metadata = static_cast<occupied_block_metadata*>(_occupied_ptr);
    if (_occupied) _occupied = false;

    else {
        _occupied = true;
        _occupied_ptr = block_metadata->next_occupied_ptr;
    }

    return *this;
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    const occupied_block_metadata* block_metadata = static_cast<occupied_block_metadata*>(_occupied_ptr);
    if (_occupied) {
        _occupied = false;
        _occupied_ptr = block_metadata->back_occupied_ptr;
    }
    else  _occupied = false;

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
                // TODO: проверить (последний свободный блок)
                auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
                next_occupied_block_ptr = static_cast<char*>(_trusted_memory) + metadata_allocator->size;
            }
            return static_cast<char*>(next_occupied_block_ptr) - static_cast<char*>(free_block_ptr);
        }
    }
    return 0;
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
        _occupied_ptr = static_cast<char*>(trusted) + allocator_metadata_size;
        if (metadata->first_occupied_block == nullptr) _occupied = true;
    }
}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void* occupied_ptr, void* trusted) :
    _occupied_ptr(occupied_ptr),
    _occupied(false),
    _trusted_memory(trusted) {}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    if (_occupied_ptr == nullptr) return nullptr;

    occupied_block_metadata* block_metadata = static_cast<occupied_block_metadata*>(_occupied_ptr);

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
        if (it.occupied() == true && it.size() < need_size) {
            continue;
        }
        auto* metadata_new_occupied_block = static_cast<occupied_block_metadata*>(*it);

        --it;

        if (it != nullptr) {
            auto* metadata_old_occupied_block = static_cast<occupied_block_metadata*>(*it);

            auto* metadata_next_occupied_block = static_cast<occupied_block_metadata*>(metadata_new_occupied_block->next_occupied_ptr);

            metadata_new_occupied_block->next_occupied_ptr = metadata_next_occupied_block;
            metadata_old_occupied_block->next_occupied_ptr = metadata_new_occupied_block;

            metadata_new_occupied_block->back_occupied_ptr = metadata_old_occupied_block;
            metadata_next_occupied_block->back_occupied_ptr = metadata_new_occupied_block;
        }

        metadata_new_occupied_block->size = useful_size;
        metadata_new_occupied_block->trusted_memory = _trusted_memory;

        return static_cast<char*>(*it) + occupied_block_metadata_size;
    }

    return nullptr;
}

void *allocator_boundary_tags::allocate_best_fit(const size_t useful_size) {
    return nullptr;
}


void *allocator_boundary_tags::allocate_worst_fit(const size_t useful_size) {
    return nullptr;
}
