#include "doctest.h"
#include "fl/printf.h"
#include "fl/strstream.h"

TEST_CASE("fl::printf basic functionality") {
    SUBCASE("simple string formatting") {
        fl::string result = fl::printf("Hello, %s!", "world");
        REQUIRE_EQ(result, fl::string("Hello, world!"));
    }
    
    SUBCASE("integer formatting") {
        fl::string result = fl::printf("Value: %d", 42);
        REQUIRE_EQ(result, fl::string("Value: 42"));
    }
    
    SUBCASE("multiple arguments") {
        fl::string result = fl::printf("Name: %s, Age: %d", "Alice", 25);
        REQUIRE_EQ(result, fl::string("Name: Alice, Age: 25"));
    }
    
    SUBCASE("floating point") {
        fl::string result = fl::printf("Pi: %f", 3.14159f);
        // Check that it contains expected parts
        REQUIRE(result.find("3.14") != fl::string::npos);
    }
    
    SUBCASE("floating point with precision") {
        fl::string result = fl::printf("Pi: %.2f", 3.14159f);
        REQUIRE_EQ(result, fl::string("Pi: 3.14"));
    }
    
    SUBCASE("character formatting") {
        fl::string result = fl::printf("Letter: %c", 'A');
        REQUIRE_EQ(result, fl::string("Letter: A"));
    }
    
    SUBCASE("hexadecimal formatting") {
        fl::string result = fl::printf("Hex: %x", 255);
        REQUIRE_EQ(result, fl::string("Hex: ff"));
    }
    
    SUBCASE("uppercase hexadecimal") {
        fl::string result = fl::printf("HEX: %X", 255);
        REQUIRE_EQ(result, fl::string("HEX: FF"));
    }
    
    SUBCASE("literal percent") {
        fl::string result = fl::printf("50%% complete");
        REQUIRE_EQ(result, fl::string("50% complete"));
    }
    
    SUBCASE("unsigned integers") {
        fl::string result = fl::printf("Unsigned: %u", 4294967295U);
        REQUIRE_EQ(result, fl::string("Unsigned: 4294967295"));
    }
}

TEST_CASE("fl::sprintf functionality") {
    SUBCASE("sprintf to StrStream") {
        fl::StrStream stream;
        fl::sprintf(stream, "Hello, %s! Value: %d", "world", 123);
        REQUIRE_EQ(stream.str(), fl::string("Hello, world! Value: 123"));
    }
    
    SUBCASE("sprintf multiple calls") {
        fl::StrStream stream;
        stream << "Prefix: ";
        fl::sprintf(stream, "Number: %d", 42);
        stream << " Suffix";
        REQUIRE_EQ(stream.str(), fl::string("Prefix: Number: 42 Suffix"));
    }
}

TEST_CASE("fl::printf edge cases") {
    SUBCASE("empty format string") {
        fl::string result = fl::printf("");
        REQUIRE_EQ(result, fl::string(""));
    }
    
    SUBCASE("no arguments") {
        fl::string result = fl::printf("No placeholders here");
        REQUIRE_EQ(result, fl::string("No placeholders here"));
    }
    
    SUBCASE("missing arguments") {
        fl::string result = fl::printf("Value: %d");
        REQUIRE(result.find("<missing_arg>") != fl::string::npos);
    }
    
    SUBCASE("extra arguments") {
        // Extra arguments should be ignored
        fl::string result = fl::printf("Value: %d", 42, 99);
        REQUIRE_EQ(result, fl::string("Value: 42"));
    }
    
    SUBCASE("zero values") {
        fl::string result = fl::printf("Zero: %d, Hex: %x", 0, 0);
        REQUIRE_EQ(result, fl::string("Zero: 0, Hex: 0"));
    }
}
