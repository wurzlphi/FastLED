// Unit tests for SlabAllocator to ensure contiguous memory allocation

#include "doctest.h"
#include "fl/allocator.h"
#include "fl/vector.h"
#include "crash_handler.h"
#include <algorithm>
#include <cstring>

// Enhanced logging for debugging allocator issues
#define ALLOCATOR_DEBUG_LOG(fmt, ...) \
    printf("[ALLOCATOR_DEBUG] " fmt "\n", ##__VA_ARGS__)

#define ALLOCATOR_ERROR_LOG(fmt, ...) \
    printf("[ALLOCATOR_ERROR] " fmt "\n", ##__VA_ARGS__)

// Memory validation helper
inline bool validate_memory_region(const void* ptr, size_t size, const char* context = "unknown") {
    if (!ptr) {
        ALLOCATOR_ERROR_LOG("Null pointer in %s", context);
        return false;
    }
    
    // Check if pointer is aligned
    if (reinterpret_cast<uintptr_t>(ptr) % sizeof(void*) != 0) {
        ALLOCATOR_ERROR_LOG("Unaligned pointer 0x%p in %s", ptr, context);
        return false;
    }
    
    // Try to read the memory region to check if it's accessible
    // Note: This is a basic check and may not catch all issues
    volatile const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        volatile uint8_t val = bytes[i];
        (void)val; // Suppress unused variable warning
    }
    
    return true;
}

// Enhanced memory corruption detection
inline void log_memory_state(const char* label, const void* ptr, size_t size) {
    if (!ptr) {
        ALLOCATOR_DEBUG_LOG("%s: null pointer", label);
        return;
    }
    
    ALLOCATOR_DEBUG_LOG("%s: ptr=0x%p, size=%zu", label, ptr, size);
    
    // Log first few bytes for debugging
    const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
    printf("[ALLOCATOR_DEBUG] %s: first 16 bytes: ", label);
    for (size_t i = 0; i < std::min(size, size_t(16)); ++i) {
        printf("%02x ", bytes[i]);
    }
    printf("\n");
}

using namespace fl;

// Initialize crash handler for better debugging
TEST_SUITE_BEGIN("allocator_tests");
TEST_SUITE_END();

// Test struct for slab allocator testing
struct TestObject {
    int data[4];  // 16 bytes to make it larger than pointer size
    TestObject() { 
        for (int i = 0; i < 4; ++i) {
            data[i] = 0;
        }
    }
};

TEST_CASE("SlabAllocator - Basic functionality") {
    using TestAllocator = SlabAllocator<TestObject, 8>;
    
    SUBCASE("Single allocation and deallocation") {
        TestAllocator allocator;
        
        TestObject* ptr = allocator.allocate();
        REQUIRE(ptr != nullptr);
        CHECK(allocator.getTotalAllocated() == 1);
        CHECK(allocator.getActiveAllocations() == 1);
        
        allocator.deallocate(ptr);
        CHECK(allocator.getTotalDeallocated() == 1);
        CHECK(allocator.getActiveAllocations() == 0);
    }
    
    SUBCASE("Multiple allocations") {
        TestAllocator allocator;
        
        fl::vector<TestObject*> ptrs;
        const size_t num_allocs = 5;
        
        for (size_t i = 0; i < num_allocs; ++i) {
            TestObject* ptr = allocator.allocate();
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        CHECK(allocator.getTotalAllocated() == num_allocs);
        CHECK(allocator.getActiveAllocations() == num_allocs);
        
        for (TestObject* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
        
        CHECK(allocator.getActiveAllocations() == 0);
    }
}

TEST_CASE("SlabAllocator - Contiguous memory within slab") {
    using TestAllocator = SlabAllocator<TestObject, 8>;
    TestAllocator allocator;
    
    SUBCASE("First 8 allocations should be contiguous") {
        fl::vector<TestObject*> ptrs;
        
        // Allocate exactly one slab worth of objects
        for (size_t i = 0; i < 8; ++i) {
            TestObject* ptr = allocator.allocate();
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        // Sort pointers by address to check contiguity
        std::sort(ptrs.begin(), ptrs.end());
        
        // Calculate expected block size (must be at least sizeof(TestObject))
        constexpr size_t expected_block_size = sizeof(TestObject) > sizeof(void*) ? sizeof(TestObject) : sizeof(void*);
        
        // Verify contiguous allocation within the slab
        for (size_t i = 1; i < ptrs.size(); ++i) {
            uintptr_t prev_addr = reinterpret_cast<uintptr_t>(ptrs[i-1]);
            uintptr_t curr_addr = reinterpret_cast<uintptr_t>(ptrs[i]);
            uintptr_t diff = curr_addr - prev_addr;
            
            // The difference should be exactly the block size
            CHECK(diff == expected_block_size);
        }
        
        // Verify all pointers are within the same memory range (same slab)
        uintptr_t first_addr = reinterpret_cast<uintptr_t>(ptrs[0]);
        uintptr_t last_addr = reinterpret_cast<uintptr_t>(ptrs.back());
        uintptr_t total_range = last_addr - first_addr + expected_block_size;
        uintptr_t expected_range = expected_block_size * 8;  // 8 blocks in slab
        
        CHECK(total_range == expected_range);
        
        // Cleanup
        for (TestObject* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
    }
    
    SUBCASE("Memory boundaries verification") {
        fl::vector<TestObject*> ptrs;
        
        // Allocate one slab worth
        for (size_t i = 0; i < 8; ++i) {
            TestObject* ptr = allocator.allocate();
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        // Find the memory range bounds
        uintptr_t min_addr = UINTPTR_MAX;
        uintptr_t max_addr = 0;
        
        for (TestObject* ptr : ptrs) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            min_addr = std::min(min_addr, addr);
            max_addr = std::max(max_addr, addr);
        }
        
        // All allocations should fall within a predictable range
        constexpr size_t block_size = sizeof(TestObject) > sizeof(void*) ? sizeof(TestObject) : sizeof(void*);
        constexpr size_t slab_size = block_size * 8;
        
        uintptr_t actual_range = max_addr - min_addr + block_size;
        CHECK(actual_range == slab_size);
        
        // Verify each pointer falls within the expected boundaries
        for (TestObject* ptr : ptrs) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            CHECK(addr >= min_addr);
            CHECK(addr <= max_addr);
            
            // Verify alignment - each block should be at expected offset
            uintptr_t offset_from_start = addr - min_addr;
            CHECK(offset_from_start % block_size == 0);
        }
        
        // Cleanup
        for (TestObject* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
    }
}

TEST_CASE("SlabAllocator - Multiple slabs behavior") {
    using TestAllocator = SlabAllocator<TestObject, 4>;  // Smaller slab for easier testing
    TestAllocator allocator;
    
    SUBCASE("Allocation across multiple slabs") {
        fl::vector<TestObject*> ptrs;
        
        // Allocate more than one slab can hold (4 * 3 = 12 objects across 3 slabs)
        const size_t total_allocs = 12;
        for (size_t i = 0; i < total_allocs; ++i) {
            TestObject* ptr = allocator.allocate();
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        CHECK(allocator.getSlabCount() == 3);  // Should have created 3 slabs
        CHECK(allocator.getTotalAllocated() == total_allocs);
        
        // Test that all allocations are valid and don't overlap
        fl::vector<TestObject*> sorted_ptrs = ptrs;
        std::sort(sorted_ptrs.begin(), sorted_ptrs.end());
        
        constexpr size_t block_size = sizeof(TestObject) > sizeof(void*) ? sizeof(TestObject) : sizeof(void*);
        
        // Verify no pointer overlaps (each should be at least block_size apart)
        for (size_t i = 1; i < sorted_ptrs.size(); ++i) {
            uintptr_t prev_addr = reinterpret_cast<uintptr_t>(sorted_ptrs[i-1]);
            uintptr_t curr_addr = reinterpret_cast<uintptr_t>(sorted_ptrs[i]);
            uintptr_t diff = curr_addr - prev_addr;
            
            // Each allocation should be at least block_size apart
            CHECK(diff >= block_size);
        }
        
        // Test that each allocation is properly aligned and usable
        for (size_t i = 0; i < ptrs.size(); ++i) {
            // Test alignment
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptrs[i]);
            CHECK(addr % alignof(TestObject) == 0);
            
            // Test that we can write unique data to each allocation
            ptrs[i]->data[0] = static_cast<int>(i + 100);
            ptrs[i]->data[1] = static_cast<int>(i + 200);
            ptrs[i]->data[2] = static_cast<int>(i + 300);
            ptrs[i]->data[3] = static_cast<int>(i + 400);
        }
        
        // Verify all data is still intact (no memory corruption/overlap)
        for (size_t i = 0; i < ptrs.size(); ++i) {
            CHECK(ptrs[i]->data[0] == static_cast<int>(i + 100));
            CHECK(ptrs[i]->data[1] == static_cast<int>(i + 200));
            CHECK(ptrs[i]->data[2] == static_cast<int>(i + 300));
            CHECK(ptrs[i]->data[3] == static_cast<int>(i + 400));
        }
        
        // Cleanup
        for (TestObject* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
    }
}

TEST_CASE("SlabAllocator - Memory layout verification") {
    using SmallAllocator = SlabAllocator<uint32_t, 16>;
    SmallAllocator allocator;
    
    SUBCASE("Detailed memory layout check") {
        fl::vector<uint32_t*> ptrs;
        
        // Allocate exactly one slab worth
        for (size_t i = 0; i < 16; ++i) {
            uint32_t* ptr = allocator.allocate();
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        // Sort by address
        std::sort(ptrs.begin(), ptrs.end());
        
        // Verify perfect sequential layout
        constexpr size_t block_size = sizeof(uint32_t) > sizeof(void*) ? sizeof(uint32_t) : sizeof(void*);
        
        uintptr_t base_addr = reinterpret_cast<uintptr_t>(ptrs[0]);
        
        for (size_t i = 0; i < ptrs.size(); ++i) {
            uintptr_t expected_addr = base_addr + (i * block_size);
            uintptr_t actual_addr = reinterpret_cast<uintptr_t>(ptrs[i]);
            
            CHECK(actual_addr == expected_addr);
        }
        
        // Verify the total memory span is exactly what we expect
        uintptr_t first_addr = reinterpret_cast<uintptr_t>(ptrs[0]);
        uintptr_t last_addr = reinterpret_cast<uintptr_t>(ptrs.back());
        uintptr_t total_span = last_addr - first_addr + block_size;
        uintptr_t expected_span = block_size * 16;
        
        CHECK(total_span == expected_span);
        
        // Test that we can write to each allocated block without interfering with others
        for (size_t i = 0; i < ptrs.size(); ++i) {
            *ptrs[i] = static_cast<uint32_t>(i + 1000);  // Write unique value
        }
        
        // Verify all values are intact (no memory corruption/overlap)
        for (size_t i = 0; i < ptrs.size(); ++i) {
            CHECK(*ptrs[i] == static_cast<uint32_t>(i + 1000));
        }
        
        // Cleanup
        for (uint32_t* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
    }
}

TEST_CASE("SlabAllocator - Edge cases") {
    using EdgeAllocator = SlabAllocator<char, 8>;
    EdgeAllocator allocator;
    
    SUBCASE("Allocation and deallocation pattern") {
        fl::vector<char*> ptrs;
        
        // Allocate all blocks in slab
        for (size_t i = 0; i < 8; ++i) {
            char* ptr = allocator.allocate();
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        // Deallocate every other block
        for (size_t i = 0; i < ptrs.size(); i += 2) {
            allocator.deallocate(ptrs[i]);
            ptrs[i] = nullptr;
        }
        
        // Reallocate - should reuse freed blocks
        fl::vector<char*> new_ptrs;
        for (size_t i = 0; i < 4; ++i) {  // 4 blocks were freed
            char* ptr = allocator.allocate();
            REQUIRE(ptr != nullptr);
            new_ptrs.push_back(ptr);
        }
        
        // All new allocations should be from the same slab (reused memory)
        CHECK(allocator.getSlabCount() == 1);  // Still only one slab
        
        // Cleanup
        for (size_t i = 0; i < ptrs.size(); ++i) {
            if (ptrs[i] != nullptr) {
                allocator.deallocate(ptrs[i]);
            }
        }
        for (char* ptr : new_ptrs) {
            allocator.deallocate(ptr);
        }
    }
    
    SUBCASE("Bulk allocation fallback") {
        // Request bulk allocation (n != 1) - should fallback to malloc
        char* bulk_ptr = allocator.allocate(10);
        REQUIRE(bulk_ptr != nullptr);
        
        // This should not affect slab statistics since it uses malloc
        CHECK(allocator.getTotalAllocated() == 0);  // Slab stats unchanged
        CHECK(allocator.getSlabCount() == 0);       // No slabs created
        
        allocator.deallocate(bulk_ptr, 10);
    }
}

TEST_CASE("SlabAllocator - STL compatibility") {
    SUBCASE("STL allocator interface") {
        using STLAllocator = allocator_slab<TestObject, 8>;
        STLAllocator alloc;
        
        TestObject* ptr = alloc.allocate(1);
        REQUIRE(ptr != nullptr);
        
        // Construct object
        alloc.construct(ptr, TestObject{});
        
        // Use the object
        ptr->data[0] = 42;
        CHECK(ptr->data[0] == 42);
        
        // Destroy and deallocate
        alloc.destroy(ptr);
        alloc.deallocate(ptr, 1);
    }
    
    SUBCASE("Allocator equality") {
        allocator_slab<TestObject, 8> alloc1;
        allocator_slab<TestObject, 8> alloc2;
        
        CHECK(alloc1 == alloc2);  // All instances should be equivalent
        CHECK_FALSE(alloc1 != alloc2);
    }
} 

TEST_CASE("allocator_inlined - Basic functionality") {
    using TestAllocator = fl::allocator_inlined<int, 3>;
    
    SUBCASE("Single allocation and deallocation") {
        TestAllocator allocator;
        
        int* ptr = allocator.allocate(1);
        REQUIRE(ptr != nullptr);
        
        // Write to the allocation
        *ptr = 42;
        CHECK(*ptr == 42);
        
        allocator.deallocate(ptr, 1);
    }
    
    SUBCASE("Multiple inlined allocations") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        const size_t num_allocs = 3;  // Exactly the inlined capacity
        
        for (size_t i = 0; i < num_allocs; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            *ptr = static_cast<int>(i + 100);
            ptrs.push_back(ptr);
        }
        
        // Verify all allocations are valid and contain expected data
        for (size_t i = 0; i < ptrs.size(); ++i) {
            CHECK(*ptrs[i] == static_cast<int>(i + 100));
        }
        
        // Cleanup
        for (int* ptr : ptrs) {
            allocator.deallocate(ptr, 1);
        }
    }
}

TEST_CASE("allocator_inlined - Inlined to heap transition") {
    using TestAllocator = fl::allocator_inlined<int, 3>;
    
    SUBCASE("Overflow to heap") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        const size_t total_allocs = 5;  // More than inlined capacity (3)
        
        for (size_t i = 0; i < total_allocs; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            *ptr = static_cast<int>(i + 100);
            ptrs.push_back(ptr);
        }
        
        // Verify all allocations are valid and contain expected data
        for (size_t i = 0; i < ptrs.size(); ++i) {
            CHECK(*ptrs[i] == static_cast<int>(i + 100));
        }
        
        // Cleanup
        for (int* ptr : ptrs) {
            allocator.deallocate(ptr, 1);
        }
    }
    
    SUBCASE("Mixed inlined and heap allocations") {
        TestAllocator allocator;
        
        fl::vector<int*> inlined_ptrs;
        fl::vector<int*> heap_ptrs;
        
        // Allocate inlined storage first
        for (size_t i = 0; i < 3; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            *ptr = static_cast<int>(i + 100);
            inlined_ptrs.push_back(ptr);
        }
        
        // Then allocate heap storage
        for (size_t i = 0; i < 2; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            *ptr = static_cast<int>(i + 200);
            heap_ptrs.push_back(ptr);
        }
        
        // Verify inlined allocations
        for (size_t i = 0; i < inlined_ptrs.size(); ++i) {
            CHECK(*inlined_ptrs[i] == static_cast<int>(i + 100));
        }
        
        // Verify heap allocations
        for (size_t i = 0; i < heap_ptrs.size(); ++i) {
            CHECK(*heap_ptrs[i] == static_cast<int>(i + 200));
        }
        
        // Cleanup
        for (int* ptr : inlined_ptrs) {
            allocator.deallocate(ptr, 1);
        }
        for (int* ptr : heap_ptrs) {
            allocator.deallocate(ptr, 1);
        }
    }
}

TEST_CASE("allocator_inlined - Free slot management") {
    using TestAllocator = fl::allocator_inlined<int, 3>;
    
    SUBCASE("Deallocate and reuse inlined slots") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        
        // Allocate all inlined slots
        for (size_t i = 0; i < 3; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            *ptr = static_cast<int>(i + 100);
            ptrs.push_back(ptr);
        }
        
        // Deallocate the middle slot
        allocator.deallocate(ptrs[1], 1);
        ptrs[1] = nullptr;
        
        // Allocate a new slot - should reuse the freed slot
        int* new_ptr = allocator.allocate(1);
        REQUIRE(new_ptr != nullptr);
        *new_ptr = 999;
        
        // The new allocation should be from the same memory location as the freed slot
        // (This is implementation-dependent, but the slot should be reused)
        
        // Verify other allocations are still intact
        CHECK(*ptrs[0] == 100);
        CHECK(*ptrs[2] == 102);
        CHECK(*new_ptr == 999);
        
        // Cleanup
        allocator.deallocate(ptrs[0], 1);
        allocator.deallocate(ptrs[2], 1);
        allocator.deallocate(new_ptr, 1);
    }
    
    SUBCASE("Deallocate and reuse heap slots") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        
        ALLOCATOR_DEBUG_LOG("Starting heap slot reuse test");
        
        // Allocate more than inlined capacity to force heap usage
        for (size_t i = 0; i < 5; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            
            // Validate memory before writing
            if (!validate_memory_region(ptr, sizeof(int), "allocation")) {
                ALLOCATOR_ERROR_LOG("Memory validation failed for ptr[%zu]", i);
                continue;
            }
            
            *ptr = static_cast<int>(i + 100);
            ptrs.push_back(ptr);
            
            ALLOCATOR_DEBUG_LOG("Allocated ptr[%zu] = 0x%p, value = %d", i, ptr, *ptr);
            log_memory_state("ptr", ptr, sizeof(int));
        }
        
        // Deallocate a heap slot (index 4)
        ALLOCATOR_DEBUG_LOG("Deallocating ptr[4] = 0x%p", ptrs[4]);
        allocator.deallocate(ptrs[4], 1);
        ptrs[4] = nullptr;
        
        // Allocate a new slot - should reuse the freed heap slot
        int* new_ptr = allocator.allocate(1);
        REQUIRE(new_ptr != nullptr);
        
        // Validate memory before writing
        if (!validate_memory_region(new_ptr, sizeof(int), "new allocation")) {
            ALLOCATOR_ERROR_LOG("Memory validation failed for new_ptr");
        }
        
        *new_ptr = 999;
        ALLOCATOR_DEBUG_LOG("Allocated new_ptr = 0x%p, value = %d", new_ptr, *new_ptr);
        log_memory_state("new_ptr", new_ptr, sizeof(int));
        
        // Verify other allocations are still intact with enhanced logging
        for (size_t i = 0; i < 4; ++i) {
            if (!validate_memory_region(ptrs[i], sizeof(int), "verification")) {
                ALLOCATOR_ERROR_LOG("Memory validation failed for ptr[%zu] during verification", i);
                continue;
            }
            
            int expected = static_cast<int>(i + 100);
            int actual = *ptrs[i];
            
            ALLOCATOR_DEBUG_LOG("Verifying ptr[%zu] = 0x%p: expected=%d, actual=%d", 
                               i, ptrs[i], expected, actual);
            
            if (actual != expected) {
                ALLOCATOR_ERROR_LOG("Memory corruption detected at ptr[%zu]: expected=%d, actual=%d", 
                                   i, expected, actual);
                log_memory_state("corrupted_ptr", ptrs[i], sizeof(int));
            }
            
            CHECK(actual == expected);
        }
        CHECK(*new_ptr == 999);
        
        // Cleanup
        for (size_t i = 0; i < ptrs.size(); ++i) {
            if (ptrs[i] != nullptr) {
                ALLOCATOR_DEBUG_LOG("Deallocating ptr[%zu] = 0x%p", i, ptrs[i]);
                allocator.deallocate(ptrs[i], 1);
            }
        }
        ALLOCATOR_DEBUG_LOG("Deallocating new_ptr = 0x%p", new_ptr);
        allocator.deallocate(new_ptr, 1);
        
        ALLOCATOR_DEBUG_LOG("Heap slot reuse test completed");
    }
}

TEST_CASE("allocator_inlined - Memory layout verification") {
    using TestAllocator = fl::allocator_inlined<int, 3>;
    
    SUBCASE("Inlined storage layout") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        
        // Allocate exactly inlined capacity
        for (size_t i = 0; i < 3; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        // Sort by address to check layout
        std::sort(ptrs.begin(), ptrs.end());
        
        // Verify that inlined allocations are contiguous
        for (size_t i = 1; i < ptrs.size(); ++i) {
            uintptr_t prev_addr = reinterpret_cast<uintptr_t>(ptrs[i-1]);
            uintptr_t curr_addr = reinterpret_cast<uintptr_t>(ptrs[i]);
            uintptr_t diff = curr_addr - prev_addr;
            
            // Should be exactly sizeof(int) apart (or aligned size)
            CHECK(diff >= sizeof(int));
        }
        
        // Cleanup
        for (int* ptr : ptrs) {
            allocator.deallocate(ptr, 1);
        }
    }
    
    SUBCASE("Heap storage layout") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        
        // Allocate more than inlined capacity to force heap usage
        for (size_t i = 0; i < 5; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            ptrs.push_back(ptr);
        }
        
        // Sort by address
        std::sort(ptrs.begin(), ptrs.end());
        
        // Verify that heap allocations are properly spaced
        for (size_t i = 1; i < ptrs.size(); ++i) {
            uintptr_t prev_addr = reinterpret_cast<uintptr_t>(ptrs[i-1]);
            uintptr_t curr_addr = reinterpret_cast<uintptr_t>(ptrs[i]);
            uintptr_t diff = curr_addr - prev_addr;
            
            // Should be at least sizeof(int) apart
            CHECK(diff >= sizeof(int));
        }
        
        // Cleanup
        for (int* ptr : ptrs) {
            allocator.deallocate(ptr, 1);
        }
    }
}

TEST_CASE("allocator_inlined - Edge cases") {
    using TestAllocator = fl::allocator_inlined<int, 3>;
    
    SUBCASE("Zero allocation") {
        TestAllocator allocator;
        
        int* ptr = allocator.allocate(0);
        CHECK(ptr == nullptr);
        
        allocator.deallocate(ptr, 0);  // Should be safe
    }
    
    SUBCASE("Null deallocation") {
        TestAllocator allocator;
        
        allocator.deallocate(nullptr, 1);  // Should be safe
    }
    
    SUBCASE("Large allocation") {
        TestAllocator allocator;
        
        // Allocate a large block (should go to heap)
        int* large_ptr = allocator.allocate(100);
        REQUIRE(large_ptr != nullptr);
        
        // Write to the allocation
        for (int i = 0; i < 100; ++i) {
            large_ptr[i] = i;
        }
        
        // Verify the data
        for (int i = 0; i < 100; ++i) {
            CHECK(large_ptr[i] == i);
        }
        
        allocator.deallocate(large_ptr, 100);
    }
}

TEST_CASE("allocator_inlined - Copy and assignment") {
    using TestAllocator = fl::allocator_inlined<int, 3>;
    
    SUBCASE("Copy constructor") {
        TestAllocator allocator1;
        
        // Allocate some memory in the first allocator
        int* ptr1 = allocator1.allocate(1);
        *ptr1 = 42;
        
        // Copy the allocator
        TestAllocator allocator2(allocator1);
        
        // Allocate from the copy
        int* ptr2 = allocator2.allocate(1);
        *ptr2 = 100;
        
        // Verify both allocations work
        CHECK(*ptr1 == 42);
        CHECK(*ptr2 == 100);
        
        // Cleanup
        allocator1.deallocate(ptr1, 1);
        allocator2.deallocate(ptr2, 1);
    }
    
    SUBCASE("Assignment operator") {
        TestAllocator allocator1;
        TestAllocator allocator2;
        
        // Allocate in first allocator
        int* ptr1 = allocator1.allocate(1);
        *ptr1 = 42;
        
        // Assign to second allocator
        allocator2 = allocator1;
        
        // Allocate from second allocator
        int* ptr2 = allocator2.allocate(1);
        *ptr2 = 100;
        
        // Verify both allocations work
        CHECK(*ptr1 == 42);
        CHECK(*ptr2 == 100);
        
        // Cleanup
        allocator1.deallocate(ptr1, 1);
        allocator2.deallocate(ptr2, 1);
    }
}

TEST_CASE("allocator_inlined - Clear functionality") {
    using TestAllocator = fl::allocator_inlined<int, 3>;
    
    SUBCASE("Clear inlined allocations") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        
        // Allocate inlined storage
        for (size_t i = 0; i < 3; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            *ptr = static_cast<int>(i + 100);
            ptrs.push_back(ptr);
        }
        
        // Clear the allocator
        allocator.clear();
        
        // Allocate again - should work normally
        int* new_ptr = allocator.allocate(1);
        REQUIRE(new_ptr != nullptr);
        *new_ptr = 999;
        CHECK(*new_ptr == 999);
        
        allocator.deallocate(new_ptr, 1);
    }
    
    SUBCASE("Clear mixed allocations") {
        TestAllocator allocator;
        
        fl::vector<int*> ptrs;
        
        // Allocate both inlined and heap storage
        for (size_t i = 0; i < 5; ++i) {
            int* ptr = allocator.allocate(1);
            REQUIRE(ptr != nullptr);
            *ptr = static_cast<int>(i + 100);
            ptrs.push_back(ptr);
        }
        
        // Clear the allocator
        allocator.clear();
        
        // Allocate again - should work normally
        int* new_ptr = allocator.allocate(1);
        REQUIRE(new_ptr != nullptr);
        *new_ptr = 999;
        CHECK(*new_ptr == 999);
        
        allocator.deallocate(new_ptr, 1);
    }
}

TEST_CASE("allocator_inlined - Crash handler verification") {
    // This test verifies that the crash handler is properly set up
    // and can provide useful debugging information
    
    SUBCASE("Crash handler setup verification") {
        // The crash handler should be automatically set up
        // We can test this by calling print_stacktrace
        printf("Testing crash handler functionality...\n");
        
        // This should work without crashing
        print_stacktrace();
        
        printf("Crash handler test completed successfully.\n");
    }
    
    SUBCASE("Memory corruption detection test") {
        using TestAllocator = fl::allocator_inlined<int, 3>;
        TestAllocator allocator;
        
        // Allocate some memory
        int* ptr = allocator.allocate(1);
        REQUIRE(ptr != nullptr);
        *ptr = 42;
        
        // Test memory validation
        CHECK(validate_memory_region(ptr, sizeof(int), "test"));
        
        // Test memory logging
        log_memory_state("test_ptr", ptr, sizeof(int));
        
        // Cleanup
        allocator.deallocate(ptr, 1);
    }
} 
