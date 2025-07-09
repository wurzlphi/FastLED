#include "src/fl/bitset.h"
#include <iostream>

using namespace fl;

int main() {
    // Test the exact scenario from the failing test
    bitset<64> bs;
    bs.set(0, true); 
    bs.set(15, true); 
    bs.set(31, true); 
    bs.set(47, true); 
    bs.set(63, true);
    
    std::cout << "Before resize to 128:" << std::endl;
    std::cout << "bs.test(0) = " << bs.test(0) << std::endl;
    std::cout << "bs.test(15) = " << bs.test(15) << std::endl;
    std::cout << "bs.test(31) = " << bs.test(31) << std::endl;
    std::cout << "bs.test(47) = " << bs.test(47) << std::endl;
    std::cout << "bs.test(63) = " << bs.test(63) << std::endl;
    
    bs.resize(128);
    std::cout << "After resize to 128:" << std::endl;
    std::cout << "bs.test(0) = " << bs.test(0) << std::endl;
    std::cout << "bs.test(15) = " << bs.test(15) << std::endl;
    std::cout << "bs.test(31) = " << bs.test(31) << std::endl;
    std::cout << "bs.test(47) = " << bs.test(47) << std::endl;
    std::cout << "bs.test(63) = " << bs.test(63) << std::endl;
    
    bs.set(100, true); 
    bs.set(127, true);
    
    std::cout << "After setting bits 100 and 127:" << std::endl;
    std::cout << "bs.test(100) = " << bs.test(100) << std::endl;
    std::cout << "bs.test(127) = " << bs.test(127) << std::endl;
    
    bs.resize(32);
    std::cout << "After resize to 32:" << std::endl;
    std::cout << "bs.test(0) = " << bs.test(0) << std::endl;
    std::cout << "bs.test(15) = " << bs.test(15) << std::endl;
    std::cout << "bs.test(31) = " << bs.test(31) << std::endl;
    std::cout << "bs.test(47) = " << bs.test(47) << std::endl;
    std::cout << "bs.test(63) = " << bs.test(63) << std::endl;
    std::cout << "bs.test(100) = " << bs.test(100) << std::endl;
    std::cout << "bs.test(127) = " << bs.test(127) << std::endl;
    
    return 0;
}
