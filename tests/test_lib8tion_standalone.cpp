// Test program to verify lib8tion can be used standalone without FastLED.h

// Only include the new lib8tion headers - no FastLED.h!
#include "../src/lib8tion_base.h"
#include "../src/lib8tion_core.h"

// For printf
#include <stdio.h>

// Test that we're in the fl namespace
using namespace fl;

// The random functions need this seed variable
// In a real project, this would come from lib8tion.cpp
uint16_t rand16seed = 1337;

// Provide get_millisecond_timer for non-Arduino platforms
#ifndef FASTLED_LIB8TION_USE_MILLIS
extern "C" uint32_t get_millisecond_timer() {
    // Simple implementation for testing
    return 0;
}
#endif

int main() {
    printf("Testing standalone lib8tion functionality...\n\n");
    
    // Test basic math functions
    printf("Testing basic math:\n");
    uint8_t a = 100;
    uint8_t b = 200;
    
    // Test qadd8 (saturating add)
    uint8_t sum = qadd8(a, b);
    printf("qadd8(%d, %d) = %d (expected 255)\n", a, b, sum);
    
    // Test qsub8 (saturating subtract)
    uint8_t diff = qsub8(50, 100);
    printf("qsub8(50, 100) = %d (expected 0)\n", diff);
    
    // Test scale8
    uint8_t scaled = scale8(200, 128);
    printf("scale8(200, 128) = %d (expected ~100)\n", scaled);
    
    // Test random8
    printf("\nTesting random functions:\n");
    uint8_t r1 = random8();
    uint8_t r2 = random8(100);
    uint8_t r3 = random8(50, 150);
    printf("random8() = %d\n", r1);
    printf("random8(100) = %d (should be 0-99)\n", r2);
    printf("random8(50, 150) = %d (should be 50-149)\n", r3);
    
    // Test abs8
    printf("\nTesting utility functions:\n");
    int8_t negative = -50;
    uint8_t absolute = abs8(negative);
    printf("abs8(%d) = %d\n", negative, absolute);
    
    // Test mul8
    uint8_t product = mul8(100, 3);
    printf("mul8(100, 3) = %d (expected 44, since 300 & 0xFF = 44)\n", product);
    
    printf("\nAll tests completed!\n");
    printf("lib8tion works standalone without FastLED.h\n");
    
    return 0;
}
