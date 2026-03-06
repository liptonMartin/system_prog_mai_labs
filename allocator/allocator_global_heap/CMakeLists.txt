add_subdirectory(tests)

add_library(
        sys_prog_allctr_allctr_glbl_hp
        src/allocator_global_heap.cpp)

target_include_directories(
        sys_prog_allctr_allctr_glbl_hp
        PUBLIC
        ./include)

target_link_libraries(
        sys_prog_allctr_allctr_glbl_hp
        PUBLIC
        sys_prog_cmmn)
target_link_libraries(
        sys_prog_allctr_allctr_glbl_hp
        PUBLIC
        sys_prog_allctr_allctr)

add_executable(
        main_global_heap
        main.cpp
)

target_include_directories(
        main_global_heap
        PRIVATE
        ./include
)

target_link_libraries(
        main_global_heap
        PRIVATE
        sys_prog_allctr_allctr_glbl_hp
)