//
// Created by rvova on 19.04.2026.
//

#include "./include/b_plus_tree.h"
#include <set>


int main() {
    /* t = 2 => keys.size = 1...3 */
    BP_tree<int, int, std::less<int>, 2> tree;

    srand(10);
    int min = 1;
    int max = 5000;
    std::set<int> added_numbers;
    std::vector<int> vector_added_numbers;

    for (int i = 0; i < 5000; ++i) {
        auto number = min + rand() % (max - min + 1);
        std::cout << "Add a number: " << ' ' << number << '\n';
        if (!added_numbers.contains(number) ) vector_added_numbers.push_back(number);
        added_numbers.insert(number);
        tree.emplace(number, number);
    }

    auto iterator = tree.begin();
    std::set<int> keys_tree;
    while (iterator != tree.end()) {
        if (keys_tree.contains(iterator->first)) {
            std::cout << "Error! The key " << iterator->first << " has already been found!\n";
            return 1;
        }
        keys_tree.insert(iterator->first);
        std::cout << iterator->first << ' ';
        ++iterator;
    }
    std::cout << '\n';

    std::cout << "Added numbers (" << added_numbers.size() << "):\n";
    for (auto item : added_numbers) std::cout << item << ' ';
    std::cout << '\n';
    std::cout << "Tree size: " << tree.size() << '\n';


    // tree.print_tree();


}
