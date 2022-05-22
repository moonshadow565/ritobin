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

    static struct BinCompatLegacy : BinCompat {
        char const* name() const noexcept override {
            return "bin-legacy1";
        }
        bool type_to_raw(Type type, uint8_t &raw) const noexcept override {
            if (type == Type::LIST2) {
                type = Type::LIST;
            }
            return compat_bin_latest.type_to_raw(type, raw);
        }
        bool raw_to_type(uint8_t raw, Type& type) const noexcept override {
            if (raw >= 18 && raw < 0x80) {
                raw -= 18;
                raw |= 0x80;
            }
            if (raw >= 0x81) {
                raw += 1;
            }
            return compat_bin_latest.raw_to_type(raw, type);
        }
    } compat_bin_legacy1 = {};

    static BinCompat const* bin_versions[] = {
        &compat_bin_latest,
        &compat_bin_legacy1,
    };
}

namespace ritobin::io::dynamic_format_impl {
    using namespace compat_impl;

    template <size_t I>
    struct BinFormat : DynamicFormat {
        std::string_view name() const noexcept override {
            return bin_versions[I]->name();
        }
        std::string_view oposite_name() const noexcept override {
            return "text";
        }
        std::string_view default_extension() const noexcept override {
            return ".bin";
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
        bool try_guess(std::string_view data, std::string_view name) const noexcept override {
            if (data.starts_with("PTCH") || data.starts_with("PROP")) {
                return true;
            }
            if (name.ends_with(".bin")) {
                return true;
            }
            return false;
        }
    };
    template <size_t I>
    static auto bin_format = BinFormat<I>{};

    static struct TextFromat : DynamicFormat {
        std::string_view name() const noexcept override {
            return "text";
        }
        std::string_view oposite_name() const noexcept override {
            return "bin";
        }
        std::string_view default_extension() const noexcept override {
            return ".py";
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
        bool try_guess(std::string_view data, std::string_view name) const noexcept override {
            if (data.starts_with("#PROP_text") || data.starts_with("#PTCH_text")) {
                return true;
            }
            if (name.ends_with(".txt") || name.ends_with(".py")) {
                return true;
            }
            return false;
        }
    } text_format = {};

    static struct JsonFromat : DynamicFormat {
        std::string_view name() const noexcept override {
            return "json";
        }
        std::string_view oposite_name() const noexcept override {
            return "bin";
        }
        std::string_view default_extension() const noexcept override {
            return ".json";
        }
        bool output_allways_hashed() const noexcept override {
            return false;
        }
        std::string read(Bin &bin, std::span<const char> data) const override {
            return read_json(bin, data);
        }
        std::string write(const Bin &bin, std::vector<char> &data) const override {
            return write_json(bin, data, 2);
        }
        bool try_guess(std::string_view data, std::string_view name) const noexcept override {
            if (data.starts_with("{")) {
                return true;
            }
            if (name.ends_with(".json")) {
                return true;
            }
            return false;
        }
    } json_format = {};

    static struct InfoFromat : DynamicFormat {
        std::string_view name() const noexcept override {
            return "info";
        }
        std::string_view oposite_name() const noexcept override {
            return "";
        }
        std::string_view default_extension() const noexcept override {
            return ".json";
        }
        bool output_allways_hashed() const noexcept override {
            return false;
        }
        std::string read(Bin&, std::span<const char> data) const override {
            return "Json info files can't be read!";
        }
        std::string write(const Bin &bin, std::vector<char> &data) const override {
            return write_json_info(bin, data, 2);
        }
        bool try_guess(std::string_view, std::string_view) const noexcept override {
            return false;
        }
    } info_format = {};

    static auto formats = []<size_t...I>(std::index_sequence<I...>) consteval {
        return std::array {
            (DynamicFormat const*)&text_format,
            (DynamicFormat const*)&json_format,
            (DynamicFormat const*)&info_format,
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

    DynamicFormat const* DynamicFormat::guess(std::span<char const> data, std::string_view file_name) noexcept {
        for (auto format: formats) {
            if (format->try_guess(std::string_view{data.data(), data.size()}, file_name)) {
                return format;
            }
        }
        return nullptr;
    }
}
