#include <iostream>
#include "allocator_boundary_tags.h"

int main() {
    allocator_boundary_tags allocator = allocator_boundary_tags(sizeof(int) * 10000);

    void* result_1 = allocator.allocate(sizeof(int) * 10);
    void* result_2 = allocator.allocate(sizeof(int) * 10);
    void* result_3 = allocator.allocate(sizeof(int) * 10);

    allocator.deallocate(result_1, sizeof(int) * 10);



    return 0;
}