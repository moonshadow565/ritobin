#include "bin_io.hpp"
#include "bin_types_helper.hpp"
#include "bin_numconv.hpp"
#include "bin_strconv.hpp"

#define bin_assert(...) do { \
    if(auto start = reader.cur_; !(__VA_ARGS__)) { \
        return fail_msg(#__VA_ARGS__, start); \
    } } while(false)

namespace ritobin::io::impl_text_read {
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

        constexpr bool read_nested_separator_or_eof() noexcept {
            if (is_eof()) {
                return true;
            }
            if (read_nested_separator()) {
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
            auto quote_end = str_unquote_fetch_end(std::string_view{cur_, (size_t)(cap_ - cur_)});
            if (quote_end == cap_) {
                return false;
            }
            result.clear();
            result.reserve(quote_end - cur_);
            ++cur_;
            cur_ = str_unquote(std::string_view{cur_, (size_t)(quote_end - cur_)}, result);
            if (cur_ != quote_end) {
                return false;
            }
            cur_++;
            return true;
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
        TextReader reader;
        std::vector<std::pair<std::string, char const*>> error;

        bool process_bin(Bin& bin) noexcept {
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

        bool process_value(Value& value) noexcept {
            reader.next_newline();
            bin_assert(read_value(value));
            return true;
        }

        bool process_list_fields(FieldList& list) noexcept {
            reader.next_newline();
            while (!reader.is_eof()) {
                bin_assert(read_field(list));
                bin_assert(reader.read_nested_separator_or_eof());
            }
            return true;
        }

        bool process_list_elements(ElementList& list, Type valueType) noexcept {
            reader.next_newline();
            while (!reader.is_eof()) {
                bin_assert(read_element(list, valueType));
                bin_assert(reader.read_nested_separator_or_eof());
            }
            return true;
        }

        bool process_list_pairs(PairList& list, Type keyType, Type valueType) noexcept {
            reader.next_newline();
            while (!reader.is_eof()) {
                bin_assert(read_pair(list, keyType, valueType));
                bin_assert(reader.read_nested_separator_or_eof());
            }
            return true;
        }
    private:
        bool fail_msg(char const* msg, char const* pos) noexcept {
            error.emplace_back(msg, pos);
            return false;
        }
    
        bool read_value_type(Value& value) noexcept {
            Type type = {};
            bin_assert(reader.read_symbol<':'>());
            bin_assert(reader.read_typename(type));
            if (type == Type::LIST) {
                List result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!ValueHelper::is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else if (type == Type::LIST2) {
                List2 result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!ValueHelper::is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else if (type == Type::OPTION) {
                Option result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!ValueHelper::is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else if (type == Type::MAP) {
                Map result = {};
                bin_assert(reader.read_symbol<'['>());
                bin_assert(reader.read_typename(result.keyType));
                bin_assert(ValueHelper::is_primitive(result.keyType));
                bin_assert(reader.read_symbol<','>());
                bin_assert(reader.read_typename(result.valueType));
                bin_assert(!ValueHelper::is_container(result.valueType));
                bin_assert(reader.read_symbol<']'>());
                value = result;
            } else {
                value = ValueHelper::type_to_value(type);
            }
            return true;
        }

        bool read_field(FieldList& list) noexcept {
            auto& item = list.emplace_back();
            bin_assert(reader.read_hash_name(item.key));
            bin_assert(read_value_type(item.value));
            bin_assert(reader.read_symbol<'='>());
            bin_assert(read_value(item.value));
            return true;
        }

        bool read_element(ElementList& list, Type valueType) noexcept {
            auto& item = list.emplace_back(ValueHelper::type_to_value(valueType));
            bin_assert(read_value(item.value));
            return true;
        }

        bool read_pair(PairList& list, Type keyType, Type valueType) noexcept {
            auto& item = list.emplace_back(ValueHelper::type_to_value(keyType),
                                           ValueHelper::type_to_value(valueType));
            bin_assert(read_value(item.key));
            bin_assert(reader.read_symbol<'='>());
            bin_assert(read_value(item.value));
            return true;
        }

        bool read_value(Value& value) noexcept {
            return std::visit([this](auto&& value) { return read_value_visit(value); }, value);
        }

        bool read_value_visit(List& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                bin_assert(read_element(value.items, value.valueType));
                bin_assert(reader.read_nested_separator_or_end(end));
            }
            return true;
        }

        bool read_value_visit(List2& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                bin_assert(read_element(value.items, value.valueType));
                bin_assert(reader.read_nested_separator_or_end(end));
            }
            return true;
        }

        bool read_value_visit(Option& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            if (!end) {
                bin_assert(read_element(value.items, value.valueType));
                bin_assert(reader.read_nested_separator_or_end(end));
                bin_assert(end);
            }
            return true;
        }

        bool read_value_visit(Map& value) noexcept {
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                bin_assert(read_pair(value.items, value.keyType, value.valueType));
                bin_assert(reader.read_nested_separator_or_end(end));
            }
            return true;
        }

        bool read_value_visit(Embed& value) noexcept {
            bin_assert(reader.read_hash_name(value.name));
            bool end = false;
            bin_assert(reader.read_nested_begin(end));
            while (!end) {
                bin_assert(read_field(value.items));
                bin_assert(reader.read_nested_separator_or_end(end));
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
                bin_assert(read_field(value.items));
                bin_assert(reader.read_nested_separator_or_end(end));
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

    public:
        std::string trace_error() noexcept {
            auto iter = reader.beg_;
            auto line_start = reader.beg_;
            size_t line_number = 1;
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

            std::string trace;
            for(auto e = error.crbegin(); e != error.crend(); e++) {
                trace.append(e->first);
                trace.append(" @ line: ");
                auto column_number = get_column(e->second);
                trace.append(std::to_string(line_number));
                trace.append(", column: ");
                trace.append(std::to_string(column_number));
                trace.append("\n");
            }
            trace.append("Last position @ line: ");
            auto column_number = get_column(reader.cur_);
            trace.append(std::to_string(line_number));
            trace.append(", column: ");
            trace.append(std::to_string(column_number));
            trace.append("\n");
            return trace;
        }
    };
}

namespace ritobin::io {
    using namespace impl_text_read;

    std::string read_text(Bin& bin, std::span<char const> data) noexcept {
        auto const begin = data.data();
        auto const end = data.data() + data.size();
        BinTextReader reader = { { begin, begin, end }, {} };
        if (!reader.process_bin(bin)) {
            return reader.trace_error();
        }
        return {};
    }

    std::string read_text(Value& value, std::span<char const> data) noexcept {
        auto const begin = data.data();
        auto const end = data.data() + data.size();
        BinTextReader reader = { { begin, begin, end }, {} };
        if (!reader.process_value(value)) {
            return reader.trace_error();
        }
        return {};
    }

    std::string read_text(FieldList& list, std::span<char const> data) noexcept {
        auto const begin = data.data();
        auto const end = data.data() + data.size();
        BinTextReader reader = { { begin, begin, end }, {} };
        if (!reader.process_list_fields(list)) {
            return reader.trace_error();
        }
        return {};
    }

    std::string read_text(ElementList& list, Type valueType, std::span<char const> data) noexcept {
        auto const begin = data.data();
        auto const end = data.data() + data.size();
        BinTextReader reader = { { begin, begin, end }, {} };
        if (!reader.process_list_elements(list, valueType)) {
            return reader.trace_error();
        }
        return {};
    }

    std::string read_text(PairList& list, Type keyType, Type valueType, std::span<char const> data) noexcept {
        auto const begin = data.data();
        auto const end = data.data() + data.size();
        BinTextReader reader = { { begin, begin, end }, {} };
        if (!reader.process_list_pairs(list, keyType, valueType)) {
            return reader.trace_error();
        }
        return {};
    }
}
