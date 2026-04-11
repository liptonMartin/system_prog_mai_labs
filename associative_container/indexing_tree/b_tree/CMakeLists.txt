add_subdirectory(tests)

add_library(
        sys_prog_assctv_cntnr_indxng_tr_b_tr
        include/b_tree.h
        src/hhh.cpp)

target_include_directories(
        sys_prog_assctv_cntnr_indxng_tr_b_tr
        PUBLIC
        ./include)

target_link_libraries(
        sys_prog_assctv_cntnr_indxng_tr_b_tr
        PUBLIC
        sys_prog_assctv_cntnr)


target_link_libraries(
        sys_prog_assctv_cntnr_indxng_tr_b_tr
        PUBLIC
        ${BOOST_LIBRARIES})
target_include_directories(
        sys_prog_assctv_cntnr_indxng_tr_b_tr
        PUBLIC
        ${Boost_INCLUDE_DIRS})
target_link_directories(
        sys_prog_assctv_cntnr_indxng_tr_b_tr
        PUBLIC
        ${Boost_LIBRARY_DIR}
)

add_executable(
        b_tree_main main.cpp
)

target_link_libraries(
        b_tree_main
        PRIVATE
        sys_prog_assctv_cntnr_indxng_tr_b_tr
)