#include <not_implemented.h>
#include "../include/allocator_sorted_list.h"

allocator_sorted_list::~allocator_sorted_list()
{
    auto* metadata = static_cast<allocator_metadata*>(_trusted_memory);
    metadata->parent_allocator->deallocate(_trusted_memory, metadata->size);
}

allocator_sorted_list::allocator_sorted_list(
    allocator_sorted_list &&other) noexcept
{
    _trusted_memory = other._trusted_memory;
    other._trusted_memory = nullptr;
}

allocator_sorted_list &allocator_sorted_list::operator=(
    allocator_sorted_list &&other) noexcept
{
    if (this != &other) {
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }
    return *this;
}

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode) : _trusted_memory(nullptr) {
    if (parent_allocator == nullptr) parent_allocator = std::pmr::get_default_resource();

    size_t need_size = allocator_metadata_size + space_size;
    void *memory = parent_allocator->allocate(need_size);

    auto *metadata_allocator = static_cast<allocator_metadata *>(memory);
    metadata_allocator->parent_allocator = parent_allocator;
    metadata_allocator->size = need_size;
    metadata_allocator->fit_mode = allocate_fit_mode;
    metadata_allocator->ptr_to_first_free_block = static_cast<char *>(memory) + allocator_metadata_size;
    new(&metadata_allocator->mutex) std::mutex();

    // инициализация первого свободного блока
    auto* metadata_first_free_block = static_cast<block_metadata*>(metadata_allocator->ptr_to_first_free_block);
    metadata_first_free_block->ptr = nullptr;
    metadata_first_free_block->size = space_size;

    _trusted_memory = metadata_allocator;
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(
    size_t size)
{
    auto* metadata_allocator = static_cast<allocator_metadata *>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata_allocator->mutex);

    void* result = nullptr;
    switch (metadata_allocator->fit_mode) {
        case (fit_mode::first_fit) : {
            result = _allocate_first_fit(size);
            break;
        }
        case (fit_mode::the_best_fit) : {
            result = _allocate_best_fit(size);
            break;
        }

        case (fit_mode::the_worst_fit) : {
            result = _allocate_worst_fit(size);
            break;
        }
    }

    if (result == nullptr) throw std::bad_alloc();

    return static_cast<char*>(result) + block_metadata_size;
}

allocator_sorted_list::allocator_sorted_list(const allocator_sorted_list &other) :
    _trusted_memory(other._trusted_memory) {}

allocator_sorted_list &allocator_sorted_list::operator=(const allocator_sorted_list &other)
{
    if (this != &other) _trusted_memory = other._trusted_memory;
    return *this;
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    auto other_allocator = dynamic_cast<const allocator_sorted_list*>(&other);
    if (other_allocator != nullptr) {
        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
        auto* metadata_other_allocator = static_cast<allocator_metadata*>(other_allocator->_trusted_memory);
        return _trusted_memory == other_allocator->_trusted_memory &&
        metadata_allocator->parent_allocator == metadata_other_allocator->parent_allocator &&
            metadata_allocator->size == metadata_other_allocator->size &&
                metadata_allocator->fit_mode == metadata_other_allocator->fit_mode &&
                    metadata_allocator->ptr_to_first_free_block == metadata_other_allocator->ptr_to_first_free_block;
    }
    return false;
}

void allocator_sorted_list::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;

    void* ptr_to_metadata_deleted_block = static_cast<char*>(at) - block_metadata_size;
    auto* metadata_deleted_block = static_cast<block_metadata*>(ptr_to_metadata_deleted_block);
    if (metadata_deleted_block->ptr != _trusted_memory) return;

    void* ptr_to_prev_free_block = nullptr;
    for (auto it = begin(); it != end(); ++it) {
        if (at != *it) {
            if (it.occupied() == false) {
                ptr_to_prev_free_block = *it;
            }
            continue;
        }

        auto* metadata_prev_free_block = static_cast<block_metadata*>(ptr_to_prev_free_block);

        ++it;
        if (it.occupied()) {
            // если за удаляемым идет занятый, просто переопределить связи
            metadata_deleted_block->ptr = metadata_prev_free_block == nullptr ? nullptr : metadata_prev_free_block->ptr;
            if (metadata_prev_free_block != nullptr) metadata_prev_free_block->ptr = metadata_deleted_block;

            // размер остается тот же
        }
        else {
            // если за удаляемым идет свободный, то надо их объединить

            void* ptr_to_metadata_next_free_block = static_cast<char*>(*it) - block_metadata_size;
            auto* metadata_next_free_block = static_cast<block_metadata*>(ptr_to_metadata_next_free_block);

            if (metadata_prev_free_block != nullptr) metadata_prev_free_block->ptr = metadata_deleted_block;
            metadata_deleted_block->ptr = metadata_next_free_block->ptr;

            // размер это их сумма
            metadata_deleted_block->size += metadata_next_free_block->size;
        }

        if (metadata_prev_free_block == nullptr) {
            auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
            metadata_allocator->ptr_to_first_free_block = metadata_deleted_block;
        }

        return;

    }
}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    metadata_allocator->fit_mode=mode;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    std::lock_guard<std::mutex> lock(metadata_allocator->mutex);

    return get_blocks_info_inner();
}


std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> result;
    for (auto it = begin(); it != end(); ++it) {
        block_info block;
        block.block_size = it.size();
        block.is_block_occupied = it.occupied();

        result.push_back(block);
    }

    return result;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_begin() const noexcept
{
    return sorted_free_iterator(_trusted_memory);
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::free_end() const noexcept
{
    return sorted_free_iterator();
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::begin() const noexcept
{
    return sorted_iterator(_trusted_memory);
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::end() const noexcept
{
    return sorted_iterator(nullptr, nullptr, _trusted_memory);
}

bool allocator_sorted_list::sorted_free_iterator::operator==(
        const allocator_sorted_list::sorted_free_iterator & other) const noexcept
{
    return _free_ptr == other._free_ptr;
}

bool allocator_sorted_list::sorted_free_iterator::operator!=(
        const allocator_sorted_list::sorted_free_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_free_iterator &allocator_sorted_list::sorted_free_iterator::operator++() & noexcept
{
    if (_free_ptr == nullptr) return *this;

    _free_ptr = static_cast<block_metadata*>(_free_ptr)->ptr;
    return *this;
}

allocator_sorted_list::sorted_free_iterator allocator_sorted_list::sorted_free_iterator::operator++(int n)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

size_t allocator_sorted_list::sorted_free_iterator::size() const noexcept
{
    // возвращаю именно сколько свободно или занято, без учета метаданных!
    if (_free_ptr == nullptr) return 0;
    return static_cast<block_metadata*>(_free_ptr)->size - block_metadata_size;
}

void *allocator_sorted_list::sorted_free_iterator::operator*() const noexcept
{
    if (_free_ptr == nullptr) return nullptr;

    return static_cast<char*>(_free_ptr) + block_metadata_size;
}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator()
    : _free_ptr(nullptr) {}

allocator_sorted_list::sorted_free_iterator::sorted_free_iterator(void *trusted)
{
    auto* metadata_allocator = static_cast<allocator_metadata*>(trusted);
    _free_ptr = metadata_allocator->ptr_to_first_free_block;
}

bool allocator_sorted_list::sorted_iterator::operator==(const allocator_sorted_list::sorted_iterator & other) const noexcept
{
    return _free_ptr == other._free_ptr && _current_ptr == other._current_ptr && _trusted_memory == other._trusted_memory;
}

bool allocator_sorted_list::sorted_iterator::operator!=(const allocator_sorted_list::sorted_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_sorted_list::sorted_iterator &allocator_sorted_list::sorted_iterator::operator++() & noexcept
{
    if (_current_ptr == nullptr) return *this;

    auto* metadata_block = static_cast<block_metadata*>(_current_ptr);
    if (_current_ptr == _free_ptr) {
        // мы находимся на свободном => надо обновить указатель на свободный блок
        _free_ptr = metadata_block->ptr;
    }
    _current_ptr = static_cast<char*>(_current_ptr) + metadata_block->size;

    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
    void* end_of_all_memory = static_cast<char*>(_trusted_memory) + metadata_allocator->size;
    if (_current_ptr >= end_of_all_memory) _current_ptr = nullptr; // дошли до конца

    return *this;
}

allocator_sorted_list::sorted_iterator allocator_sorted_list::sorted_iterator::operator++(int n)
{
    auto iterator = *this;
    ++(*this);
    return iterator;
}

size_t allocator_sorted_list::sorted_iterator::size() const noexcept
{
    // возвращает именно сколько свободно (занято), без учета метаданных!
    if (_current_ptr == nullptr) return 0;
    return static_cast<block_metadata*>(_current_ptr)->size - block_metadata_size;
}

void *allocator_sorted_list::sorted_iterator::operator*() const noexcept
{
    if (_current_ptr == nullptr) return nullptr;

    return static_cast<char*>(_current_ptr) + block_metadata_size;
}

allocator_sorted_list::sorted_iterator::sorted_iterator()
    : _current_ptr(nullptr), _free_ptr(nullptr), _trusted_memory(nullptr) {}

allocator_sorted_list::sorted_iterator::sorted_iterator(void *trusted) {
    auto* metadata_allocator = static_cast<allocator_metadata*>(trusted);

    _current_ptr = static_cast<char*>(trusted) + allocator_metadata_size;
    _free_ptr = metadata_allocator->ptr_to_first_free_block;
    _trusted_memory = trusted;
}

allocator_sorted_list::sorted_iterator::sorted_iterator(void* current_ptr, void* free_ptr, void* trusted_memory)
    : _current_ptr(current_ptr), _free_ptr(free_ptr), _trusted_memory(trusted_memory) {}

bool allocator_sorted_list::sorted_iterator::occupied() const noexcept
{
    return _free_ptr != _current_ptr;
}

void *allocator_sorted_list::_allocate_first_fit(const size_t useful_size) {
    // возвращает указатель на новые метаданные
    void* prev_free_block = nullptr;
    for (auto it = free_begin(); it != free_end(); ++it) {
        if (it.size() < useful_size) {
            prev_free_block = *it;
            continue;
        }

        return _link_new_block(*it, prev_free_block, useful_size);
    }

    return nullptr;
}

void *allocator_sorted_list::_allocate_best_fit(const size_t useful_size) {
    // возвращает указатель на новые метаданные
    auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);

    void* prev_free_block = nullptr;
    void* best_free_block = nullptr;
    size_t best_free_block_size = metadata_allocator->size; // больше этого значения все равно не выделить

    for (auto it = free_begin(); it != free_end(); ++it) {
        if (it.size() < useful_size) {
            prev_free_block = *it;
            continue;
        }

        if (it.size() < best_free_block_size) {
            best_free_block = *it;
            best_free_block_size = it.size();
        }
    }

    if (best_free_block == nullptr) return nullptr;
    return _link_new_block(best_free_block, prev_free_block, useful_size);
}

void *allocator_sorted_list::_allocate_worst_fit(const size_t useful_size) {
    // возвращает указатель на новые метаданные

    void* prev_free_block = nullptr;
    void* worst_free_block = nullptr;
    size_t worst_free_block_size = 0;

    for (auto it = free_begin(); it != free_end(); ++it) {
        if (it.size() < useful_size) {
            prev_free_block = *it;
            continue;
        }

        if (it.size() > worst_free_block_size) {
            worst_free_block = *it;
            worst_free_block_size = it.size();
        }
    }

    if (worst_free_block == nullptr) return nullptr;
    return _link_new_block(worst_free_block, prev_free_block, useful_size);

}

void *allocator_sorted_list::_link_new_block(void* ptr_to_new_block, void* prev_free_block, const size_t useful_size) {
    // возвращает указатель на метаданные
    void* ptr_to_metadata_block = static_cast<char*>(ptr_to_new_block) - block_metadata_size;
    auto* metadata_block = static_cast<block_metadata*>(ptr_to_metadata_block);

    auto* metadata_prev_free_block = static_cast<block_metadata*>(prev_free_block);

    if (metadata_prev_free_block != nullptr) {
        metadata_prev_free_block->ptr = metadata_block->ptr;
    }
    else {
        // тогда свободный блок куда мы вставили, был первым во всем аллокаторе
        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
        metadata_allocator->ptr_to_first_free_block = metadata_block->ptr;
    }

    size_t old_size = metadata_block->size;
    size_t new_free_size = old_size - (useful_size + block_metadata_size); /* так как old_size - всего памяти (вместе с метаданными),
                                                                            *  а useful_size это сколько новой памяти (без метаданными) */
    size_t new_size = useful_size + block_metadata_size;
    if (new_free_size <= block_metadata_size) {
        new_size += new_free_size;
    }

    if (metadata_block->ptr == nullptr) {
        /* этот свободный блок был последним
         * то есть там либо не осталось вообще памяти
         * либо нужно ее фрагментировать */

        auto* metadata_allocator = static_cast<allocator_metadata*>(_trusted_memory);
        char* end_of_all_memory = static_cast<char*>(_trusted_memory) + metadata_allocator->size;

        char* end_of_new_occupied_block = static_cast<char*>(ptr_to_metadata_block) + new_size;
        if (end_of_new_occupied_block < end_of_all_memory) {
            // нужно фрагментировать
            void* ptr_to_new_next_free_block = static_cast<char*>(ptr_to_metadata_block) + new_size;
            auto* metadata_new_next_free_block = static_cast<block_metadata*>(ptr_to_new_next_free_block);

            metadata_new_next_free_block->ptr = nullptr; // потому что дальше нет свободных блоков
            metadata_new_next_free_block->size = end_of_all_memory - end_of_new_occupied_block;

            if (metadata_prev_free_block != nullptr) {
                metadata_prev_free_block->ptr = metadata_new_next_free_block;
            }
            else {
                metadata_allocator->ptr_to_first_free_block = metadata_new_next_free_block;
            }
        }
    }

    metadata_block->ptr = _trusted_memory;
    metadata_block->size = new_size;

    return ptr_to_metadata_block;
}