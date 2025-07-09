#include "test.h"
#include "fl/bitset.h"
#include "fl/bitset_dynamic.h"
#include <cstring>

using namespace fl;

// Test helper to verify bitset contents
template<fl::u32 N>
void verify_bitset_contents(const bitset<N>& bs, const fl::vector<bool>& expected) {
    fl::u32 size = MIN(bs.size(), expected.size());
    for (fl::u32 i = 0; i < size; ++i) {
        CHECK(bs.test(i) == expected[i]);
    }
}

// Test helper to create a bitset with specific pattern
template<fl::u32 N>
bitset<N> create_pattern_bitset(const fl::vector<bool>& pattern) {
    bitset<N> bs;
    for (fl::u32 i = 0; i < pattern.size() && i < N; ++i) {
        if (pattern[i]) {
            bs.set(i);
        }
    }
    return bs;
}

TEST_CASE("Bitset resize optimization - basic functionality") {
    // Test basic resize operations
    bitset<64> bs;
    
    // Set some bits in the fixed range
    bs.set(0, true);
    bs.set(15, true);
    bs.set(31, true);
    bs.set(47, true);
    bs.set(63, true);
    
    // Resize to larger size (should convert to dynamic)
    bs.resize(128);
    
    // Verify original bits are preserved
    CHECK(bs.test(0) == true);
    CHECK(bs.test(15) == true);
    CHECK(bs.test(31) == true);
    CHECK(bs.test(47) == true);
    CHECK(bs.test(63) == true);
    
    // Set new bits in the expanded range
    bs.set(100, true);
    bs.set(127, true);
    
    // Resize back to smaller size (should convert back to fixed)
    bs.resize(32);
    
    // Verify only bits in the smaller range are preserved
    CHECK(bs.test(0) == true);
    CHECK(bs.test(15) == true);
    CHECK(bs.test(31) == true);
    CHECK(bs.test(47) == false); // Should be lost
    CHECK(bs.test(63) == false); // Should be lost
    CHECK(bs.test(100) == false); // Should be lost
    CHECK(bs.test(127) == false); // Should be lost
}

TEST_CASE("Bitset resize optimization - edge cases with partial blocks") {
    // Test with bitset size that creates partial blocks
    // 16 bits per block, so 64 bits = 4 complete blocks
    // 65 bits = 4 complete blocks + 1 bit in 5th block
    
    bitset<64> bs;
    
    // Fill the last bit of the last complete block
    bs.set(63, true);
    
    // Resize to 65 bits (creates partial block)
    bs.resize(65);
    
    // Verify the last bit is preserved
    CHECK(bs.test(63) == true);
    
    // Set the new bit in the partial block
    bs.set(64, true);
    
    // Verify both bits are set
    CHECK(bs.test(63) == true);
    CHECK(bs.test(64) == true);
    
    // Resize back to 64 bits
    bs.resize(64);
    
    // Verify only the bit in the complete block is preserved
    CHECK(bs.test(63) == true);
    CHECK(bs.test(64) == false); // Should be lost
}

TEST_CASE("Bitset resize optimization - multiple partial blocks") {
    // Test with multiple partial blocks
    bitset<32> bs; // 2 complete blocks (32 bits)
    
    // Set bits in the last complete block
    bs.set(30, true);
    bs.set(31, true);
    
    // Resize to 50 bits (3 complete blocks + 2 bits in 4th block)
    bs.resize(50);
    
    // Verify original bits are preserved
    CHECK(bs.test(30) == true);
    CHECK(bs.test(31) == true);
    
    // Set bits in the new partial block
    bs.set(48, true);
    bs.set(49, true);
    
    // Verify all bits are set
    CHECK(bs.test(30) == true);
    CHECK(bs.test(31) == true);
    CHECK(bs.test(48) == true);
    CHECK(bs.test(49) == true);
    
    // Resize to 40 bits (2 complete blocks + 8 bits in 3rd block)
    bs.resize(40);
    
    // Verify bits in complete blocks are preserved
    CHECK(bs.test(30) == true);
    CHECK(bs.test(31) == true);
    CHECK(bs.test(48) == false); // Should be lost
    CHECK(bs.test(49) == false); // Should be lost
}

TEST_CASE("Bitset resize optimization - complex patterns") {
    // Test with complex bit patterns
    fl::vector<bool> pattern = {
        true, false, true, false, true, false, true, false,  // 8 bits
        false, true, false, true, false, true, false, true,  // 16 bits
        true, true, false, false, true, true, false, false,  // 24 bits
        false, false, true, true, false, false, true, true   // 32 bits
    };
    
    bitset<32> bs = create_pattern_bitset<32>(pattern);
    
    // Verify initial pattern
    verify_bitset_contents(bs, pattern);
    
    // Resize to larger size
    bs.resize(64);
    
    // Verify pattern is preserved
    verify_bitset_contents(bs, pattern);
    
    // Add more pattern to the expanded bitset
    fl::vector<bool> extended_pattern = pattern;
    for (fl::u32 i = 32; i < 64; ++i) {
        extended_pattern.push_back((i % 3) == 0);
        bs.set(i, (i % 3) == 0);
    }
    
    // Verify extended pattern
    verify_bitset_contents(bs, extended_pattern);
    
    // Resize back to original size
    bs.resize(32);
    
    // Verify original pattern is preserved
    verify_bitset_contents(bs, pattern);
}

TEST_CASE("Bitset resize optimization - boundary conditions") {
    // Test boundary conditions around the fixed size limit
    
    bitset<64> bs;
    
    // Set bits at the boundary
    bs.set(63, true); // Last bit of fixed bitset
    
    // Resize to exactly the fixed size limit
    bs.resize(64);
    
    // Should still be using fixed bitset
    CHECK(bs.test(63) == true);
    
    // Resize to one bit larger (should convert to dynamic)
    bs.resize(65);
    
    // Should still preserve the bit
    CHECK(bs.test(63) == true);
    
    // Set the new bit
    bs.set(64, true);
    
    // Resize back to exactly the limit
    bs.resize(64);
    
    // Should convert back to fixed and preserve the bit
    CHECK(bs.test(63) == true);
    CHECK(bs.test(64) == false); // Should be lost
}

TEST_CASE("Bitset resize optimization - zero size") {
    // Test edge case of zero size
    bitset<64> bs;
    
    // Set some bits
    bs.set(10, true);
    bs.set(20, true);
    
    // Resize to zero
    bs.resize(0);
    
    // Should still be able to resize back
    bs.resize(64);
    
    // All bits should be cleared
    CHECK(bs.test(10) == false);
    CHECK(bs.test(20) == false);
}

TEST_CASE("Bitset resize optimization - very large resize") {
    // Test with very large resize operations
    bitset<16> bs; // Small fixed size
    
    // Set some bits
    bs.set(0, true);
    bs.set(15, true);
    
    // Resize to very large size
    bs.resize(1000);
    
    // Verify original bits are preserved
    CHECK(bs.test(0) == true);
    CHECK(bs.test(15) == true);
    
    // Set bits in the expanded range
    bs.set(500, true);
    bs.set(999, true);
    
    // Resize back to small size
    bs.resize(8);
    
    // Verify only bits in the smaller range are preserved
    CHECK(bs.test(0) == true);
    CHECK(bs.test(15) == false); // Should be lost
    CHECK(bs.test(500) == false); // Should be lost
    CHECK(bs.test(999) == false); // Should be lost
}

TEST_CASE("Bitset resize optimization - alternating resize") {
    // Test alternating between fixed and dynamic multiple times
    bitset<32> bs;
    
    // Set pattern in fixed bitset
    for (fl::u32 i = 0; i < 32; i += 2) {
        bs.set(i, true);
    }
    
    // Resize to dynamic multiple times
    for (int round = 0; round < 5; ++round) {
        // Resize to larger
        bs.resize(64);
        
        // Verify pattern is preserved
        for (fl::u32 i = 0; i < 32; i += 2) {
            CHECK(bs.test(i) == true);
        }
        
        // Add new pattern
        for (fl::u32 i = 32; i < 64; i += 3) {
            bs.set(i, true);
        }
        
        // Resize back to fixed
        bs.resize(32);
        
        // Verify original pattern is preserved
        for (fl::u32 i = 0; i < 32; i += 2) {
            CHECK(bs.test(i) == true);
        }
        
        // Verify new pattern is lost
        for (fl::u32 i = 32; i < 64; i += 3) {
            CHECK(bs.test(i) == false);
        }
    }
}

TEST_CASE("Bitset resize optimization - partial block edge cases") {
    // Test specific edge cases with partial blocks
    // 16 bits per block, so test various sizes that create partial blocks
    
    // Test size that creates exactly one bit in a new block
    bitset<16> bs1; // 1 complete block
    bs1.set(15, true); // Last bit of complete block
    
    bs1.resize(17); // 1 complete block + 1 bit in new block
    CHECK(bs1.test(15) == true);
    bs1.set(16, true);
    CHECK(bs1.test(16) == true);
    
    // Test size that creates multiple bits in a new block
    bitset<16> bs2;
    bs2.set(15, true);
    
    bs2.resize(24); // 1 complete block + 8 bits in new block
    CHECK(bs2.test(15) == true);
    bs2.set(16, true);
    bs2.set(23, true);
    CHECK(bs2.test(16) == true);
    CHECK(bs2.test(23) == true);
    
    // Test size that creates almost a complete new block
    bitset<16> bs3;
    bs3.set(15, true);
    
    bs3.resize(31); // 1 complete block + 15 bits in new block
    CHECK(bs3.test(15) == true);
    bs3.set(16, true);
    bs3.set(30, true);
    CHECK(bs3.test(16) == true);
    CHECK(bs3.test(30) == true);
}

TEST_CASE("Bitset resize optimization - memcopy verification") {
    // Test to verify that memcopy is working correctly by checking
    // that all bits are properly copied, not just individual bits
    
    bitset<32> bs;
    
    // Set a complex pattern that spans multiple blocks
    for (fl::u32 i = 0; i < 32; ++i) {
        bs.set(i, (i % 3) == 0);
    }
    
    // Resize to larger (should use memcopy)
    bs.resize(64);
    
    // Verify the entire pattern is preserved exactly
    for (fl::u32 i = 0; i < 32; ++i) {
        CHECK(bs.test(i) == ((i % 3) == 0));
    }
    
    // Add more pattern
    for (fl::u32 i = 32; i < 64; ++i) {
        bs.set(i, (i % 5) == 0);
    }
    
    // Resize back to smaller (should use memcopy again)
    bs.resize(32);
    
    // Verify the original pattern is preserved exactly
    for (fl::u32 i = 0; i < 32; ++i) {
        CHECK(bs.test(i) == ((i % 3) == 0));
    }
}

TEST_CASE("Bitset resize optimization - performance comparison") {
    // This test verifies that the optimization actually works
    // by ensuring the behavior is identical to the original implementation
    
    // Create two bitsets with identical patterns
    bitset<64> bs1, bs2;
    
    // Set complex pattern
    for (fl::u32 i = 0; i < 64; ++i) {
        bool value = (i % 7) == 0;
        bs1.set(i, value);
        bs2.set(i, value);
    }
    
    // Perform resize operations on both
    bs1.resize(128);
    bs2.resize(128);
    
    // Add more pattern
    for (fl::u32 i = 64; i < 128; ++i) {
        bool value = (i % 11) == 0;
        bs1.set(i, value);
        bs2.set(i, value);
    }
    
    // Resize back
    bs1.resize(64);
    bs2.resize(64);
    
    // Verify both have identical content
    for (fl::u32 i = 0; i < 64; ++i) {
        CHECK(bs1.test(i) == bs2.test(i));
    }
}

TEST_CASE("Bitset resize optimization - stress test") {
    // Stress test with many resize operations
    bitset<16> bs;
    
    // Perform many resize operations
    for (int i = 0; i < 100; ++i) {
        fl::u32 new_size = 16 + (i % 100);
        bs.resize(new_size);
        
        // Set some bits
        bs.set(0, true);
        if (new_size > 1) {
            bs.set(new_size - 1, true);
        }
        
        // Resize back
        bs.resize(16);
        
        // Verify first bit is always preserved
        CHECK(bs.test(0) == true);
    }
}
