//
// Created by rvova on 12.04.2026.
//
#include "include/b_star_tree.h"

int main() {
    BS_tree<int, int, std::less<>, 2> tree;

    tree.emplace(1, 1);

    return 0;
}