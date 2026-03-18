#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H

#include <pp_allocator.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <mutex>

class allocator_red_black_tree final:
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode
{

private:

    enum class block_color : unsigned char
    { RED, BLACK };

    struct block_data
    {
        bool occupied : 4;
        block_color color : 4;
    };

    /* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO ALLOCATOR FIELDS ------------------------------ */
    std::pmr::memory_resource*& parent_allocator_ref(void* trusted);
    fit_mode& fit_mode_ref(void* trusted);
    size_t& size_ref(void* trusted);
    std::mutex mutex_ref(void* trusted);
    void*& root_ref(void* trusted);

    /* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO COMMON BLOCK FIELDS ------------------------------ */
    block_data& block_data_ref(void* block);
    void*& prev_ref(void* block);
    void*& next_ref(void* block);

    /* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO OCCUPIED BLOCK FIELDS ------------------------------ */
    void*& trusted_memory_ref(void* block);

    /* ------------------------------ HELPER FUNCTIONS FOR ACCESS TO FREE BLOCK FIELDS ------------------------------ */
    void*& right_ref(void* block);
    void*& left_ref(void* block);
    void*& parent_ref(void* block);

    struct allocator_metadata {
        std::pmr::memory_resource *parent_allocator;
        fit_mode mode;
        size_t size;
        std::mutex mutex;
        void* root;
    };

    struct common_block_metadata {
        block_data data;
        void* prev;
        void* next;
    };

    struct occupied_block_metadata {
        common_block_metadata common_metadata;
        void* trusted_memory;
    };

    struct free_block_metadata {
        common_block_metadata common_metadata;

        void* right;
        void* left;
        void* parent;

        [[nodiscard]] size_t get_size(void* trusted) const;

        [[nodiscard]] bool is_left_child() const;
        [[nodiscard]] bool is_right_child() const;

    };

    /* ------------------------------ HELPER FUNCTIONS FOR RED BLACK TREE ------------------------------ */
    static int compare_free_blocks(const free_block_metadata* left, const free_block_metadata* right, void* trusted);

    static bool is_red_parent(free_block_metadata* node);
    static bool is_red_right_child(free_block_metadata* node);
    static bool is_red_left_child(free_block_metadata* node);


    void *_trusted_memory;

    static constexpr const size_t allocator_metadata_size = sizeof(allocator_metadata);
    static constexpr const size_t occupied_block_metadata_size = sizeof(occupied_block_metadata);
    static constexpr const size_t free_block_metadata_size = sizeof(free_block_metadata);

public:
    
    ~allocator_red_black_tree() override;
    
    allocator_red_black_tree(
        allocator_red_black_tree const &other);
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree const &other);
    
    allocator_red_black_tree(
        allocator_red_black_tree &&other) noexcept;
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree &&other) noexcept;

public:
    
    explicit allocator_red_black_tree(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

    // для дебага (inorder обход дерева)
    std::vector<std::pair<free_block_metadata, size_t>> free_blocks();
    void free_blocks(free_block_metadata* current_node, std::vector<std::pair<free_block_metadata, size_t>> &result);

private:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t size) override;
    
    void do_deallocate_sm(
        void *at) override;

    bool do_is_equal(const std::pmr::memory_resource&) const noexcept override;

    std::vector<allocator_test_utils::block_info> get_blocks_info() const override;
    
    inline void set_fit_mode(allocator_with_fit_mode::fit_mode mode) override;

    void* allocate_first_fit(size_t size);
    void* allocate_best_fit(size_t size);
    void* allocate_worst_fit(size_t size);

    void* on_block_allocate(free_block_metadata* free_block, size_t size);

    void rotate_left(void* ptr);
    void rotate_right(void* ptr);

    void transplant(free_block_metadata* u, free_block_metadata* v);

    void add(free_block_metadata* new_node);
    void remove(free_block_metadata* node);

    void on_node_added(free_block_metadata* new_node);
    void on_node_removed(free_block_metadata* child);

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    class rb_iterator
    {
        void* _block_ptr;
        void* _trusted;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const rb_iterator&) const noexcept;

        bool operator!=(const rb_iterator&) const noexcept;

        rb_iterator& operator++() & noexcept;

        rb_iterator operator++(int n);

        size_t size() const noexcept;

        void* operator*() const noexcept;

        bool occupied()const noexcept;

        rb_iterator();

        rb_iterator(void* trusted);
    };

    friend class rb_iterator;

    rb_iterator begin() const noexcept;
    rb_iterator end() const noexcept;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H