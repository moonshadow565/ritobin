#include <vector>
#include <cstring>
#include <string_view>
#include <array>
#include "bin.hpp"


namespace ritobin {
    struct BytesIter {
        char const* const beg_;
        char const* cur_;
        char const* const cap_;

        inline constexpr size_t position() const noexcept {
            return cur_ - beg_;
        }

        template<typename T>
        inline bool read(T& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            if (cur_ + sizeof(T) > cap_) {
                return false;
            }
            memcpy(&value, cur_, sizeof(T));
            cur_ += sizeof(T);
            return true;
        }

        template<typename T, size_t SIZE>
        bool read(std::array<T, SIZE>& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            if (cur_ + sizeof(T) * SIZE > cap_) {
                return false;
            }
            memcpy(value.data(), cur_, sizeof(T) * SIZE);
            cur_ += sizeof(T) * SIZE;
            return true;
        }

        template<typename T>
        bool read(std::vector<T>& value, size_t size) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            if (cur_ + sizeof(T) * size > cap_) {
                return false;
            }
            value.resize(size);
            memcpy(value.data(), cur_, sizeof(T) * size);
            cur_ += sizeof(T) * size;
            return true;
        }

        bool read(std::string& value) noexcept {
            uint16_t size = {};
            if (!read(size)) {
                return false;
            }
            if (cur_ + size > cap_) {
                return false;
            }
            value = { cur_, size };
            cur_ += size;
            return true;
        }

        bool read(Type& value) noexcept {
            uint8_t raw = {};
            if (!read(raw)) {
                return false;
            }
            uint32_t backup = raw;
            if (raw & 0x80) {
                raw -= 0x80;
                raw += 18;
            }
            value = static_cast<Type>(raw);
            if (value > Type::FLAG) {
                return false;
            }
            return value <= Type::FLAG;
        }

        bool read(NameHash& value) noexcept {
            // TODO: verify a-zA-Z0-9_
            value = {};
            return read(value.value);
        }

        bool read(StringHash& value) noexcept {
            // TODO: verify utf-8
            value = {};
            return read(value.value);
        }
    };

    struct ReadBinaryImpl {
        BytesIter bytes_;
        std::vector<Node>& nodes;
        std::string& error;

        bool process() noexcept {
            nodes.clear();
            error.clear();
            if (!process_sections()) {
                error.append("Failed to read at: " + std::to_string(bytes_.position()));
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

        bool read_value(Value& value, Type type) noexcept {
            value = Value_from_type(type);
            return std::visit([this](auto&& value) -> bool {
                return read_value_visit(value);
                }, value);
        }

        bool read_value_visit(Embed& value) {
            bin_assert(bytes_.read(value.value));
            return true;
        }

        bool read_value_visit(Pointer& value) {
            bin_assert(bytes_.read(value.value));
            return true;
        }

        bool read_value_visit(List& value) {
            bin_assert(bytes_.read(value.valueType));
            return true;
        }

        bool read_value_visit(Option& value) {
            bin_assert(bytes_.read(value.valueType));
            return true;
        }

        bool read_value_visit(Map& value) {
            bin_assert(bytes_.read(value.keyType));
            bin_assert(bytes_.read(value.valueType));
            return true;
        }

        bool read_value_visit(None&) {
            return true;
        }

        template<typename T>
        bool read_value_visit(T& value) {
            bin_assert(bytes_.read(value.value));
            return true;
        }

        bool process_sections() noexcept {
            std::array<char, 4> magic = {};
            uint32_t version = 0;
            bin_assert(bytes_.read(magic));
            if (magic == std::array{ 'P', 'T', 'C', 'H' }) {
                uint64_t unk = {};
                bin_assert(bytes_.read(unk));
                bin_assert(bytes_.read(magic));
                handle_section("type" , String{ "PTCH" });
            } else {
                handle_section("type", String{ "PROP" });
            }
            bin_assert(magic == std::array{ 'P', 'R', 'O', 'P' });
            bin_assert(bytes_.read(version));
            handle_section("version", U32{ version });

            if (version >= 2) {
                bin_assert(process_linked());
            }

            bin_assert(process_entries());
            return true;
        }

        bool process_linked() noexcept {
            uint32_t linkedFilesCount = {};
            String linked = {};
            bin_assert(bytes_.read(linkedFilesCount));

            handle_section("linked", List{ Type::STRING });
            for (uint32_t i = 0; i < linkedFilesCount; i++) {
                bin_assert(bytes_.read(linked.value));
                handle_item(i, linked);
            }
            handle_nested_end(Type::LIST, linkedFilesCount);
            return true;
        }

        bool process_entries() noexcept {
            uint32_t entryCount = 0;
            std::vector<uint32_t> entryTypes;
            bin_assert(bytes_.read(entryCount));
            bin_assert(bytes_.read(entryTypes, entryCount));

            handle_section("entries", Map { Type::HASH, Type::EMBED });
            for (uint32_t type : entryTypes) {
                bin_assert(process_entry(type));
            }
            handle_nested_end(Type::MAP, entryCount);
            return true;
        }

        bool process_entry(uint32_t type)  noexcept {
            uint32_t entryLength;
            StringHash hash;
            uint16_t count;
            bin_assert(bytes_.read(entryLength));
            bin_assert(bytes_.read(hash));
            bin_assert(bytes_.read(count));

            handle_pair(Hash{ hash }, Pointer{ type });
            for (uint32_t i = 0; i < count; i++) {
                bin_assert(process_field());
            }
            handle_nested_end(Type::POINTER, count);
            return true;
        }

        bool process_field() noexcept {
            NameHash key;
            Type type;
            Value value;
            bin_assert(bytes_.read(key.value));
            bin_assert(bytes_.read(type));
            bin_assert(read_value(value, type));

            handle_field(key, value);
            bin_assert(process_value(value));
            return true;
        }

        bool process_value_visit(Pointer const& pointer) noexcept {
            if (pointer.value.value != 0) {
                uint32_t size = 0;
                uint16_t count = 0;
                bin_assert(bytes_.read(size));
                bin_assert(bytes_.read(count));
                for (uint32_t i = 0; i < count; i++) {
                    bin_assert(process_field());
                }
                handle_nested_end(pointer.type, count);
            }
            return true;
        }

        bool process_value_visit(Embed const& embed) noexcept {
            uint32_t size = 0;
            uint16_t count = 0;
            bin_assert(bytes_.read(size));
            bin_assert(bytes_.read(count));
            for (uint32_t i = 0; i < count; i++) {
                bin_assert(process_field());
            }
            handle_nested_end(embed.type, count);
            return true;
        }

        bool process_value_visit(List const& list) noexcept {
            uint32_t size;
            uint32_t count;
            Value value = None{};
            bin_assert(bytes_.read(size));
            bin_assert(bytes_.read(count));
            for (uint32_t i = 0; i < count; i++) {
                bin_assert(read_value(value, list.valueType));
                handle_item(i, value);
                bin_assert(process_value(value));
            }
            handle_nested_end(list.type, count);
            return true;
        }

        bool process_value_visit(Option const& option) noexcept {
            uint8_t count;
            bin_assert(bytes_.read(count));
            if (count != 0) {
                Value value = None{};
                bin_assert(read_value(value, option.valueType));
                handle_item(1, value);
                bin_assert(process_value(value));
            }
            handle_nested_end(option.type, count);
            return true;
        }

        bool process_value_visit(Map const& map) noexcept {
            uint32_t size;
            uint32_t count;
            Value key;
            Value value;
            bin_assert(bytes_.read(size));
            bin_assert(bytes_.read(count));
            for (uint32_t i = 0; i < count; i++) {
                bin_assert(read_value(key, map.keyType));
                bin_assert(read_value(value, map.valueType));
                handle_pair(key, value);
                bin_assert(process_value(value));
            }
            handle_nested_end(map.type, count);
            return true;
        }

        template<typename T>
        bool process_value_visit(T const& map) noexcept {
            return true;
        }

        bool process_value(Value const& value) noexcept {
            return std::visit([this](auto&& value) -> bool {
                return process_value_visit(value);
            }, value);
        }
#undef bin_assert
    };

    bool binary_read(std::string_view data, std::vector<Node>& result, std::string& error) noexcept {
        ReadBinaryImpl reader {
            { data.data(), data.data(), data.data() + data.size() }, result, error
        };
        return reader.process();
    }
}
