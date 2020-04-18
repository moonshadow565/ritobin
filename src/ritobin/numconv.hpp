#ifndef FLOATCONV_HPP
#define FLOATCONV_HPP
#include <charconv>
#include <cstdio>
#include <string>

#ifndef MSVC
inline bool to_num(std::string_view str, float& num) noexcept {
    auto copy = std::string(str.begin(), str.end());
    return sscanf(copy.c_str(), "%g", &num) == 1;
}

inline bool from_num(std::string& str, float const& num) noexcept {
    char buffer[64] = {};
    auto const size = sprintf(buffer, "%.9g", num);
    str = std::string(buffer, buffer + size);
    return false;
}
#endif

template<typename T>
inline bool to_num(std::string_view str, T& num, int base = 10) noexcept {
    auto const [p, ec] = std::from_chars(str.data(), str.data() + str.size(), num, base);
    if (ec == std::errc{} && p == str.data() + str.size()) {
        return true;
    }
    return false;
}

template<typename T>
inline bool from_num(std::string& str, T const& num, int base = 10) noexcept {
    char buffer[256] = {};
    auto const [p, ec] = std::to_chars(buffer, buffer + sizeof(buffer), num, base);
    if (ec == std::errc{}) {
        str = std::string(buffer, p);
        return true;
    }
    return false;
}


#endif // FLOATCONV_HPP
