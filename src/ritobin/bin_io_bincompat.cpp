#include "bin_io.hpp"
#include "bin_types_helper.hpp"

namespace ritobin::io::compat_impl {
    static struct VLatest : BinCompat {
        char const* name() const noexcept override {
            return "latest";
        }
        bool type_to_raw(Type type, uint8_t &raw) const noexcept override {
            raw = static_cast<uint8_t>(type);
            return true;
        }
        bool raw_to_type(uint8_t raw, Type &type) const noexcept override {
            type = static_cast<Type>(raw);
            if (ValueHelper::is_primitive(type)) {
                if (type <= ValueHelper::MAX_PRIMITIVE) {
                    return true;
                } else {
                    return false;
                }
            } else {
                if (type <= ValueHelper::MAX_COMPLEX) {
                    return true;
                } else {
                    return false;
                }
            }
        }
    } latest = {};

    static BinCompat const* const versions[] = {
        &latest
    };
}
namespace ritobin::io {
    using namespace compat_impl;

    BinCompat const* BinCompat::get(std::string_view name) noexcept {
        for (auto version: versions) {
            if (version->name() == name) {
                return version;
            }
        }
        return nullptr;
    }
}
