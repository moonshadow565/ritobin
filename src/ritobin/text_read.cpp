#include <stdexcept>
#include "bin.hpp"
#include "numconv.hpp"


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
            return static_cast<size_t>(cur_ - beg_);
        }

        template<char Symbol>
        constexpr bool read_symbol() noexcept {
            while (!is_eof() && one_of<' ', '\t', '\r'>(*cur_)) {
                cur_++;
            }
            if (cur_ != cap_ && *cur_ == Symbol) {
                cur_++;
                return true;
            }
            return false;
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
            if (read_symbol<'{'>()) {
                next_newline();
                end = read_symbol<'}'>();
                return true;
            }
            return false;
        }

        constexpr bool read_nested_separator() noexcept {
            if (next_newline()) {
                return true;
            }

            if (read_symbol<','>()) {
                next_newline();
                return true;
            }
            return false;
        }

        constexpr bool read_nested_separator_or_end(bool& end) noexcept {
            if (read_symbol<'}'>()) {
                end = true;
                return true;
            }
            if (read_nested_separator()) {
                end = read_symbol<'}'>();
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
                return false;
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
                        return false;
                    } else if (c == 'x') {
                        state = State::Escape_Byte_0;
                    } else {
                        return false;
                    }
                } else if (state == State::Escape_CariageReturn) {
                    if (c == '\n') {
                        state = State::Take;
                        result.push_back('\n');
                    } else {
                        return false;
                    }
                } else if (state == State::Escape_Byte_0) {
                    state = State::Escape_Byte_1;
                    escaped[0] = c;
                } else if (state == State::Escape_Byte_1) {
                    state = State::Take;
                    escaped[1] = c;
                    uint8_t value = 0;
                    if (to_num({escaped, 2}, value, 16)) {
                        result.push_back(static_cast<char>(value));
                    } else {
                        return false;
                    }
                } else {
                    if (c == '\\') {
                        state = State::Escape;
                    } else if (c < ' ') {
                        return false;
                    } else if (c == term) {
                        cur_++;
                        return true;
                    } else {
                        result.push_back(c);
                    }
                }
                cur_++;
            }
            return false;
        }

        bool read_hash(FNV1a& value) noexcept {
            auto const word = read_word();
            if (word.size() < 2) {
                return false;
            }
            if (word[0] != '0' || (word[1] != 'x' && word[1] != 'X')) {
                return false;
            }
            uint32_t result = 0;
            if (to_num({word.data() + 2, word.size() - 2}, result, 16)) {
                value = FNV1a{ result };
                return true;
            }
            return false;
        }

        bool read_hash(XXH64& value) noexcept {
            auto const word = read_word();
            if (word.size() < 2) {
                return false;
            }
            if (word[0] != '0' || (word[1] != 'x' && word[1] != 'X')) {
                return false;
            }
            uint64_t result = 0;
            if (to_num({word.data() + 2, word.size() - 2}, result, 16)) {
                value = XXH64{ result };
                return true;
            }
            return false;
        }

        bool read_name(std::string& value) noexcept {
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

        bool read_hash_name(FNV1a& value) noexcept {
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

        bool read_hash_string(FNV1a& value) noexcept {
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

        bool read_hash_string(XXH64& value) noexcept {
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

        bool read_bool(bool& value) noexcept {
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

        bool read_typename(Type& value) noexcept {
            auto const name = read_word();
            if (name.empty()) {
                return false;
            }
            return ValueHelper::try_type_name_to_type(name, value);
        }

        template<typename T>
        bool read_number(T& value) noexcept {
            static_assert (std::is_arithmetic_v<T>);
            auto const word = read_word();
            if (word.empty()) {
                return false;
            }
            return to_num(word, value);
        }
    };

    struct BinTextReader {
        Bin& bin;
        TextReader reader;
        std::vector<std::pair<std::string, char const*>> error;

        #define bin_assert(...) do { \
            if(auto start = reader.cur_; !(__VA_ARGS__)) { \
                return fail_msg(#__VA_ARGS__, start); \
            } } while(false)

        bool process() noexcept {
            bin_assert(read_sections());
            return true;
        }
    private:
        bool fail_msg(char const* msg, char const* pos) noexcept {
            error.emplace_back(msg, pos);
            return false;
        }

        bool read_sections() noexcept {
            reader.next_newline();
            while (!reader.is_eof()) {
                std::string section_name = {};
                Value section_value = {};
                bin_assert(reader.read_name(section_name));
                bin_assert(read_value_type(section_value));
                bin_assert(reader.read_symbol<'='>());
                bin_assert(read_value(section_value));
                bin_assert(reader.is_eof() || reader.read_nested_separator());
                bin.sections.emplace(std::move(section_name), std::move(section_value));
            }
            return true;
        }
    
        bool read_value_type(Value& value) noexcept {
            Type type = {};
            bin_assert(reader.read_symbol<':'>());
            bin_assert(reader.read_typename(type));
            // TODO: do we need any checks on type here?

            if (type == Type::LIST) {
                List result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else if (type == Type::LIST2) {
                List2 result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else if (type == Type::OPTION) {
                Option result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else if (type == Type::MAP) {
                Map result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.keyType));
                bin_assert(reader.read_symbol<','>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else {
                value = ValueHelper::type_to_value(type);
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
                auto list_item = ValueHelper::type_to_value(value.valueType);
                bin_assert(read_value(list_item));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Element{ std::move(list_item) });
            }
            return true;
        }

        bool read_value_visit(List2& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                auto list_item = ValueHelper::type_to_value(value.valueType);
                bin_assert(read_value(list_item));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Element{ std::move(list_item) });
            }
            return true;
        }

        bool read_value_visit(Option& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            if (!end) {
                auto option_item = ValueHelper::type_to_value(value.valueType);
                bin_assert(read_value(option_item));
                bin_assert(reader.read_nested_separator_or_end(end));
                bin_assert(end);
                value.items.emplace_back(Element{ std::move(option_item) });
            }
            return true;
        }

        bool read_value_visit(Map& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                auto map_key = ValueHelper::type_to_value(value.keyType);
                auto map_value = ValueHelper::type_to_value(value.valueType);
                bin_assert(read_value(map_key));
                bin_assert(reader.read_symbol<'='>());
                bin_assert(read_value(map_value));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Pair{ std::move(map_key), std::move(map_value) });
            }
            return true;
        }

        bool read_value_visit(Embed& value) noexcept {
            bin_assert(reader.read_hash_name(value.name));
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                FNV1a field_name = {};
                Value field_value = {};
                bin_assert(reader.read_hash_name(field_name));
                bin_assert(read_value_type(field_value));
                bin_assert(reader.read_symbol<'='>());
                bin_assert(read_value(field_value));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Field{ std::move(field_name), std::move(field_value) });
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
                FNV1a field_name = {};
                Value field_value = {};
                bin_assert(reader.read_hash_name(field_name));
                bin_assert(read_value_type(field_value));
                bin_assert(reader.read_symbol<'='>());
                bin_assert(read_value(field_value));
                bin_assert(reader.read_nested_separator_or_end(end));
                value.items.emplace_back(Field{ std::move(field_name), std::move(field_value) });
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

        bool read_value_visit(File& value) noexcept {
            bin_assert(reader.read_hash_string(value.value));
            return true;
        }

        bool read_value_visit(Vec2& value) noexcept {
            bin_assert(read_array<float, 2>(value.value));
            return true;
        }

        bool read_value_visit(Vec3& value) noexcept {
            bin_assert(read_array<float, 3>(value.value));
            return true;
        }

        bool read_value_visit(Vec4& value) noexcept {
            bin_assert(read_array<float, 4>(value.value));
            return true;
        }

        bool read_value_visit(Mtx44& value) noexcept {
            bin_assert(read_array<float, 16>(value.value));
            return true;
        }

        bool read_value_visit(RGBA& value) noexcept {
            bin_assert(read_array<uint8_t, 4>(value.value));
            return true;
        }

        bool read_value_visit(None&) noexcept {
            std::string name;
            bin_assert(reader.read_name(name));
            bin_assert(name == "null");
            return true;
        }

        bool read_value_visit(Bool& value) noexcept {
            bin_assert(reader.read_bool(value.value));
            return true;
        }

        bool read_value_visit(Flag& value) noexcept {
            bin_assert(reader.read_bool(value.value));
            return true;
        }

        template<typename T>
        bool read_value_visit(T& value) noexcept {
            bin_assert(reader.read_number(value.value));
            return true;
        }

        template<typename T, size_t S>
        bool read_array(std::array<T, S>& value) noexcept {
            uint32_t counter = 0;
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                bin_assert(counter < S);
                bin_assert(reader.read_number(value[counter]));
                bin_assert(reader.read_nested_separator_or_end(end));
                counter++;
            }
            bin_assert(counter == static_cast<uint32_t>(S));
            return true;
        }
    };

    void Bin::read_text(char const* data, size_t size) {
        BinTextReader reader = { *this, { data, data, data + size }, {} };
        if (!reader.process()) {
            auto iter = data;
            size_t line_number = 1;
            auto line_start = data;
            auto get_column = [&iter, &line_number, &line_start](char const* end) noexcept {
                while(iter != end) {
                    if(*iter == '\n') {
                        line_start = iter;
                        ++iter;
                        ++line_number;
                    } else {
                        ++iter;
                    }
                }
                return end - line_start;
            };

            std::string error;
            for(auto e = reader.error.crbegin(); e != reader.error.crend(); e++) {
                error.append(e->first);
                error.append(" @ line: ");
                auto column_number = get_column(e->second);
                error.append(std::to_string(line_number));
                error.append(", column: ");
                error.append(std::to_string(column_number));
                error.append("\n");
            }
            error.append("Last position @ line: ");
            auto column_number = get_column(reader.reader.cur_);
            error.append(std::to_string(line_number));
            error.append(", column: ");
            error.append(std::to_string(column_number));
            error.append("\n");
            throw std::runtime_error(std::move(error));
        }
    }
}
