#include <iostream>
#include "allocator_boundary_tags.h"

int main() {
    allocator_boundary_tags allocator = allocator_boundary_tags(100);

    void* result = allocator.allocate(10);

    if (result == nullptr) std::cout << "Что то пошло не так!\n";

    int* int_result = static_cast<int*>(result);

    *int_result = 10;

    std::cout << *int_result << std::endl;

    allocator.deallocate(int_result, 10);


    return 0;
}