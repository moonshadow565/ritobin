#include "bin_io.hpp"
#include "bin_types_helper.hpp"
#include "bin_numconv.hpp"
#include "bin_strconv.hpp"

namespace ritobin::io::text_write_impl {
    struct TextWriter {
        std::vector<char>& buffer_;
        size_t indent_size_ = 2;
        size_t ident_ = {};

        inline void ident_inc() noexcept {
            ident_ += indent_size_;
        }

        inline void ident_dec() noexcept {
            ident_ -= indent_size_;
        }

        void pad() {
            buffer_.insert(buffer_.end(), ident_, ' ');
        }

        void write_raw(std::string const& str) noexcept {
            buffer_.insert(buffer_.end(), str.cbegin(), str.cend());
        }

        void write_raw(std::string_view str) noexcept {
            buffer_.insert(buffer_.end(), str.cbegin(), str.cend());
        }

        template<size_t S>
        void write_raw(char const(&str)[S]) noexcept {
            buffer_.insert(buffer_.end(), str, str + S - 1);
        }

        template<typename T>
        void write(T value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            std::string result;
            from_num(result, value);
            buffer_.insert(buffer_.end(), result.begin(), result.end());
        }

        template<typename T, size_t SIZE>
        void write(std::array<T, SIZE> const& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            write_raw("{ ");
            for (size_t i = 0; i < (SIZE); i++) {
                write(value[i]);
                if (i < (SIZE - 1)) {
                    write_raw(", ");
                }
            }
            write_raw(" }");
        }

        void write(std::array<float, 16> const& value) noexcept {
            ident_inc();
            write_raw("{\n");
            pad();
            for (size_t i = 0; i < 16; i++) {
                write(value[i]);
                if (i % 4 == 3) {
                    write_raw("\n");
                    if (i == 15) {
                        ident_dec();
                    }
                    pad();
                } else {
                    write_raw(", ");
                }
            }
            write_raw("}");
        }

        void write(bool value) noexcept {
            if(value) {
                write_raw("true");
            } else {
                write_raw("false");
            }
        }
    
        void write(Type type) noexcept {
            write_raw(ValueHelper::type_to_type_name(type));
        }

        void write(std::string_view str) noexcept {
            str_quote(str, buffer_);
        }

        void write(std::string const& str) noexcept {
            write(std::string_view{str.data(), str.size()});
        }

        void write_hex(uint32_t hex) noexcept {
            constexpr char digits[] = "0123456789abcdef";
            char result[10] = { '0', 'x' };
            for (size_t i = 9; i > 1; i--) {
                result[i] = digits[hex & 0x0Fu];
                hex >>= 4;
            }
            write_raw(std::string_view{ result, sizeof(result) });
        }

        void write_hex(uint64_t hex) noexcept {
            constexpr char digits[] = "0123456789abcdef";
            char result[18] = { '0', 'x' };
            for (size_t i = 17; i > 1; i--) {
                result[i] = digits[hex & 0x0Fu];
                hex >>= 4;
            }
            write_raw(std::string_view{ result, sizeof(result) });
        }

        void write_name(FNV1a const& value) noexcept {
            if (!value.str().empty()) {
                write_raw(value.str());
            } else {
                write_hex(value.hash());
            }
        }

        void write_string(FNV1a const& value) noexcept {
            if (!value.str().empty()) {
                write(value.str());
            } else {
                write_hex(value.hash());
            }
        }

        void write_string(XXH64 const& value) noexcept {
            if (!value.str().empty()) {
                write(value.str());
            } else {
                write_hex(value.hash());
            }
        }
    };

    struct BinTextWriter {
        TextWriter writer;

        bool process_bin(Bin const& bin) noexcept {
            writer.buffer_.clear();
            writer.write_raw("#PROP_text\n");
            for (auto const& section : bin.sections) {
                write_section(section);
            }
            return true;
        }

        bool process_value(Value const& value) noexcept {
            write_value(value);
            return true;
        }

        template <typename T>
        bool process_list(std::span<T const> list) noexcept {
            for (auto const& item: list) {
                write_item(item);
            }
            return true;
        }
    private:
        void write_section(std::pair<std::string_view, Value> const& section) {
            writer.write_raw(section.first);
            writer.write_raw(": ");
            write_type(section.second);
            writer.write_raw(" = ");
            write_value(section.second);
            writer.write_raw("\n");
        }

        void write_item(Field const& item) {
            writer.pad();
            writer.write_name(item.key);
            writer.write_raw(": ");
            write_type(item.value);
            writer.write_raw(" = ");
            write_value(item.value);
            writer.write_raw("\n");
        }

        void write_item(Element const& item) {
            writer.pad();
            write_value(item.value);
            writer.write_raw("\n");
        }

        void write_item(Pair const& item) {
            writer.pad();
            write_value(item.key);
            writer.write_raw(" = ");
            write_value(item.value);
            writer.write_raw("\n");
        }

        template <typename T>
        void write_items(T const& items) noexcept {
            if (items.empty()) {
                writer.write_raw("{}");
                return;
            }
            writer.write_raw("{\n");
            writer.ident_inc();
            for (auto const& item : items) {
                write_item(item);
            }
            writer.ident_dec();
            writer.pad();
            writer.write_raw("}");
        }

        void write_type_visit(List const& value) noexcept {
            writer.write(value.type);
            writer.write_raw("[");
            writer.write(value.valueType);
            writer.write_raw("]");
        }

        void write_type_visit(List2 const& value) noexcept {
            writer.write(value.type);
            writer.write_raw("[");
            writer.write(value.valueType);
            writer.write_raw("]");
        }

        void write_type_visit(Option const& value) noexcept {
            writer.write(value.type);
            writer.write_raw("[");
            writer.write(value.valueType);
            writer.write_raw("]");
        }

        void write_type_visit(Map const& value) noexcept {
            writer.write(value.type);
            writer.write_raw("[");
            writer.write(value.keyType);
            writer.write_raw(",");
            writer.write(value.valueType);
            writer.write_raw("]");
        }

        void write_type_visit(None const& value) noexcept {
            writer.write(value.type);
        }

        template<typename T>
        void write_type_visit(T const& value) noexcept {
            writer.write(value.type);
        }

        void write_type(Value const& value) noexcept {
            std::visit([this](auto&& value) {
                write_type_visit(value);
            }, value);
        }
    
        void write_value_visit(Pointer const& value) noexcept {
            if (value.name.str().empty() && value.name.hash() == 0) {
                writer.write_raw("null");
                return;
            }
            writer.write_name(value.name);
            writer.write_raw(" ");
            write_items(value.items);
        }

        void write_value_visit(Embed const& value) noexcept {
            writer.write_name(value.name);
            writer.write_raw(" ");
            write_items(value.items);
        }

        void write_value_visit(List const& value) noexcept {
            write_items(value.items);
        }

        void write_value_visit(List2 const& value) noexcept {
            write_items(value.items);
        }

        void write_value_visit(Option const& value) noexcept {
            write_items(value.items);
        }

        void write_value_visit(Map const& value) noexcept {
            write_items(value.items);
        }

        void write_value_visit(Hash const& value) noexcept {
            writer.write_string(value.value);
        }

        void write_value_visit(File const& value) noexcept {
            writer.write_string(value.value);
        }

        void write_value_visit(Link const& value) noexcept {
            writer.write_string(value.value);
        }

        void write_value_visit(String const& value) noexcept {
            writer.write(value.value);
        }

        void write_value_visit(None const&) noexcept {
            writer.write_raw("null");
        }

        template<typename T>
        void write_value_visit(T const& value) noexcept {
            writer.write(value.value);
        }

        void write_value(Value const& value) noexcept {
            std::visit([this](auto&& value)  {
                write_value_visit(value);
            }, value);
        }

    public:
        std::string trace_error() {
            return "Something went wrong!";
        }
    };
}

namespace ritobin::io {
    using namespace text_write_impl;

    std::string write_text(Bin const& bin, std::vector<char>& out, size_t indent_size) noexcept {
        BinTextWriter writer = { { out, indent_size } };
        if (!writer.process_bin(bin)) {
            return writer.trace_error();
        }
        return {};
    }
}
