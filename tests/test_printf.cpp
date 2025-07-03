#include "doctest.h"
#include "fl/printf.h"
#include "fl/strstream.h"
#include "fl/io.h"

// Test helper for capturing platform output
namespace test_helper {
    static fl::string captured_output;
    
    void capture_print(const char* str) {
        captured_output += str;
    }
    
    void capture_println(const char* str) {
        captured_output += str;
        captured_output += "\n";
    }
    
    void clear_capture() {
        captured_output.clear();
    }
    
    fl::string get_capture() {
        return captured_output;
    }
}

TEST_CASE("fl::sprintf_str basic test") {
    // Test sprintf_str which doesn't involve platform output
    SUBCASE("simple string test") {
        fl::string result = fl::sprintf_str("Hello, %s!", "world");
        REQUIRE(result.size() > 0);
        // Just check that it contains the basic parts
        REQUIRE(result.find("Hello") != fl::string::npos);
        REQUIRE(result.find("world") != fl::string::npos);
    }
    
    SUBCASE("integer test") {
        fl::string result = fl::sprintf_str("Value: %d", 42);
        REQUIRE(result.find("Value") != fl::string::npos);
        REQUIRE(result.find("42") != fl::string::npos);
    }
}

TEST_CASE("fl::printf basic functionality") {
    // Setup capture for testing platform output
    fl::inject_print_handler(test_helper::capture_print);
    fl::inject_println_handler(test_helper::capture_println);
    
    SUBCASE("simple string formatting") {
        test_helper::clear_capture();
        fl::printf("Hello, %s!", "world");
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Hello, world!");
        
        // Use basic string comparison
        REQUIRE_EQ(result.size(), expected.size());
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("integer formatting") {
        test_helper::clear_capture();
        fl::printf("Value: %d", 42);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Value: 42");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("multiple arguments") {
        test_helper::clear_capture();
        fl::printf("Name: %s, Age: %d", "Alice", 25);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Name: Alice, Age: 25");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("floating point") {
        test_helper::clear_capture();
        fl::printf("Pi: %f", 3.14159f);
        // Check that it contains expected parts
        fl::string result = test_helper::get_capture();
        REQUIRE(result.find("3.14") != fl::string::npos);
    }
    
    SUBCASE("floating point with precision") {
        test_helper::clear_capture();
        fl::printf("Pi: %.2f", 3.14159f);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Pi: 3.14");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("character formatting") {
        test_helper::clear_capture();
        fl::printf("Letter: %c", 'A');
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Letter: A");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("hexadecimal formatting") {
        test_helper::clear_capture();
        fl::printf("Hex: %x", 255);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Hex: ff");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("uppercase hexadecimal") {
        test_helper::clear_capture();
        fl::printf("HEX: %X", 255);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("HEX: FF");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("literal percent") {
        test_helper::clear_capture();
        fl::printf("50%% complete");
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("50% complete");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("unsigned integers") {
        test_helper::clear_capture();
        fl::printf("Unsigned: %u", 4294967295U);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Unsigned: 4294967295");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    // Cleanup
    fl::clear_io_handlers();
}

TEST_CASE("fl::printfln functionality") {
    // Setup capture for testing platform output
    fl::inject_print_handler(test_helper::capture_print);
    fl::inject_println_handler(test_helper::capture_println);
    
    SUBCASE("printfln adds newline") {
        test_helper::clear_capture();
        fl::printfln("Hello, %s!", "world");
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Hello, world!\n");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    // Cleanup
    fl::clear_io_handlers();
}

TEST_CASE("fl::sprintf functionality") {
    SUBCASE("sprintf to StrStream") {
        fl::StrStream stream;
        fl::sprintf(stream, "Hello, %s! Value: %d", "world", 123);
        fl::string result = stream.str();
        fl::string expected = fl::string("Hello, world! Value: 123");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("sprintf multiple calls") {
        fl::StrStream stream;
        stream << "Prefix: ";
        fl::sprintf(stream, "Number: %d", 42);
        stream << " Suffix";
        fl::string result = stream.str();
        fl::string expected = fl::string("Prefix: Number: 42 Suffix");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
}

TEST_CASE("fl::sprintf_str functionality") {
    SUBCASE("sprintf_str returns string") {
        fl::string result = fl::sprintf_str("Hello, %s! Value: %d", "world", 123);
        fl::string expected = fl::string("Hello, world! Value: 123");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("sprintf_str with precision") {
        fl::string result = fl::sprintf_str("Pi: %.2f", 3.14159f);
        fl::string expected = fl::string("Pi: 3.14");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
}

TEST_CASE("fl::printf edge cases") {
    // Setup capture for testing platform output
    fl::inject_print_handler(test_helper::capture_print);
    fl::inject_println_handler(test_helper::capture_println);
    
    SUBCASE("empty format string") {
        test_helper::clear_capture();
        fl::printf("");
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("no arguments") {
        test_helper::clear_capture();
        fl::printf("No placeholders here");
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("No placeholders here");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("missing arguments") {
        test_helper::clear_capture();
        fl::printf("Value: %d");
        fl::string result = test_helper::get_capture();
        REQUIRE(result.find("<missing_arg>") != fl::string::npos);
    }
    
    SUBCASE("extra arguments") {
        test_helper::clear_capture();
        // Extra arguments should be ignored
        fl::printf("Value: %d", 42, 99);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Value: 42");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    SUBCASE("zero values") {
        test_helper::clear_capture();
        fl::printf("Zero: %d, Hex: %x", 0, 0);
        fl::string result = test_helper::get_capture();
        fl::string expected = fl::string("Zero: 0, Hex: 0");
        REQUIRE_EQ(strcmp(result.c_str(), expected.c_str()), 0);
    }
    
    // Cleanup
    fl::clear_io_handlers();
}
