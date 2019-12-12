#include <sstream>
#include <stack>
#include <vector>
#include "bin.hpp"

namespace ritobin {
    struct WordInserter {
        std::vector<char>& buffer_;
        int ident_ = {};
        void pad() {
            for (int i = 0; i < ident_; i++) {
                write_raw("  ");
            }
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
            if constexpr (sizeof(T) < 4 && std::is_unsigned_v<T>) {
                write_raw(std::to_string(uint32_t(value)));
            } else if constexpr (sizeof(T) < 4 && std::is_signed_v<T>) {
                write_raw(std::to_string(int32_t(value)));
            } else {
                write_raw(std::to_string(value));
            }
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
            ident_++;
            write_raw("{\n");
            for (size_t i = 0; i < 16; i++) {
                write(value[i]);
                if (i % 4 == 3) {
                    write_raw("\n");
                    if (i == 15) {
                        ident_--;
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
            switch (type) {
            case Type::NONE: write_raw("none"); break;
            case Type::BOOL: write_raw("bool"); break;
            case Type::I8: write_raw("i8"); break;
            case Type::U8: write_raw("u8"); break;
            case Type::I16: write_raw("i16"); break;
            case Type::U16: write_raw("u16"); break;
            case Type::I32: write_raw("i32"); break;
            case Type::U32: write_raw("u32"); break;
            case Type::I64: write_raw("i64"); break;
            case Type::U64: write_raw("u64"); break;
            case Type::F32: write_raw("f32"); break;
            case Type::VEC2: write_raw("vec2"); break;
            case Type::VEC3: write_raw("vec3"); break;
            case Type::VEC4: write_raw("vec4"); break;
            case Type::MTX44: write_raw("mtx44"); break;
            case Type::RGBA: write_raw("rgba"); break;
            case Type::STRING: write_raw("string"); break;
            case Type::HASH: write_raw("hash"); break;
            case Type::LIST: write_raw("list"); break;
            case Type::POINTER: write_raw("pointer"); break;
            case Type::EMBED: write_raw("embed"); break;
            case Type::LINK: write_raw("link"); break;
            case Type::OPTION: write_raw("option"); break;
            case Type::MAP: write_raw("map"); break;
            case Type::FLAG: write_raw("flag"); break;
            }
        }

        void write(std::string const& str) noexcept {
            write_raw("\"");
            write_raw(str);
            write_raw("\"");
        }

        void write_hash(uint32_t hex) noexcept {
            constexpr char digits[] = "0123456789abcdef";
            char result[10] = { '0', 'x' };
            for (size_t i = 9; i > 1; i--) {
                result[i] = digits[hex & 0x0Fu];
                hex >>= 4;
            }
            write_raw(std::string_view{ result, sizeof(result) });
        }

        void write(StringHash const& value) noexcept {
            if (value.unhashed.empty()) {
                write_hash(value.value);
            } else {
                write(value.unhashed);
            }
        }

        void write(NameHash const& value) noexcept {
            if (value.unhashed.empty()) {
                write_hash(value.value);
            } else {
                write_raw(value.unhashed);
            }
        }
    };

    struct WriteTextImpl  {
        std::vector<Node> const& nodes_;
        std::string& error_;
        WordInserter word_;

        bool process() noexcept {
            error_.clear();
            word_.buffer_.clear();
            word_.ident_ = 0;
            word_.write_raw("#PROP_text\n");
            return handle_nodes();
        }
    private:
        bool handle_nodes() noexcept {
            for (auto const& node : nodes_) {
                std::visit([this](auto&& node) -> void {
                    handle_node_visit(std::forward<decltype(node)>(node));
                }, node);
            }
            word_.write_raw("\n");
            return true;
        }

        void handle_node_visit(Section const& section) noexcept {
            auto const& [name, value] = section;
            word_.write_raw("\n");
            word_.pad();
            word_.write_raw(name);
            word_.write_raw(": ");
            write_type(value);
            word_.write_raw(" = ");
            write_value(value);
        }

        void handle_node_visit(Item const& item) noexcept {
            auto const& [index, value] = item;
            word_.write_raw("\n");
            word_.pad();
            write_value(value);
        }

        void handle_node_visit(Field const& field) noexcept {
            auto const& [key, value] = field;
            word_.write_raw("\n");
            word_.pad();
            word_.write(key);
            word_.write_raw(": ");
            write_type(value);
            word_.write_raw(" = ");
            write_value(value);
        }

        void handle_node_visit(Pair const& pair) noexcept {
            auto const& [key, value] = pair;
            word_.write_raw("\n");
            word_.pad();
            write_value(key);
            word_.write_raw(" = ");
            write_value(value);
        }

        void handle_node_visit(NestedEnd const& end) noexcept {
            auto const [type, count] = end;
            word_.ident_--;
            if (count > 0) {
                word_.write_raw("\n");
                word_.pad();
            }
            word_.write_raw("}");
        }



        void write_type_visit(List const& value) noexcept {
            word_.write(value.type);
            word_.write_raw("[");
            word_.write(value.valueType);
            word_.write_raw("]");
        }

        void write_type_visit(Option const& value) noexcept {
            word_.write(value.type);
            word_.write_raw("[");
            word_.write(value.valueType);
            word_.write_raw("]");
        }

        void write_type_visit(Map const& value) noexcept {
            word_.write(value.type);
            word_.write_raw("[");
            word_.write(value.keyType);
            word_.write_raw(",");
            word_.write(value.valueType);
            word_.write_raw("]");
        }

        void write_type_visit(None const& value) noexcept {
            word_.write(value.type);
        }

        template<typename T>
        void write_type_visit(T const& value) noexcept {
            word_.write(value.type);
        }

        void write_type(Value const& value) noexcept {
            std::visit([this](auto&& value) {
                write_type_visit(value);
            }, value);
        }
        
        void write_value_visit(Pointer const& value) noexcept {
            if (value.value.value == 0 && value.value.unhashed.empty()) {
                word_.write_raw("null");
            } else {
                word_.write(value.value);
                word_.write_raw(" {");
                word_.ident_++;
            }
        }

        void write_value_visit(Embed const& value) noexcept {
            word_.write(value.value);
            word_.write_raw(" {");
            word_.ident_++;
        }

        void write_value_visit(List const&) noexcept {
            word_.write_raw("{");
            word_.ident_++;
        }

        void write_value_visit(Option const&) noexcept {
            word_.write_raw("{");
            word_.ident_++;
        }

        void write_value_visit(Map const&) noexcept {
            word_.write_raw("{");
            word_.ident_++;
        }

        void write_value_visit(Hash const& value) noexcept {
            word_.write(value.value);
        }

        void write_value_visit(Link const& value) noexcept {
            word_.write(value.value);
        }

        void write_value_visit(String const& value) noexcept {
            word_.write(value.value);
        }

        void write_value_visit(None const&) noexcept {
            word_.write_raw("null");
        }

        template<typename T>
        void write_value_visit(T const& value) noexcept {
            word_.write(value.value);
        }

        void write_value(Value const& value) noexcept {
            std::visit([this](auto&& value)  {
                write_value_visit(value);
            }, value);
        }
    };

    bool text_write(std::vector<Node> const& nodes, std::vector<char>& result, std::string& error) noexcept {
        WriteTextImpl writer = {
            nodes, error, { result }
        };
        return writer.process();
    }
}
