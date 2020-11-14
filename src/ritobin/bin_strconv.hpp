#ifndef BIN_STRCONV_HPP
#define BIN_STRCONV_HPP

#include <vector>
#include <string>
#include <string_view>

namespace ritobin {
    extern char const* str_unquote_fetch_end(std::string_view data) noexcept;

    extern char const* str_unquote(std::string_view data, std::string& out) noexcept;

    extern char const* str_quote(std::string_view data, std::vector<char>& out) noexcept;
}

#endif // BIN_STRCONV_HPP
