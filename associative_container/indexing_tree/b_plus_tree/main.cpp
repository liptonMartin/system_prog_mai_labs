//
// Created by rvova on 19.04.2026.
//

#include "./include/b_plus_tree.h"


int main() {
    /* t = 2 => keys.size = 1...3 */
    BP_tree<int, int, std::less<int>, 2> tree;


    tree.emplace(10, 10);
    tree.emplace(5, 5);
    tree.emplace(100, 100);
    tree.emplace(45, 45);
    tree.emplace(34, 34);
    // tree.emplace(90, 90);
    // tree.emplace(23, 23);
    // tree.emplace(46, 46);



    tree.print_tree();
}
