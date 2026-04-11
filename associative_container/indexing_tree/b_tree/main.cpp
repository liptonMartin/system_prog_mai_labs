//
// Created by rvova on 05.04.2026.
//

#include "include/b_tree.h"

int main() {

    B_tree<int, int, std::less<>, 2> tree;

    B_tree<int, int, std::less<>, 2>::tree_data_type data {1, 1};
    B_tree<int, int, std::less<>, 2>::tree_data_type data_2 {2, 1};
    B_tree<int, int, std::less<>, 2>::tree_data_type data_3 {3, 1};
    B_tree<int, int, std::less<>, 2>::tree_data_type data_4 {4, 1};
    B_tree<int, int, std::less<>, 2>::tree_data_type data_5 {5, 1};

    tree.insert(std::move(data));
    tree.insert(std::move(data_2));

    // tree.print_tree();

    tree.insert(std::move(data_3));
    tree.insert(std::move(data_4));
    tree.insert(std::move(data_5));

    tree.emplace(-1, 1);
    tree.emplace(10, 1);
    tree.emplace(15, 1);
    tree.emplace(6, 1);

    tree.emplace(7, 1);

    tree.emplace(9, 1);

    tree.emplace(0, 1);



    return 0;
}