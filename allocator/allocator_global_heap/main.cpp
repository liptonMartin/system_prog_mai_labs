#include <iostream>
#include "include/allocator_global_heap.h"


int main() {
    allocator_global_heap global_allocator = allocator_global_heap();

    void* ptr = global_allocator.allocate(10);
    if (ptr == nullptr) {
        throw std::exception();
    }

    int* int_ptr = static_cast<int*>(ptr);

    *int_ptr = 42;

    std::cout << *int_ptr << std::endl;

    global_allocator.deallocate(static_cast<void*>(ptr), 10);

    return 0;
}