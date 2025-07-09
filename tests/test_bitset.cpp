// g++ --std=c++11 test.cpp

#include "test.h"
#include "fl/bitset.h"
#include "fl/bitset_dynamic.h"


using namespace fl;

TEST_CASE("test bitset") {
    // default‐constructed bitset is empty
    bitset_fixed<10> bs;
    REQUIRE_EQ(bs.none(), true);
    REQUIRE_EQ(bs.count(), 0);
    REQUIRE_EQ(bs.size(), 10);

    // set a bit
    bs.set(3);
    REQUIRE_EQ(bs.test(3), true);
    REQUIRE_EQ(bs[3], true);
    REQUIRE_EQ(bs.any(), true);
    REQUIRE_EQ(bs.count(), 1);

    // reset that bit
    bs.reset(3);
    REQUIRE_EQ(bs.test(3), false);
    REQUIRE_EQ(bs.none(), true);

    // toggle a bit
    bs.flip(2);
    REQUIRE_EQ(bs.test(2), true);
    bs.flip(2);
    REQUIRE_EQ(bs.test(2), false);

    // flip all bits
    bitset_fixed<5> bs2;
    for (size_t i = 0; i < 5; ++i)
        bs2.set(i, (i % 2) == 0);
    auto bs2_flipped = ~bs2;
    for (size_t i = 0; i < 5; ++i)
        REQUIRE_EQ(bs2_flipped.test(i), !bs2.test(i));

    // all() and count()
    bitset_fixed<4> bs3;
    for (size_t i = 0; i < 4; ++i)
        bs3.set(i);
    REQUIRE_EQ(bs3.all(), true);
    REQUIRE_EQ(bs3.count(), 4);

    // check that the count can auto expand
    bs3.set(100);
    REQUIRE_EQ(bs3.count(), 4);

    // bitwise AND, OR, XOR
    bitset_fixed<4> a, b;
    a.set(0); a.set(2);
    b.set(1); b.set(2);

    auto or_ab  = a | b;
    REQUIRE_EQ(or_ab.test(0), true);
    REQUIRE_EQ(or_ab.test(1), true);
    REQUIRE_EQ(or_ab.test(2), true);
    REQUIRE_EQ(or_ab.test(3), false);

    auto and_ab = a & b;
    REQUIRE_EQ(and_ab.test(2), true);
    REQUIRE_EQ(and_ab.test(0), false);

    auto xor_ab  = a ^ b;
    REQUIRE_EQ(xor_ab.test(0), true);
    REQUIRE_EQ(xor_ab.test(1), true);
    REQUIRE_EQ(xor_ab.test(2), false);

    // reset and none()
    a.reset(); b.reset();
    REQUIRE_EQ(a.none(), true);
    
    // Test expected size of bitset_fixed
    REQUIRE_EQ(bitset_fixed<8>().size(), 8);
    REQUIRE_EQ(bitset_fixed<16>().size(), 16);
    REQUIRE_EQ(bitset_fixed<32>().size(), 32);
    REQUIRE_EQ(bitset_fixed<64>().size(), 64);
    REQUIRE_EQ(bitset_fixed<100>().size(), 100);
    REQUIRE_EQ(bitset_fixed<1000>().size(), 1000);
    
    // Test memory size of bitset_fixed class (sizeof)
    // For bitset_fixed<8>, we expect 1 uint16_t block (2 bytes)
    REQUIRE_EQ(sizeof(bitset_fixed<8>), 2);
    
    // For bitset_fixed<16>, we expect 1 uint16_t block (2 bytes)
    REQUIRE_EQ(sizeof(bitset_fixed<16>), 2);
    
    // For bitset_fixed<17>, we expect 2 uint16_t blocks (4 bytes)
    REQUIRE_EQ(sizeof(bitset_fixed<17>), 4);
    
    // For bitset_fixed<32>, we expect 2 uint16_t blocks (4 bytes)
    REQUIRE_EQ(sizeof(bitset_fixed<32>), 4);
    
    // For bitset_fixed<33>, we expect 3 uint16_t blocks (6 bytes)
    REQUIRE_EQ(sizeof(bitset_fixed<33>), 6);
}


TEST_CASE("compare fixed and dynamic bitsets") {
    // Test that fixed and dynamic bitsets behave the same
    bitset_fixed<10> fixed_bs;
    fl::bitset_dynamic dynamic_bs(10);
    
    // Set the same bits in both
    fixed_bs.set(1);
    fixed_bs.set(5);
    fixed_bs.set(9);
    
    dynamic_bs.set(1);
    dynamic_bs.set(5);
    dynamic_bs.set(9);
    
    // Verify they have the same state
    REQUIRE_EQ(fixed_bs.size(), dynamic_bs.size());
    REQUIRE_EQ(fixed_bs.count(), dynamic_bs.count());
    
    for (size_t i = 0; i < 10; ++i) {
        REQUIRE_EQ(fixed_bs.test(i), dynamic_bs.test(i));
    }
}



TEST_CASE("test bitset_dynamic") {
    // default-constructed bitset is empty
    bitset_dynamic bs;
    REQUIRE_EQ(bs.size(), 0);
    REQUIRE_EQ(bs.none(), true);
    REQUIRE_EQ(bs.count(), 0);
    
    // resize and test
    bs.resize(10);
    REQUIRE_EQ(bs.size(), 10);
    REQUIRE_EQ(bs.none(), true);
    
    // set a bit
    bs.set(3);
    REQUIRE_EQ(bs.test(3), true);
    REQUIRE_EQ(bs[3], true);
    REQUIRE_EQ(bs.any(), true);
    REQUIRE_EQ(bs.count(), 1);
    
    // reset that bit
    bs.reset(3);
    REQUIRE_EQ(bs.test(3), false);
    REQUIRE_EQ(bs.none(), true);
    
    // toggle a bit
    bs.flip(2);
    REQUIRE_EQ(bs.test(2), true);
    bs.flip(2);
    REQUIRE_EQ(bs.test(2), false);
    
    // resize larger
    bs.set(5);
    bs.resize(20);
    REQUIRE_EQ(bs.size(), 20);
    REQUIRE_EQ(bs.test(5), true);
    REQUIRE_EQ(bs.count(), 1);
    
    // resize smaller (truncate)
    bs.resize(4);
    REQUIRE_EQ(bs.size(), 4);
    REQUIRE_EQ(bs.test(5), false); // out of range now
    REQUIRE_EQ(bs.count(), 0);
    
    // test with larger sizes that span multiple blocks
    bitset_dynamic large_bs(100);
    large_bs.set(0);
    large_bs.set(63);
    large_bs.set(64);
    large_bs.set(99);
    REQUIRE_EQ(large_bs.count(), 4);
    REQUIRE_EQ(large_bs.test(0), true);
    REQUIRE_EQ(large_bs.test(63), true);
    REQUIRE_EQ(large_bs.test(64), true);
    REQUIRE_EQ(large_bs.test(99), true);
    
    // flip all bits
    bitset_dynamic bs2(5);
    for (size_t i = 0; i < 5; ++i)
        bs2.set(i, (i % 2) == 0);
    
    bs2.flip();
    for (size_t i = 0; i < 5; ++i)
        REQUIRE_EQ(bs2.test(i), !((i % 2) == 0));
    
    // all() and count()
    bitset_dynamic bs3(4);
    for (size_t i = 0; i < 4; ++i)
        bs3.set(i);
    REQUIRE_EQ(bs3.all(), true);
    REQUIRE_EQ(bs3.count(), 4);
    
    // out-of-range ops are no-ops
    bs3.set(100);
    REQUIRE_EQ(bs3.count(), 4);
    
    // bitwise AND, OR, XOR
    bitset_dynamic a(4), b(4);
    a.set(0); a.set(2);
    b.set(1); b.set(2);
    
    auto or_ab = a | b;
    REQUIRE_EQ(or_ab.test(0), true);
    REQUIRE_EQ(or_ab.test(1), true);
    REQUIRE_EQ(or_ab.test(2), true);
    REQUIRE_EQ(or_ab.test(3), false);
    
    auto and_ab = a & b;
    REQUIRE_EQ(and_ab.test(2), true);
    REQUIRE_EQ(and_ab.test(0), false);
    
    auto xor_ab = a ^ b;
    REQUIRE_EQ(xor_ab.test(0), true);
    REQUIRE_EQ(xor_ab.test(1), true);
    REQUIRE_EQ(xor_ab.test(2), false);
    
    // reset and none()
    a.reset(); b.reset();
    REQUIRE_EQ(a.none(), true);
    REQUIRE_EQ(b.none(), true);
    
    // copy constructor
    bitset_dynamic original(10);
    original.set(3);
    original.set(7);
    
    bitset_dynamic copy(original);
    REQUIRE_EQ(copy.size(), 10);
    REQUIRE_EQ(copy.test(3), true);
    REQUIRE_EQ(copy.test(7), true);
    REQUIRE_EQ(copy.count(), 2);
    
    // move constructor
    bitset_dynamic moved(fl::move(copy));
    REQUIRE_EQ(moved.size(), 10);
    REQUIRE_EQ(moved.test(3), true);
    REQUIRE_EQ(moved.test(7), true);
    REQUIRE_EQ(moved.count(), 2);
    REQUIRE_EQ(copy.size(), 0); // moved from should be empty
    
    // assignment operator
    bitset_dynamic assigned = original;
    REQUIRE_EQ(assigned.size(), 10);
    REQUIRE_EQ(assigned.test(3), true);
    REQUIRE_EQ(assigned.test(7), true);
    
    // clear
    assigned.clear();
    REQUIRE_EQ(assigned.size(), 0);
    REQUIRE_EQ(assigned.none(), true);
    
    // Test memory size changes with resize
    bitset_dynamic small_bs(8);
    bitset_dynamic medium_bs(65);
    bitset_dynamic large_bs2(129);
    
    // These sizes should match the fixed bitset tests
    REQUIRE_EQ(small_bs.size(), 8);
    REQUIRE_EQ(medium_bs.size(), 65);
    REQUIRE_EQ(large_bs2.size(), 129);
}


TEST_CASE("test bitset_fixed find_first") {
    // Test find_first for true bits
    bitset_fixed<64> bs;
    
    // Initially no bits are set, so find_first(true) should return -1
    REQUIRE_EQ(bs.find_first(true), -1);
    
    // find_first(false) should return 0 (first unset bit)
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Set bit at position 5
    bs.set(5);
    REQUIRE_EQ(bs.find_first(true), 5);
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Set bit at position 0
    bs.set(0);
    REQUIRE_EQ(bs.find_first(true), 0);
    REQUIRE_EQ(bs.find_first(false), 1);
    
    // Set bit at position 63 (last bit)
    bs.set(63);
    REQUIRE_EQ(bs.find_first(true), 0);
    REQUIRE_EQ(bs.find_first(false), 1);
    
    // Clear bit 0, now first set bit should be 5
    bs.reset(0);
    REQUIRE_EQ(bs.find_first(true), 5);
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Test with larger bitset
    bitset_fixed<128> bs2;
    bs2.set(100);
    REQUIRE_EQ(bs2.find_first(true), 100);
    REQUIRE_EQ(bs2.find_first(false), 0);
    
    // Test edge case: all bits set
    bitset_fixed<8> bs3;
    for (fl::u32 i = 0; i < 8; ++i) {
        bs3.set(i);
    }
    REQUIRE_EQ(bs3.find_first(true), 0);
    REQUIRE_EQ(bs3.find_first(false), -1);
    
    // Test edge case: no bits set
    bitset_fixed<8> bs4;
    REQUIRE_EQ(bs4.find_first(true), -1);
    REQUIRE_EQ(bs4.find_first(false), 0);
}

TEST_CASE("test bitset_dynamic find_first") {
    // Test find_first for dynamic bitset
    bitset_dynamic bs(64);
    
    // Initially no bits are set, so find_first(true) should return -1
    REQUIRE_EQ(bs.find_first(true), -1);
    
    // find_first(false) should return 0 (first unset bit)
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Set bit at position 5
    bs.set(5);
    REQUIRE_EQ(bs.find_first(true), 5);
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Set bit at position 0
    bs.set(0);
    REQUIRE_EQ(bs.find_first(true), 0);
    REQUIRE_EQ(bs.find_first(false), 1);
    
    // Set bit at position 63 (last bit)
    bs.set(63);
    REQUIRE_EQ(bs.find_first(true), 0);
    REQUIRE_EQ(bs.find_first(false), 1);
    
    // Clear bit 0, now first set bit should be 5
    bs.reset(0);
    REQUIRE_EQ(bs.find_first(true), 5);
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Test with all bits set
    bitset_dynamic bs2(16);
    for (fl::u32 i = 0; i < 16; ++i) {
        bs2.set(i);
    }
    REQUIRE_EQ(bs2.find_first(true), 0);
    REQUIRE_EQ(bs2.find_first(false), -1);
    
    // Test with no bits set
    bitset_dynamic bs3(16);
    REQUIRE_EQ(bs3.find_first(true), -1);
    REQUIRE_EQ(bs3.find_first(false), 0);
}

TEST_CASE("test bitset_inlined find_first") {
    // Test find_first for inlined bitset (uses fixed bitset internally for small sizes)
    bitset<64> bs;
    
    // Initially no bits are set, so find_first(true) should return -1
    REQUIRE_EQ(bs.find_first(true), -1);
    
    // find_first(false) should return 0 (first unset bit)
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Set bit at position 5
    bs.set(5);
    REQUIRE_EQ(bs.find_first(true), 5);
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Set bit at position 0
    bs.set(0);
    REQUIRE_EQ(bs.find_first(true), 0);
    REQUIRE_EQ(bs.find_first(false), 1);
    
    // Set bit at position 63 (last bit)
    bs.set(63);
    REQUIRE_EQ(bs.find_first(true), 0);
    REQUIRE_EQ(bs.find_first(false), 1);
    
    // Clear bit 0, now first set bit should be 5
    bs.reset(0);
    REQUIRE_EQ(bs.find_first(true), 5);
    REQUIRE_EQ(bs.find_first(false), 0);
    
    // Test with all bits set
    bitset<16> bs2;
    for (fl::u32 i = 0; i < 16; ++i) {
        bs2.set(i);
    }
    REQUIRE_EQ(bs2.find_first(true), 0);
    REQUIRE_EQ(bs2.find_first(false), -1);
    
    // Test with no bits set
    bitset<16> bs3;
    REQUIRE_EQ(bs3.find_first(true), -1);
    REQUIRE_EQ(bs3.find_first(false), 0);
    
    // Test with larger size that uses dynamic bitset internally
    bitset<300> bs4;
    bs4.set(150);
    REQUIRE_EQ(bs4.find_first(true), 150);
    REQUIRE_EQ(bs4.find_first(false), 0);
}

// --- Bitset resize and edge case tests folded from test_bitset_resize_optimization.cpp ---

// Test helper to verify bitset contents
template<typename BitsetT>
static void verify_bitset_contents(const BitsetT& bs, const fl::vector<bool>& expected) {
    fl::u32 size = MIN(bs.size(), expected.size());
    for (fl::u32 i = 0; i < size; ++i) {
        REQUIRE_EQ(bs.test(i), expected[i]);
    }
}

TEST_CASE("bitset resize and edge cases") {
    // Basic resize up/down and bit preservation
    bitset<64> bs;
    bs.set(0, true); bs.set(15, true); bs.set(31, true); bs.set(47, true); bs.set(63, true);
    bs.resize(128);
    REQUIRE_EQ(bs.test(0), true);
    REQUIRE_EQ(bs.test(15), true);
    REQUIRE_EQ(bs.test(31), true);
    REQUIRE_EQ(bs.test(47), true);
    REQUIRE_EQ(bs.test(63), true);
    bs.set(100, true); bs.set(127, true);
    bs.resize(32);
    REQUIRE_EQ(bs.test(0), true);
    REQUIRE_EQ(bs.test(15), true);
    REQUIRE_EQ(bs.test(31), true);
    REQUIRE_EQ(bs.test(47), false);
    REQUIRE_EQ(bs.test(63), false);
    REQUIRE_EQ(bs.test(100), false);
    REQUIRE_EQ(bs.test(127), false);

    // Edge case: partial blocks
    bitset<64> bs2;
    bs2.set(63, true);
    bs2.resize(65);
    REQUIRE_EQ(bs2.test(63), true);
    bs2.set(64, true);
    REQUIRE_EQ(bs2.test(64), true);
    bs2.resize(64);
    REQUIRE_EQ(bs2.test(63), true);
    REQUIRE_EQ(bs2.test(64), false);

    // Multiple partial blocks
    bitset<32> bs3;
    bs3.set(30, true); bs3.set(31, true);
    bs3.resize(50);
    REQUIRE_EQ(bs3.test(30), true);
    REQUIRE_EQ(bs3.test(31), true);
    bs3.set(48, true); bs3.set(49, true);
    REQUIRE_EQ(bs3.test(48), true);
    REQUIRE_EQ(bs3.test(49), true);
    bs3.resize(40);
    REQUIRE_EQ(bs3.test(30), true);
    REQUIRE_EQ(bs3.test(31), true);
    REQUIRE_EQ(bs3.test(48), false);
    REQUIRE_EQ(bs3.test(49), false);

    // Complex pattern
    fl::vector<bool> pattern = {
        true, false, true, false, true, false, true, false,
        false, true, false, true, false, true, false, true,
        true, true, false, false, true, true, false, false,
        false, false, true, true, false, false, true, true
    };
    bitset<32> bs4;
    for (fl::u32 i = 0; i < pattern.size(); ++i) if (pattern[i]) bs4.set(i);
    verify_bitset_contents(bs4, pattern);
    bs4.resize(64);
    verify_bitset_contents(bs4, pattern);
    for (fl::u32 i = 32; i < 64; ++i) bs4.set(i, (i % 3) == 0);
    bs4.resize(32);
    verify_bitset_contents(bs4, pattern);

    // Boundary conditions
    bitset<64> bs5;
    bs5.set(63, true);
    bs5.resize(64);
    REQUIRE_EQ(bs5.test(63), true);
    bs5.resize(65);
    REQUIRE_EQ(bs5.test(63), true);
    bs5.set(64, true);
    bs5.resize(64);
    REQUIRE_EQ(bs5.test(63), true);
    REQUIRE_EQ(bs5.test(64), false);

    // Zero size
    bitset<64> bs6;
    bs6.set(10, true); bs6.set(20, true);
    bs6.resize(0);
    bs6.resize(64);
    REQUIRE_EQ(bs6.test(10), false);
    REQUIRE_EQ(bs6.test(20), false);

    // Very large resize
    bitset<16> bs7;
    bs7.set(0, true); bs7.set(15, true);
    bs7.resize(1000);
    REQUIRE_EQ(bs7.test(0), true);
    REQUIRE_EQ(bs7.test(15), true);
    bs7.set(500, true); bs7.set(999, true);
    bs7.resize(8);
    REQUIRE_EQ(bs7.test(0), true);
    REQUIRE_EQ(bs7.test(15), false);
    REQUIRE_EQ(bs7.test(500), false);
    REQUIRE_EQ(bs7.test(999), false);

    // Alternating resize
    bitset<32> bs8;
    for (fl::u32 i = 0; i < 32; i += 2) bs8.set(i, true);
    for (int round = 0; round < 3; ++round) {
        bs8.resize(64);
        for (fl::u32 i = 0; i < 32; i += 2) REQUIRE_EQ(bs8.test(i), true);
        for (fl::u32 i = 32; i < 64; i += 3) bs8.set(i, true);
        bs8.resize(32);
        for (fl::u32 i = 0; i < 32; i += 2) REQUIRE_EQ(bs8.test(i), true);
        for (fl::u32 i = 32; i < 64; i += 3) REQUIRE_EQ(bs8.test(i), false);
    }

    // Partial block edge cases
    bitset<16> bs9; bs9.set(15, true); bs9.resize(17); REQUIRE_EQ(bs9.test(15), true); bs9.set(16, true); REQUIRE_EQ(bs9.test(16), true);
    bitset<16> bs10; bs10.set(15, true); bs10.resize(24); REQUIRE_EQ(bs10.test(15), true); bs10.set(16, true); bs10.set(23, true); REQUIRE_EQ(bs10.test(16), true); REQUIRE_EQ(bs10.test(23), true);
    bitset<16> bs11; bs11.set(15, true); bs11.resize(31); REQUIRE_EQ(bs11.test(15), true); bs11.set(16, true); bs11.set(30, true); REQUIRE_EQ(bs11.test(16), true); REQUIRE_EQ(bs11.test(30), true);

    // Memcopy verification
    bitset<32> bs12; for (fl::u32 i = 0; i < 32; ++i) bs12.set(i, (i % 3) == 0);
    bs12.resize(64);
    for (fl::u32 i = 0; i < 32; ++i) REQUIRE_EQ(bs12.test(i), ((i % 3) == 0));
    for (fl::u32 i = 32; i < 64; ++i) bs12.set(i, (i % 5) == 0);
    bs12.resize(32);
    for (fl::u32 i = 0; i < 32; ++i) REQUIRE_EQ(bs12.test(i), ((i % 3) == 0));

    // Stress test
    bitset<16> bs13;
    for (int i = 0; i < 20; ++i) {
        fl::u32 new_size = 16 + (i % 20);
        bs13.resize(new_size);
        bs13.set(0, true);
        if (new_size > 1) bs13.set(new_size - 1, true);
        bs13.resize(16);
        REQUIRE_EQ(bs13.test(0), true);
    }
}
// --- End of folded tests ---
