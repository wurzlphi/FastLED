#include <iostream>
#include <string>

// Include doctest to run all tests
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

int main(int argc, char* argv[]) {
    std::cout << "FastLED Test Runner - Running all tests from single DLL" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    // doctest will automatically discover and run all tests
    // that are compiled into the DLL
    return doctest::Context(argc, argv).run();
}
