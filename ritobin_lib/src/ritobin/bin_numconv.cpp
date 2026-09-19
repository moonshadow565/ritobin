#include "bin_numconv.hpp"

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
