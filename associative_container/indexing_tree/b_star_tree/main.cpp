//
// Created by rvova on 12.04.2026.
//
#include "include/b_star_tree.h"
#include <set>

int main() {
    BS_tree<int, int, std::less<>, 2> tree;

    srand(10);
    int min = 1;
    int max = 500;
    std::set<int> added_numbers;

    for (int i = 0; i < 50; ++i) {
        auto number = min + rand() % (max - min + 1);
        std::cout << "Add a number: " << ' ' << number << '\n';
        added_numbers.insert(number);
        tree.emplace(number, number);
    }
    std::cout << "Added numbers (" << added_numbers.size() << "): ";
    for (auto item : added_numbers) std::cout << item << ' ';
    std::cout << '\n';
    std::cout << "Tree size: " << tree.size() << '\n';

    tree.print_tree();

    std::cout << "Process deleting keys...\n";
    int count = 0;
    for (auto item : added_numbers) {
        std::cout << count << ". " << item << '\n';
        tree.erase(item);
        ++count;
    }

    std::cout << "Count remaining from set: " << added_numbers.size() - count << '\n';
    std::cout << "Count remaining from tree: " << tree.size() << '\n' ;




    return 0;
}