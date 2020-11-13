#ifndef BIN_IO_HPP
#define BIN_IO_HPP

#include <span>
#include "bin_types.hpp"

namespace ritobin::io {
    struct BinCompat {
        virtual char const* name() const noexcept = 0;
        [[nodiscard]] virtual bool type_to_raw(Type type, uint8_t& raw) const noexcept = 0;
        [[nodiscard]] virtual bool raw_to_type(uint8_t raw, Type& type) const noexcept = 0;

        static std::span<BinCompat const* const> list() noexcept;
        static BinCompat const* get(std::string_view name) noexcept;
    };

    struct DynamicFormat {
        virtual char const* name() const noexcept = 0;
        virtual bool output_allways_hashed() const noexcept = 0;
        virtual std::string read(ritobin::Bin& bin, std::span<char const> data) const = 0;
        virtual std::string write(ritobin::Bin const& bin, std::vector<char>& data) const = 0;

        static std::span<DynamicFormat const* const> list() noexcept;
        static DynamicFormat const* get(std::string_view name) noexcept;
    };

    // Read .bin files
    extern std::string read_binary(Bin& value, std::span<char const> data, BinCompat const* compat) noexcept;
    // Write .bin files
    extern std::string write_binary(Bin const& value, std::vector<char>& out, BinCompat const* compat) noexcept;

    // Read .txt file
    extern std::string read_text(Bin& value, std::span<char const> data) noexcept;
    // Write .txt
    extern std::string write_text(Bin const& value, std::vector<char>& out, size_t indent_size = 2) noexcept;

    // Read single value
    extern std::string read_text(Value& value, std::span<char const> data) noexcept;
    // Read zero or more fields
    extern std::string read_text(FieldList& list, std::span<char const> data) noexcept;
    // Read zero or more elements
    extern std::string read_text(ElementList& list, Type valueType, std::span<char const> data) noexcept;
    // Read zero or more pairs
    extern std::string read_text(PairList& list, Type keyType, Type valueType, std::span<char const> data) noexcept;

    // Write single value
    extern std::string write_text(Value const& value, std::vector<char>& out, size_t indent_size = 2) noexcept;
    // Write zero or more fields
    extern std::string write_text(FieldList const& list, std::vector<char>& out, size_t indent_size = 2) noexcept;
    // Write zero or more elements
    extern std::string write_text(ElementList const& list, std::vector<char>& out, size_t indent_size = 2) noexcept;
    // Write zero or more pairs
    extern std::string write_text(PairList const& list, std::vector<char>& out, size_t indent_size = 2) noexcept;
}

#endif // BIN_IO_HPP
