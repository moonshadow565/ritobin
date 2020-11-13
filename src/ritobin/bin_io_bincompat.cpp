#include "bin_io.hpp"
#include "bin_types_helper.hpp"

namespace ritobin::io::compat_impl {
    static struct BinCompatLatest : BinCompat {
        char const* name() const noexcept override {
            return "bin";
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
    } compat_bin_latest = {};

    constinit static BinCompat const* bin_versions[] = {
        &compat_bin_latest
    };
}

namespace ritobin::io::dynamic_format_impl {
    using namespace compat_impl;

    template <size_t I>
    struct BinFormat : DynamicFormat {
        char const* name() const noexcept override {
            return bin_versions[I]->name();
        }
        bool output_allways_hashed() const noexcept override {
            return true;
        }
        std::string read(Bin &bin, std::span<const char> data) const override {
            return read_binary(bin, data, bin_versions[I]);
        }
        std::string write(const Bin &bin, std::vector<char> &data) const override {
            return write_binary(bin, data, bin_versions[I]);
        }
    };
    template <size_t I>
    static constinit auto bin_format = BinFormat<I>{};

    static struct TextFromat : DynamicFormat {
        char const* name() const noexcept override {
            return "text";
        }
        bool output_allways_hashed() const noexcept override {
            return false;
        }
        std::string read(Bin &bin, std::span<const char> data) const override {
            return read_text(bin, data);
        }
        std::string write(const Bin &bin, std::vector<char> &data) const override {
            return write_text(bin, data, 4);
        }
    } text_format = {};

    static constinit auto formats = []<size_t...I>(std::index_sequence<I...>) {
        return std::array {
            (DynamicFormat const*)&text_format,
            ((DynamicFormat const*)&bin_format<I>)...
        };
    } (std::make_index_sequence<sizeof(bin_versions) / sizeof(*bin_versions)>());
}

namespace ritobin::io {
    using namespace compat_impl;
    using namespace dynamic_format_impl;

    std::span<BinCompat const* const> BinCompat::list() noexcept {
        return bin_versions;
    }

    BinCompat const* BinCompat::get(std::string_view name) noexcept {
        for (auto version: bin_versions) {
            if (version->name() == name) {
                return version;
            }
        }
        return nullptr;
    }

    std::span<DynamicFormat const* const> DynamicFormat::list() noexcept {
        return formats;
    }

    DynamicFormat const* DynamicFormat::get(std::string_view name) noexcept {
        for (auto format: formats) {
            if (format->name() == name) {
                return format;
            }
        }
        return nullptr;
    }
}
