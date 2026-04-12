//
// Created by rvova on 05.04.2026.
//

#include "include/b_tree.h"

int main() {

    B_tree<int, std::string, std::less<int>, 5> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    auto b = tree.upper_bound(4);
    auto e = tree.lower_bound(102);
    std::vector<decltype(tree)::value_type> actual_result(b, e);

    // B_tree<int, int, std::less<>, 3> tree;
    //
    // tree.emplace(1, 1);
    // tree.emplace(2, 2);
    // tree.emplace(3, 3);
    // tree.emplace(4, 4);
    // tree.emplace(5, 5);
    //
    // tree.emplace(10, 10);
    // tree.emplace(15, 15);
    // tree.emplace(14, 14);
    // tree.emplace(8, 8);
    // tree.emplace(9, 9);
    //
    // tree.emplace(6, 6);
    // tree.emplace(7, 7);
    // tree.emplace(20, 20);
    // tree.emplace(18, 18);
    // tree.emplace(19, 19);
    //
    // // tree.erase(tree.find(4));
    // //
    // // tree.print_tree();
    // //
    // //
    // // auto const_iterator = tree.cbegin();
    // // while (const_iterator != tree.cend()) {
    // //     std::cout << "key: " << const_iterator->first << " value: " << const_iterator->second <<  "\n";
    // //     ++const_iterator;
    // // }
    //
    // auto iter = tree.lower_bound(1);
    // auto iter_2 = tree.upper_bound(2);

    return 0;
}