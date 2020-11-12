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
    return true;
}

inline bool to_num(std::string_view str, double& num) noexcept {
    auto copy = std::string(str.begin(), str.end());
    return sscanf(copy.c_str(), "%lg", &num) == 1;
}

inline bool from_num(std::string& str, double const& num) noexcept {
    char buffer[64] = {};
    auto const size = sprintf(buffer, "%.17lg", num);
    str = std::string(buffer, buffer + size);
    return true;
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

inline bool to_num(std::string_view str, bool& num) noexcept {
    if (str.empty()) {
        num = false;
        return true;
    }
    if (double tmp = 0.0; to_num(str, tmp)) {
        num = tmp;
        return true;
    }
    return false;
}

inline bool from_num(std::string& str, bool const& num) noexcept {
    str = num ? "1" : "0";
    return true;
}


#endif // FLOATCONV_HPP
