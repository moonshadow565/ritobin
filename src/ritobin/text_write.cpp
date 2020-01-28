#include <stdexcept>
#include <charconv>
#include "bin.hpp"

namespace ritobin {
    struct TextWriter {
        std::vector<char>& buffer_;
        size_t ident_size_ = 2;
        size_t ident_ = {};

        inline void ident_inc() noexcept {
            ident_ += ident_size_;
        }

        inline void ident_dec() noexcept {
            ident_ -= ident_size_;
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
            char buffer[64] = {};
            auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
            buffer_.insert(buffer_.end(), buffer, ptr);
            // FIXME: this only works on msvc for now
            // alternatively use: https://github.com/ulfjack/ryu
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
            write_raw(ValueHelper::to_type_name(type));
        }

        void write(std::string_view str) noexcept {
            write_raw("\"");
            for (char c : str) {
                if (c == '\t') { write_raw("\\t"); } 
                else if (c == '\n') {  write_raw("\\n"); } 
                else if (c == '\r') {  write_raw("\\r"); }
                else if (c == '\b') {  write_raw("\\b"); }
                else if (c == '\f') {  write_raw("\\f"); }
                else if (c == '\\') { write_raw("\\\\"); }
                else if (c == '"') { write_raw("\""); }
                else if (c < 0x20 || c > 0x7E) {
                    constexpr char digits[] = "0123456789abcdef";
                    char hex[] = { 
                        '\\', 'x', 
                        digits[(c >> 4) & 0x0F],
                        digits[c & 0x0F],
                        '\0' 
                    };
                    write_raw(hex);
                }
                else { buffer_.push_back(c); }
            }
            write_raw("\"");
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
    };

    struct BinTextWriter {
        Bin const& bin;
        TextWriter writer;

        bool process() noexcept {
            writer.buffer_.clear();
            writer.write_raw("#PROP_text\n");
            write_sections();
            return true;
        }
    private:
        void write_sections() noexcept {
            for (auto const& section : bin.sections) {
                write_section(section);
            }
        }

        void write_section(std::pair<std::string_view,Value> const& section) {
            auto const& [name, value] = section;
            writer.write_raw(name);
            writer.write_raw(": ");
            write_type(value);
            writer.write_raw(" = ");
            write_value(value);
            writer.write_raw("\n");
        }

        void write_items(ElementList const& items) noexcept {
            if (items.empty()) {
                writer.write_raw("{}");
                return;
            }

            writer.write_raw("{\n");
            writer.ident_inc();
            for (auto const& [value] : items) {
                writer.pad();
                write_value(value);
                writer.write_raw("\n");
            }
            writer.ident_dec();
            writer.pad();
            writer.write_raw("}");
        }

        void write_items(PairList const& items) noexcept {
            if (items.empty()) {
                writer.write_raw("{}");
                return;
            }

            writer.write_raw("{\n");
            writer.ident_inc();
            for (auto const& [key, value] : items) {
                writer.pad();
                write_value(key);
                writer.write_raw(" = ");
                write_value(value);
                writer.write_raw("\n");
            }
            writer.ident_dec();
            writer.pad();
            writer.write_raw("}");
        }

        void write_items(FieldList const& items) noexcept {
            if (items.empty()) {
                writer.write_raw("{}");
                return;
            }

            writer.write_raw("{\n");
            writer.ident_inc();
            for (auto const& [key, value] : items) {
                writer.pad();
                writer.write_name(key);
                writer.write_raw(": ");
                write_type(value);
                writer.write_raw(" = ");
                write_value(value);
                writer.write_raw("\n");
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

        void write_value_visit(Option const& value) noexcept {
            write_items(value.items);
        }

        void write_value_visit(Map const& value) noexcept {
            write_items(value.items);
        }

        void write_value_visit(Hash const& value) noexcept {
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
    };

    void Bin::write_text(std::vector<char>& out, size_t ident_size) const {
        BinTextWriter writer = { *this, { out, ident_size } };
        writer.process();
    }
}
