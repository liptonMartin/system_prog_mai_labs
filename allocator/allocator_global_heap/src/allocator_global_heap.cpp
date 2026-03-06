#include <not_implemented.h>
#include "../include/allocator_global_heap.h"

allocator_global_heap::allocator_global_heap() = default;

[[nodiscard]] void *allocator_global_heap::do_allocate_sm(size_t size)
{
    const size_t total_size = size_t_size + size;

    void *ptr = ::operator new(total_size);

    if (ptr != nullptr) {
        *static_cast<size_t*>(ptr) = size;
        return static_cast<size_t*>(ptr) + 1;
    }
    return nullptr;
}

void allocator_global_heap::do_deallocate_sm(void *at)
{
    if (at == nullptr) return;

    void* ptr_with_size = static_cast<size_t*>(at) - 1;

    ::operator delete(ptr_with_size);
}

allocator_global_heap::~allocator_global_heap() = default;

allocator_global_heap::allocator_global_heap(const allocator_global_heap &other) = default;

allocator_global_heap &allocator_global_heap::operator=(const allocator_global_heap &other) = default;

bool allocator_global_heap::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}

allocator_global_heap::allocator_global_heap(allocator_global_heap &&other) noexcept = default;

allocator_global_heap &allocator_global_heap::operator=(allocator_global_heap &&other) noexcept = default;
