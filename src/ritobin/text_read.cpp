#include <stdexcept>
#include <charconv>
#include "bin.hpp"


namespace ritobin {
    struct TextReader {
        char const* const beg_ = nullptr;
        char const* cur_ = nullptr;
        char const* const cap_ = nullptr;


        template<char...C>
        static inline constexpr bool one_of(char c) noexcept {
            return ((c == C) || ...);
        }

        template<char F, char T>
        static inline constexpr bool in_range(char c) noexcept {
            return c >= F && c <= T;
        }


        inline constexpr bool is_eof() const noexcept {
            return cur_ == cap_;
        }

        inline constexpr size_t position() const noexcept {
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

        bool read_string(std::string& result) noexcept {
            // FIXME: unicode verification
            while (!is_eof() && one_of<' ', '\t', '\r'>(*cur_)) {
                cur_++;
            }
            if (cur_ == cap_) {
                return false;
            }
            if (*cur_ != '"' && *cur_ != '\'') {
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
                    } else if (c == 'b') {
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
                        // FIXME: implement unicode code points
                        state = State::Escape_Unicode_0;
                        return false;
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
                    } else {
                        return false;;
                    }
                } else {
                    if (c == '\\') {
                        state = State::Escape;
                    }else if (c < ' ') {
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


        bool read_hash(FNV1a& value) noexcept {
            auto const word = read_word();
            if (word.size() < 2) {
                return false;
            }
            if (word[0] != '0' || (word[1] != 'x' && word[1] != 'X')) {
                return false;
            }
            auto const beg = word.data() + 2;
            auto const end = word.data() + word.size();
            uint32_t result = 0;
            auto const [ptr, ec] = std::from_chars(beg, end, result, 16);
            if (ptr == end && ec == std::errc{}) {
                value = { result };
                return true;
            }
            return false;
        }


        bool read_name(std::string& value) {
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
            value = { std::string(word) };
            return true;
        }

        bool read_hash_name(FNV1a& value) {
            auto const backup = cur_;
            if (read_hash(value)) {
                return true;
            }
            cur_ = backup;
            if (std::string str; read_name(str)) {
                value = { str };
                return true;
            }
            return false;
        }

        bool read_hash_string(FNV1a& value) {
            auto const backup = cur_;
            if (read_hash(value)) {
                return true;
            }
            cur_ = backup;
            if (std::string str; read_string(str)) {
                value = { str };
                return true;
            }
            return false;
        }

        bool read(bool& value) noexcept {
            auto const word = read_word();
            if (word.empty()) {
                return false;
            }
            if (word == "true") {
                value = true;
                return true;
            }
            else if (word == "false") {
                value = false;
                return true;
            }
            else {
                return false;
            }
        }

        bool read(Type& value) noexcept {
            auto const name = read_word();
            if (name.empty()) {
                return false;
            }
            return ValueHelper::from_type_name(name, value);
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
            // FIXME: this only works on msvc for now
            // alternatively use: https://github.com/ulfjack/ryu
            return false;
        }
    };

    struct BinTextReader {
        Bin& bin;
        TextReader reader;
        std::string error;

        bool process() noexcept {
            bin.sections.clear();
            error.clear();
            if (!read_sections()) {
                error.append("Failed to read @ " + std::to_string(reader.position()));
                return false;
            }
            return true;
        }

    private:
        // NOTE: change this macro to include full stack messages
#define bin_assert(...) do { if(!(__VA_ARGS__)) { return fail_fast(); } } while(false)
        inline constexpr bool fail_fast() const noexcept { return false; }

        bool read_sections() noexcept {
            reader.next_newline();
            while (!reader.is_eof()) {
                std::string name = {};
                Value value = {};
                bin_assert(reader.read_name(name));
                bin_assert(read_value_type(value));
                bin_assert(reader.read<'='>());
                bin_assert(read_value(value));
                bin_assert(reader.is_eof() || reader.read_nested_separator());
                bin.sections.emplace(std::move(name), std::move(value));
            }
            return true;
        }
    
        bool read_value_type(Value& value) {
            Type type = {};
            bin_assert(reader.read<':'>());
            bin_assert(reader.read(type));
            bin_assert(type <= Type::FLAG);

            if (type == Type::LIST) {
                List result = {};
                bin_assert(reader.read <'['>());
                bin_assert(reader.read(result.valueType));
                bin_assert(reader.read <']'>());
                value = result;
            } else if (type == Type::OPTION) {
                Option result = {};
                bin_assert(reader.read <'['>());
                bin_assert(reader.read(result.valueType));
                bin_assert(reader.read <']'>());
                value = result;
            } else if (type == Type::MAP) {
                Map result = {};
                bin_assert(reader.read <'['>());
                bin_assert(reader.read(result.keyType));
                bin_assert(reader.read <','>());
                bin_assert(reader.read(result.valueType));
                bin_assert(reader.read <']'>());
                value = result;
            } else {
                value = ValueHelper::from_type(type);
            }
            return true;
        }

        bool read_value(Value& value) noexcept {
            return std::visit([this](auto&& value) { return read_value_visit(value); }, value);
        }

        bool read_value_visit(List& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                auto item = ValueHelper::from_type(value.valueType);
                bin_assert(read_value(item));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Element{ std::move(item) });
            }
            return true;
        }

        bool read_value_visit(Option& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            if (!end) {
                auto item = ValueHelper::from_type(value.valueType);
                bin_assert(read_value(item));
                bin_assert(reader.read_nested_separator_or_end(end));
                bin_assert(end);
                value.items.emplace_back(Element{ std::move(item) });
            }
            return true;
        }

        bool read_value_visit(Map& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                auto key = ValueHelper::from_type(value.keyType);
                auto item = ValueHelper::from_type(value.valueType);
                bin_assert(read_value(key));
                bin_assert(reader.read<'='>());
                bin_assert(read_value(item));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Pair{ std::move(key), std::move(item) });
            }
            return true;
        }

        bool read_value_visit(Embed& value) noexcept {
            bin_assert(reader.read_hash_name(value.name));
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                FNV1a name = {};
                Value item = {};
                bin_assert(reader.read_hash_name(name));
                bin_assert(read_value_type(item));
                bin_assert(reader.read<'='>());
                bin_assert(read_value(item));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Field{ std::move(name), std::move(item) });
            }
            return true;
        }

        bool read_value_visit(Pointer& value) noexcept {
            bin_assert(reader.read_hash_name(value.name));
            if (value.name.str() == "null") {
                value.name = {};
                return true;
            }
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                FNV1a name = {};
                Value item = {};
                bin_assert(reader.read_hash_name(name));
                bin_assert(read_value_type(item));
                bin_assert(reader.read<'='>());
                bin_assert(read_value(item));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Field{ std::move(name), std::move(item) });
            }
            return true;
        }

        bool read_value_visit(String& value) noexcept {
            bin_assert(reader.read_string(value.value));
            return true;
        }

        bool read_value_visit(Link& value) noexcept {
            bin_assert(reader.read_hash_string(value.value));
            return true;
        }

        bool read_value_visit(Hash& value) noexcept {
            bin_assert(reader.read_hash_string(value.value));
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

        bool read_value_visit(None& value) noexcept {
            std::string name;
            bin_assert(reader.read_name(name));
            bin_assert(name == "null");
            return true;
        }

        template<typename T>
        bool read_value_visit(T& value) noexcept {
            bin_assert(reader.read(value.value));
            return true;
        }

        template<typename T, size_t S>
        bool read_array(std::array<T, S>& value) noexcept {
            uint32_t counter = 0;
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                bin_assert(counter < S);
                bin_assert(reader.read(value[counter]));
                bin_assert(reader.read_nested_separator_or_end(end));
                counter++;
            }
            bin_assert(counter == static_cast<uint32_t>(S));
            return true;
        }
    };

    void Bin::read_text(char const* data, size_t size) {
        BinTextReader reader = { *this, { data, data, data + size } };
        if (!reader.process()) {
            throw std::runtime_error(std::move(reader.error));
        }
    }
}