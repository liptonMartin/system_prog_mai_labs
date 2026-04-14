//
// Created by rvova on 12.04.2026.
//
#include "include/b_star_tree.h"
#include <set>

int main() {
    BS_tree<int, int, std::less<>, 2> tree;

    srand(10);
    int min = 1;
    int max = 100;

    std::set<int> added_numbers;

    for (int i = 0; i < 60; ++i) {
        auto number = min + rand() % (max - min + 1);
        std::cout << "Add a number: " << ' ' << number << '\n';
        added_numbers.insert(number);
        tree.emplace(number, number);
    }
    std::cout << "Added numbers (" << added_numbers.size() << "): ";
    for (auto item : added_numbers) std::cout << item << ' ';
    std::cout << '\n';
    tree.print_tree();

    return 0;
}