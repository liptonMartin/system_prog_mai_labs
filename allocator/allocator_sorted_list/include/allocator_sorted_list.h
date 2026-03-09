#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H

#include <pp_allocator.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <iterator>
#include <mutex>

class allocator_sorted_list final:
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode
{

private:

    class allocator_metadata {
    public:
        std::pmr::memory_resource *parent_allocator = nullptr;
        fit_mode fit_mode = fit_mode::first_fit;
        size_t size = 0;
        std::mutex mutex;
        void* ptr_to_first_free_block = nullptr;
    };

    class block_metadata {
    public:
        void*ptr = nullptr;
        size_t size = 0;
    };

private:
    
    void *_trusted_memory;

    static constexpr const size_t allocator_metadata_size = sizeof(allocator_metadata);

    static constexpr const size_t block_metadata_size = sizeof(block_metadata);

public:

    explicit allocator_sorted_list(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);
    
    allocator_sorted_list(
        allocator_sorted_list const &other);
    
    allocator_sorted_list &operator=(
        allocator_sorted_list const &other);

    allocator_sorted_list(
        allocator_sorted_list &&other) noexcept;
    
    allocator_sorted_list &operator=(
        allocator_sorted_list &&other) noexcept;

    ~allocator_sorted_list() override;

private:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t size) override;
    
    void do_deallocate_sm(
        void *at) override;

    bool do_is_equal(const std::pmr::memory_resource&) const noexcept override;
    
    inline void set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) override;

    std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

    void* _allocate_first_fit(size_t useful_size);
    void* _allocate_best_fit(size_t useful_size);
    void* _allocate_worst_fit(size_t useful_size);

    void *_link_new_block(void* ptr_to_new_block, void* prev_to_free_block, const size_t useful_size);

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    class sorted_free_iterator
    {
        void* _free_ptr;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const sorted_free_iterator&) const noexcept;

        bool operator!=(const sorted_free_iterator&) const noexcept;

        sorted_free_iterator& operator++() & noexcept;

        sorted_free_iterator operator++(int n);

        size_t size() const noexcept;

        void* operator*() const noexcept;

        sorted_free_iterator();

        sorted_free_iterator(void* trusted);
    };

    class sorted_iterator
    {
        void* _free_ptr;
        void* _current_ptr;
        void* _trusted_memory;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const sorted_iterator&) const noexcept;

        bool operator!=(const sorted_iterator&) const noexcept;

        sorted_iterator& operator++() & noexcept;

        sorted_iterator operator++(int n);

        size_t size() const noexcept;

        void* operator*() const noexcept;

        bool occupied()const noexcept;

        sorted_iterator();

        sorted_iterator(void* trusted);

        sorted_iterator(void* current_ptr, void* free_ptr, void* trusted_memory);
    };

    friend class sorted_iterator;
    friend class sorted_free_iterator;

    sorted_free_iterator free_begin() const noexcept;
    sorted_free_iterator free_end() const noexcept;

    sorted_iterator begin() const noexcept;
    sorted_iterator end() const noexcept;
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_SORTED_LIST_H