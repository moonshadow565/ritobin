#ifndef BIN_NUMCONV_HPP
#define BIN_NUMCONV_HPP
#include <charconv>
#include <string>

#ifndef MSVC
namespace ritobin {
    extern bool to_num(std::string_view str, float& num) noexcept;

    extern bool from_num(std::string& str, float const& num) noexcept;

    extern bool to_num(std::string_view str, double& num) noexcept;

    extern bool from_num(std::string& str, double const& num) noexcept;
}
#endif

namespace ritobin {
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

    extern bool to_num(std::string_view str, bool& num) noexcept;

    extern bool from_num(std::string& str, bool const& num) noexcept;
}

#endif // BIN_NUMCONV_HPP
