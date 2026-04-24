//
// Created by rvova on 22.04.2026.
//
#include "include/b_star_plus_tree.h"
#include <set>

bool test_search_tree(BSP_tree<int, int, std::less<int>, 2> &tree, std::set<int> &added_numbers) {
    for (auto& item : added_numbers) {
        std::cout << "Test search " << item << '\n';
        auto result = tree.find(item);

        if (result == tree.end() || result->first != item) return false;
    }

    return true;

}

int main() {
    BSP_tree<int, int, std::less<int>, 2> tree;


    srand(10);
    int min = 1;
    int max = 100000;
    std::set<int> added_numbers;
    std::vector<int> vector_added_numbers;

    for (int i = 0; i < 10000; ++i) {
        auto number = min + rand() % (max - min + 1);
        std::cout << "Add a number: " << ' ' << number << '\n';
        if (!added_numbers.contains(number)) vector_added_numbers.push_back(number);
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
    for (auto item: added_numbers) std::cout << item << ' ';
    std::cout << '\n';
    std::cout << "Tree size: " << tree.size() << '\n';

    tree.print_tree();

    std::cout << "Process deleting keys...\n";
    int count = 0;
    for (auto item: vector_added_numbers) {
        std::cout << count << ". " << item << '\n';
        tree.erase(item);
        added_numbers.erase(item);
        ++count;

        if (count == 1000) {
            auto result = test_search_tree(tree, added_numbers);
            if (!result) {
                std::cout << "Failed search test!\n";
                return 1;
            }
        }
    }

    std::cout << "Count remaining from set: " << added_numbers.size() << '\n';
    std::cout << "Count remaining from tree: " << tree.size() << '\n';

    return 0;
}
