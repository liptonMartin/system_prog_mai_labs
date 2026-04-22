//
// Created by rvova on 22.04.2026.
//
#include "include/b_star_plus_tree.h"

int main() {
    BSP_tree<int, int, std::less<int>, 2> tree;
    tree.emplace(1, 1);

    tree.print_tree();

    return 0;
}
