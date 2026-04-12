//
// Created by rvova on 05.04.2026.
//

#include "include/b_tree.h"

int main() {

    B_tree<int, int, std::less<>, 3> tree;

    tree.emplace(1, 1);
    tree.emplace(2, 2);
    tree.emplace(3, 3);
    tree.emplace(4, 4);
    tree.emplace(5, 5);

    tree.emplace(10, 10);
    tree.emplace(15, 15);
    tree.emplace(14, 14);
    tree.emplace(8, 8);
    tree.emplace(9, 9);

    tree.emplace(6, 6);
    tree.emplace(7, 7);
    tree.emplace(20, 20);
    tree.emplace(18, 18);
    tree.emplace(19, 19);

    // tree.erase(tree.find(4));
    //
    // tree.print_tree();
    //
    //
    auto const_iterator = tree.cbegin();
    while (const_iterator != tree.cend()) {
        std::cout << "key: " << const_iterator->first << " value: " << const_iterator->second <<  "\n";
        ++const_iterator;
    }

    return 0;
}