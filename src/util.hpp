#pragma once
#include <string>
#include <algorithm>
#include <functional>

// see: http://en.cppreference.com/w/cpp/string/char_traits
struct ci_char_traits : public std::char_traits<char> {
    static bool eq(char c1, char c2) {
         return std::toupper(c1) == std::toupper(c2);
     }
    static bool lt(char c1, char c2) {
         return std::toupper(c1) <  std::toupper(c2);
    }
    static int compare(const char* s1, const char* s2, size_t n) {
        while ( n-- != 0 ) {
            if ( std::toupper(*s1) < std::toupper(*s2) ) return -1;
            if ( std::toupper(*s1) > std::toupper(*s2) ) return 1;
            ++s1; ++s2;
        }
        return 0;
    }
    static const char* find(const char* s, int n, char a) {
        auto const ua (std::toupper(a));
        while ( n-- != 0 ) 
        {
            if (std::toupper(*s) == ua)
                return s;
            s++;
        }
        return nullptr;
    }
};
typedef std::basic_string<char, ci_char_traits> ci_string;

inline int hex2int(char ch) {
    if ('0' <= ch && ch <= '9') return ch - '0';
    if ('a' <= ch && ch <= 'f') return ch - 'a' + 10;
    if ('A' <= ch && ch <= 'F') return ch - 'A' + 10;
    return -1;
}
