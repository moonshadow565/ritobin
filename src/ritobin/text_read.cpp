#include <cstring>
#include <type_traits>
#include <charconv>
#include <optional>
#include <utility>
#include <vector>
#include "bin.hpp"

namespace ritobin {
    template<char...C>
    inline constexpr bool one_of(char c) noexcept {
        return ((c == C) || ...);
    }

    template<char F, char T>
    inline constexpr bool in_range(char c) noexcept {
        return c >= F && c <= T;
    }

    struct WordIter {
        char const* const beg_ = nullptr;
        char const* cur_ = nullptr;
        char const* const cap_ = nullptr;

        inline constexpr bool is_eof() const noexcept {
            return cur_ == cap_;
        }

        inline constexpr size_t position() const noexcept{
            return cur_ - beg_;
        }

        constexpr bool next_newline() noexcept {
            bool comment = false;
            bool newline = false;
            while (!is_eof()) {
                if (one_of<' ', '\t', '\r'>(*cur_)) {
                    cur_++;
                } else if (*cur_ == '\n') {
                    comment = false;
                    newline = true;
                    cur_++;
                } else if (*cur_ == '#') {
                    comment = true;
                    cur_++;
                } else if (comment) {
                    cur_++;
                } else {
                    break;
                }
            }
            return newline;
        }


        constexpr std::string_view read_word() noexcept {
            while (!is_eof() && one_of<' ', '\t', '\r'>(*cur_)) {
                cur_++;
            }
            auto const beg = cur_;
            while (!is_eof() && (one_of<'_', '+', '-', '.'>(*cur_)
                || in_range<'A', 'Z'>(*cur_)
                || in_range<'a', 'z'>(*cur_)
                || in_range<'0', '9'>(*cur_))) {
                cur_++;
            }
            return { beg, static_cast<size_t>(cur_ - beg) };
        }
    
        constexpr bool read_nested_begin(bool& end) noexcept {
            if (read<'{'>()) {
                next_newline();
                end = read<'}'>();
                return true;
            }
            return false;
        }

        constexpr bool read_nested_separator() noexcept {
            if (next_newline()) {
                return true;
            }

            if (read<','>()) {
                next_newline();
                return true;
            }
            return false;
        }

        constexpr bool read_nested_separator_or_end(bool& end) noexcept {
            if (read<'}'>()) {
                end = true;
                return true;
            }
            if (read_nested_separator()) {
                end = read<'}'>();
                return true;
            }
            return false;
        }
        
        bool read_hash(uint32_t& value) noexcept {
            auto const backup = cur_;
            auto const word = read_word();
            if (word.size() < 2) {
                cur_ = backup;
                return false;
            }
            if (word[0] != '0' || (word[1] != 'x' && word[1] != 'X')) {
                cur_ = backup;
                return false;
            }
            auto const beg = word.data() + 2;
            auto const end = word.data() + word.size();
            auto const [ptr, ec] = std::from_chars(beg, end, value, 16);
            if (ptr == end && ec == std::errc{}) {
                return true;
            }
            cur_ = backup;
            return false;
        }



        template<char Symbol>
        constexpr bool read() noexcept {
            while (!is_eof() && one_of<' ', '\t', '\r'>(*cur_)) {
                cur_++;
            }
            if (cur_ != cap_ && *cur_ == Symbol) {
                cur_++;
                return true;
            }
            return false;
        }

        bool read(std::string& result) noexcept {
            // FIXME: unicode verification
            while (!is_eof() && one_of<' ', '\t', '\r'>(*cur_)) {
                cur_++;
            }
            if (cur_ == cap_) {
                return false;
            }
            if (*cur_ != '"' && *cur_ != '\"') {
                return false;;
            }
            auto const term = *cur_;
            cur_++;
            result.clear();
            enum class State {
                Take,
                Escape,
                Escape_CariageReturn,
                Escape_Byte_0,
                Escape_Byte_1,
                Escape_Unicode_0,
                Escape_Unicode_1,
                Escape_Unicode_2,
                Escape_Unicode_3,
            } state = State::Take;
            char escaped[8];
            while (cur_ != cap_) {
                char const c = *cur_;
                if (state == State::Escape) {
                    if (c == term) {
                        state = State::Take;
                        result.push_back(term);
                    } else if (c == '\\') {
                        state = State::Take;
                        result.push_back('\\');
                    }  else if (c == 'b') {
                        state = State::Take;
                        result.push_back('\b');
                    } else if (c == 'f') {
                        state = State::Take;
                        result.push_back('\f');
                    } else if (c == 'n') {
                        state = State::Take;
                        result.push_back('\n');
                    } else if (c == 'r') {
                        state = State::Take;
                        result.push_back('\r');
                    } else if (c == 't') {
                        state = State::Take;
                        result.push_back('\t');
                    } else if (c == '\n') {
                        state = State::Take;
                        result.push_back('\n');
                    } else if (c == '\r') {
                        state = State::Escape_CariageReturn;
                    } else if (c == 'u') {
                        state = State::Escape_Unicode_0;
                    } else if (c == 'x') {
                        state = State::Escape_Byte_0;
                    } else {
                        return false;;
                    }
                } else if (state == State::Escape_CariageReturn) {
                    if (c == '\n') {
                        state = State::Take;
                        result.push_back('\n');
                    } else {
                        return false;;
                    }
                } else if (state == State::Escape_Byte_0) {
                    state = State::Escape_Byte_1;
                    escaped[0] = c;
                } else if (state == State::Escape_Byte_1) {
                    state = State::Take;
                    escaped[1] = c;
                    uint8_t value = 0;
                    auto [p, ec] = std::from_chars(escaped, escaped + 2, value, 16);
                    if (ec == std::errc{} && p == escaped + 2) {
                        result.push_back(static_cast<char>(value));
                    } else {
                        return false;;
                    }
                } else if (state == State::Escape_Unicode_0) {
                    state = State::Escape_Unicode_1;
                    escaped[0] = c;
                } else if (state == State::Escape_Unicode_1) {
                    state = State::Escape_Unicode_2;
                    escaped[1] = c;
                } else if (state == State::Escape_Unicode_2) {
                    state = State::Escape_Unicode_3;
                    escaped[2] = c;
                } else if (state == State::Escape_Unicode_3) {
                    state = State::Take;
                    escaped[3] = c;
                    uint16_t value = 0;
                    auto [p, ec] = std::from_chars(escaped, escaped + 4, value, 16);
                    if (ec == std::errc{} && p == escaped + 4) {
                        // FIXME: encode unicode
                        // result.push_back(static_cast<char>(value));
                    } else {
                        return false;;
                    }
                } else {
                    if (c == '\\') {
                        state = State::Escape;
                    } else if (c < ' ') {
                        return false;;
                    } else if (c == term) {
                        cur_++;
                        return true;
                    } else {
                        result.push_back(c);
                    }
                }
                cur_++;
            }
            return false;;
        }
    
        bool read(NameHash& value) {
            value = {};
            if (read_hash(value.value)) {
                return true;
            }
            auto const word = read_word();
            if (word.empty()) {
                return false;
            }
            if (!in_range<'a', 'z'>(word[0]) && !in_range<'A', 'Z'>(word[0])) {
                return false;
            }
            for (auto const& c : word) {
                if (c != '_'
                    && !in_range<'a', 'z'>(c)
                    && !in_range<'A', 'Z'>(c)
                    && !in_range<'0', '9'>(c)) {
                    return false;
                }
            }
            value.unhashed = word;
            return true;
        }

        bool read(StringHash& value) {
            value = {};
            if (read_hash(value.value)) {
                return true;
            }
            if (!read(value.unhashed)) {
                return false;
            }
            return true;
        }

        bool read(bool& value) noexcept {
            auto const word = read_word();
            if (word.empty()) {
                return false;
            }
            if (word == "true") {
                value = true;
                return true;
            } else if (word == "false") {
                value = false;
                return true;
            } else {
                return false;
            }
        }

        bool read(Type& value) noexcept {
            auto const name = read_word();
            if (name.empty()) {
                return false;
            }
            if (name == "none") { value = Type::NONE; }
            else if (name == "bool") { value = Type::BOOL; }
            else if (name == "i8") { value = Type::I8; }
            else if (name == "u8") { value = Type::U8; }
            else if (name == "i16") { value = Type::I16; }
            else if (name == "u16") { value = Type::U16; }
            else if (name == "i32") { value = Type::I32; }
            else if (name == "u32") { value = Type::U32; }
            else if (name == "i64") { value = Type::I64; }
            else if (name == "u64") { value = Type::U64; }
            else if (name == "f32") { value = Type::F32; }
            else if (name == "vec2") { value = Type::VEC2; }
            else if (name == "vec3") { value = Type::VEC3; }
            else if (name == "vec4") { value = Type::VEC4; }
            else if (name == "mtx44") { value = Type::MTX44; }
            else if (name == "rgba") { value = Type::RGBA; }
            else if (name == "string") { value = Type::STRING; }
            else if (name == "hash") { value = Type::HASH; }
            else if (name == "list") { value = Type::LIST; }
            else if (name == "pointer") { value = Type::POINTER; }
            else if (name == "embed") { value = Type::EMBED; }
            else if (name == "link") { value = Type::LINK; }
            else if (name == "option") { value = Type::OPTION; }
            else if (name == "map") { value = Type::MAP; }
            else if (name == "flag") { value = Type::FLAG; }
            else { value = Type::NONE; return false; }
            return true;
        }

        template<typename T>
        bool read(T& value) noexcept {
            static_assert (std::is_arithmetic_v<T>);
            auto const word = read_word();
            if (word.empty()) {
                return false;
            }
            auto const beg = word.data();
            auto const end = word.data() + word.size();
            auto const [ptr, ec] = std::from_chars(beg, end, value);
            if (ptr == end && ec == std::errc{}) {
                return true;
            }
            return false;
        }
    };

    struct ReadTextImpl {
        WordIter word_;
        std::vector<Node>& nodes;
        std::string& error;

        bool process() noexcept {
            nodes.clear();
            error.clear();
            if (!process_sections()) {
                error.append("Failed to read at: " + std::to_string(word_.position()));
                return false;
            }
            return true;
        }
    private:
        inline void handle_section(std::string name, Value value) noexcept {
            nodes.emplace_back(Section{ std::move(name), std::move(value) });
        }

        inline void handle_item(uint32_t index, Value value) noexcept {
            nodes.emplace_back(Item{ std::move(index), std::move(value) });
        }

        inline void handle_field(NameHash key, Value value) noexcept {
            nodes.emplace_back(Field{ std::move(key), std::move(value) });
        }

        inline void handle_pair(Value key, Value value) noexcept {
            nodes.emplace_back(Pair{ std::move(key), std::move(value) });
        }

        inline void handle_nested_end(Type type, uint32_t count) noexcept {
            nodes.emplace_back(NestedEnd{ std::move(type), std::move(count) });
        }

        inline bool fail_fast() noexcept {
            return false;
        }

        // NOTE: change this macro to include full stack messages
#define bin_assert(...) do { if(!(__VA_ARGS__)) { return fail_fast(); } } while(false)
        bool process_sections() noexcept {
            NameHash name = {};
            Value value = {};
            word_.next_newline();
            while (!word_.is_eof()) {
                bin_assert(word_.read(name) && !name.unhashed.empty());
                bin_assert(read_value_type(value));
                bin_assert(word_.read<'='>());
                bin_assert(read_value(value));
                handle_section(name.unhashed, value);
                bin_assert(process_value(value));
                bin_assert(word_.is_eof() || word_.read_nested_separator());
            }
            return true;
        }

        bool process_value_visit(List& value) noexcept {
            uint32_t counter = 0;
            auto item = Value_from_type(value.valueType);
            bin_assert(read_nested([&,this]() {
                bin_assert(read_value(item));
                handle_item(counter, item);
                bin_assert(process_value(item));
                return true;
                }, counter));
            handle_nested_end(Type::LIST, counter);
            return true;
        }

        bool process_value_visit(Option& value) noexcept {
            uint32_t counter = 0;
            auto item = Value_from_type(value.valueType);
            bin_assert(read_nested([&, this]() {
                bin_assert(counter == 0);
                bin_assert(read_value(item));
                handle_item(counter, item);
                bin_assert(process_value(item));
                return true;
                }, counter));
            handle_nested_end(Type::OPTION, counter);
            return true;
        }

        bool process_value_visit(Map& value) noexcept {
            uint32_t counter = 0;
            auto key = Value_from_type(value.keyType);
            auto item = Value_from_type(value.valueType);
            bin_assert(read_nested([&, this]() {
                bin_assert(read_value(key));
                bin_assert(word_.read<'='>());
                bin_assert(read_value(item));
                handle_pair(key, item);
                bin_assert(process_value(item));
                return true;
                }, counter));
            handle_nested_end(Type::MAP, counter);
            return true;
        }

        bool process_value_visit(Embed& value) noexcept {
            uint32_t counter = 0;
            NameHash name = {};
            Value item = {};
            bin_assert(read_nested([&, this]() {
                bin_assert(word_.read(name));
                bin_assert(read_value_type(item));
                bin_assert(word_.read<'='>());
                bin_assert(read_value(item));
                handle_field(name, item);
                bin_assert(process_value(item));
                return true;
                }, counter));
            handle_nested_end(Type::EMBED, counter);
            return true;
        }

        bool process_value_visit(Pointer& value) noexcept {
            if (value.value.value != 0 || !value.value.unhashed.empty()) {
                uint32_t counter = 0;
                NameHash name = {};
                Value item = {};
                bin_assert(read_nested([&, this]() {
                    bin_assert(word_.read(name));
                    bin_assert(read_value_type(item));
                    bin_assert(word_.read<'='>());
                    bin_assert(read_value(item));
                    handle_field(name, item);
                    bin_assert(process_value(item));
                    return true;
                    }, counter));
                handle_nested_end(Type::POINTER, counter);
            }
            return true;
        }

        template<typename T>
        bool process_value_visit(T&) noexcept {
            return true;
        }

        bool process_value(Value& value) noexcept {
            return std::visit([this](auto&& value) {
                return process_value_visit(value);
                }, value);
        }


        bool read_value_type(Value& value) {
            Type type = {};
            bin_assert(word_.read<':'>());
            bin_assert(word_.read(type));
            bin_assert(type <= Type::FLAG);

            if (type == Type::LIST) {
                List result = {};
                bin_assert(word_.read <'['>());
                bin_assert(word_.read(result.valueType));
                bin_assert(word_.read <']'>());
                value = result;
            } else if (type == Type::OPTION) {
                Option result = {};
                bin_assert(word_.read <'['>());
                bin_assert(word_.read(result.valueType));
                bin_assert(word_.read <']'>());
                value = result;
            } else if (type == Type::MAP) {
                Map result = {};
                bin_assert(word_.read <'['>());
                bin_assert(word_.read(result.keyType));
                bin_assert(word_.read <','>());
                bin_assert(word_.read(result.valueType));
                bin_assert(word_.read <']'>());
                value = result;
            } else {
                value = Value_from_type(type);
            }
            return true;
        }

        bool read_value_visit(String& value) noexcept {
            bin_assert(word_.read(value.value));
            return true;
        }

        bool read_value_visit(Map& value) noexcept {
            return true;
        }

        bool read_value_visit(List& value) noexcept {
            return true;
        }

        bool read_value_visit(Option& value) noexcept {
            return true;
        }

        bool read_value_visit(Embed& value) noexcept {
            bin_assert(word_.read(value.value));
            return true;
        }

        bool read_value_visit(Pointer& value) noexcept {
            bin_assert(word_.read(value.value));
            if (value.value.value == 0 && value.value.unhashed == "null") {
                value.value = {};
            }
            return true;
        }

        bool read_value_visit(None& value) noexcept {
            // FIXME: ???
            return true;
        }

        bool read_value_visit(Vec2& value) noexcept {
            return read_array(value.value);
        }

        bool read_value_visit(Vec3& value) noexcept {
            return read_array(value.value);
        }

        bool read_value_visit(Vec4& value) noexcept {
            return read_array(value.value);
        }

        bool read_value_visit(Mtx44& value) noexcept {
            return read_array(value.value);
        }

        bool read_value_visit(RGBA& value) noexcept {
            return read_array(value.value);
        }

        template<typename T>
        bool read_value_visit(T& value) noexcept {
            bin_assert(word_.read(value.value));
            return true;
        }

        bool read_value(Value& value) noexcept {
             return std::visit([this](auto&& value) {
                    return read_value_visit(value);
                }, value);
        }

        template<typename F>
        bool read_nested(F&& nested, uint32_t& counter) noexcept {
            bool end = false;
            counter = 0;
            bin_assert(word_.read_nested_begin(end));
            while (!end) {
                bin_assert(nested());
                bin_assert(word_.read_nested_separator_or_end(end));
                counter++;
            }
            return true;
        }

        template<typename T, size_t S>
        bool read_array(std::array<T, S>& value) noexcept {
            uint32_t counter = 0;
            bin_assert(read_nested([&, this]() {
                bin_assert(counter < S);
                bin_assert(word_.read(value[counter]));
                return true;
                }, counter));
            bin_assert(counter == static_cast<uint32_t>(S));
            return true;
        }
    };

    bool text_read(std::string_view data, std::vector<Node>& result, std::string& error) noexcept {
        ReadTextImpl reader{
            { data.data(), data.data(), data.data() + data.size() }, result, error
        };
        return reader.process();
    }
}
