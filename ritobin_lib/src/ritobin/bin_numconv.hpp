#ifndef BIN_NUMCONV_HPP
#define BIN_NUMCONV_HPP
#include <charconv>
#include <string>
#include <type_traits>

#if !defined(__cpp_lib_to_chars) &&                                            \
    !(defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 200000) &&                \
    !(defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE >= 11) &&                  \
    !(defined(_MSC_VER) && _MSC_VER >= 1924)
#error                                                                         \
    "std::from_chars for floating-point types is unavailable: need libc++ 20+, libstdc++ 11+ (GCC 11), or MSVC 19.24 (VS 2019 16.4)."
#endif

namespace ritobin {
    template<typename T>
    inline bool to_num(std::string_view str, T& num, int base) noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            auto const [p, ec] = std::from_chars(str.data(), str.data() + str.size(), num);
            if (ec == std::errc{} && p == str.data() + str.size()) {
                return true;
            }
        } else {
            auto const [p, ec] = std::from_chars(str.data(), str.data() + str.size(), num, base);
            if (ec == std::errc{} && p == str.data() + str.size()) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    inline bool to_num(std::string_view str, T& num) noexcept {
        if constexpr (!std::is_floating_point_v<T>) {
            if constexpr (std::is_unsigned_v<T>) {
                if (str.starts_with("0x") || str.starts_with("0X")) {
                    return to_num(str.substr(2), num, 16);
                } else if (str.starts_with("0b") || str.starts_with("0B")) {
                    return to_num(str.substr(2), num, 2);
                } else if (str.starts_with("0o") || str.starts_with("0O")) {
                    return to_num(str.substr(2), num, 8);
                }
            }
        }
        return to_num(str, num, 10);
    }

    template<typename T>
    inline bool from_num(std::string& str, T const& num, int base = 10) noexcept {
        char buffer[256] = {};
        if constexpr (std::is_floating_point_v<T>) {
            auto const [p, ec] = std::to_chars(buffer, buffer + sizeof(buffer), num);
            if (ec == std::errc{}) {
                str = std::string(buffer, p);
                return true;
            }
        } else {
            auto const [p, ec] = std::to_chars(buffer, buffer + sizeof(buffer), num, base);
            if (ec == std::errc{}) {
                str = std::string(buffer, p);
                return true;
            }
        }
        return false;
    }

    extern bool to_num(std::string_view str, bool& num) noexcept;

    extern bool from_num(std::string& str, bool const& num) noexcept;
}

#endif // BIN_NUMCONV_HPP
