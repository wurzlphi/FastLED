// Test file to measure header reduction from lib8tion refactoring
// This file ONLY includes lib8tion.h to see how many headers it pulls in

#include "src/lib8tion.h"

int main() {
    // Just some basic lib8tion usage to ensure it compiles
    uint8_t a = 100;
    uint8_t b = 200;
    
    uint8_t sum = qadd8(a, b);  // Should be 255 (saturated)
    uint8_t scaled = scale8(sum, 128);  // Should be ~127
    
    return 0;
}
