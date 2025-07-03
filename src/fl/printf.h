#pragma once

#include "fl/strstream.h"
#include "fl/namespace.h"
#include "fl/type_traits.h"
#include "fl/str.h"
#include "fl/io.h"  // For fl::print and fl::println

namespace fl {

namespace printf_detail {

// Helper to parse format specifiers and extract precision
struct FormatSpec {
    char type = '\0';          // Format character (d, f, s, etc.)
    int precision = -1;        // Precision for floating point
    bool uppercase = false;    // For hex formatting
    
    FormatSpec() = default;
    explicit FormatSpec(char t) : type(t) {}
    FormatSpec(char t, int prec) : type(t), precision(prec) {}
};

// Parse a format specifier from the format string
// Returns the format spec and advances the pointer past the specifier
FormatSpec parse_format_spec(const char*& format);

// Format floating point with specified precision
fl::string format_float(float value, int precision);

// Base case: no more arguments
void format_impl(StrStream& stream, const char* format);

// Template functions are implemented in the .cpp.hpp file
// These declarations are needed for the template instantiation
template<typename T, typename... Args>
void format_impl(StrStream& stream, const char* format, const T& first, const Args&... rest);

}

/// @brief Printf-like formatting function that prints directly to the platform output
/// @param format Format string with placeholders like "%d", "%s", "%f" etc.
/// @param args Arguments to format
/// 
/// Supported format specifiers:
/// - %d, %i: integers (all integral types)
/// - %u: unsigned integers  
/// - %f: floating point numbers
/// - %s: strings (const char*, fl::string)
/// - %c: characters
/// - %x: hexadecimal (lowercase)
/// - %X: hexadecimal (uppercase)
/// - %%: literal % character
///
/// Example usage:
/// @code
/// fl::printf("Value: %d, Name: %s", 42, "test");
/// fl::printf("Float: %.2f", 3.14159);
/// @endcode
template<typename... Args>
void printf(const char* format, const Args&... args) {
    StrStream stream;
    printf_detail::format_impl(stream, format, args...);
    fl::print(stream.str().c_str());
}

/// @brief Printf-like formatting function that prints directly to the platform output with newline
/// @param format Format string with placeholders
/// @param args Arguments to format
template<typename... Args>
void printfln(const char* format, const Args&... args) {
    StrStream stream;
    printf_detail::format_impl(stream, format, args...);
    fl::println(stream.str().c_str());
}

/// @brief Printf-like formatting function that outputs directly to a StrStream
/// @param stream Output StrStream to write formatted result to
/// @param format Format string with placeholders
/// @param args Arguments to format
template<typename... Args>
void sprintf(StrStream& stream, const char* format, const Args&... args) {
    printf_detail::format_impl(stream, format, args...);
}

/// @brief Printf-like formatting function that returns a formatted string
/// @param format Format string with placeholders
/// @param args Arguments to format
/// @return Formatted string as fl::string
template<typename... Args>
fl::string sprintf_str(const char* format, const Args&... args) {
    StrStream stream;
    printf_detail::format_impl(stream, format, args...);
    return stream.str();
}

} // namespace fl

// Include template implementations
#include "fl/printf.cpp.hpp"
