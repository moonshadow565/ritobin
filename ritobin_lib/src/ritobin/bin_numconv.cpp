#include "bin_numconv.hpp"

#ifndef MSVC
#include <cstdio>
namespace ritobin {
    bool to_num(std::string_view str, float& num) noexcept {
        auto copy = std::string(str.begin(), str.end());
        return sscanf(copy.c_str(), "%g", &num) == 1;
    }

    bool from_num(std::string& str, float const& num) noexcept {
        char buffer[64] = {};
        auto const size = sprintf(buffer, "%.9g", num);
        str = std::string(buffer, buffer + size);
        return true;
    }

    bool to_num(std::string_view str, double& num) noexcept {
        auto copy = std::string(str.begin(), str.end());
        return sscanf(copy.c_str(), "%lg", &num) == 1;
    }

    bool from_num(std::string& str, double const& num) noexcept {
        char buffer[64] = {};
        auto const size = sprintf(buffer, "%.17lg", num);
        str = std::string(buffer, buffer + size);
        return true;
    }
}
#endif

namespace ritobin {
    bool to_num(std::string_view str, bool& num) noexcept {
        if (str == "true") {
            num = true;
            return true;
        } else if (str == "false") {
            num = false;
            return true;
        } else if (str.empty()) {
            num = false;
            return false;
        } else {
            double tmp = 0.0;
            if (to_num(str, tmp)) {
                num = tmp;
                return true;
            } else {
                num = false;
                return false;
            }
        }
    }

    bool from_num(std::string& str, bool const& num) noexcept {
        str = num ? "true" : "false";
        return true;
    }
}
