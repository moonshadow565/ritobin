#ifndef RITOBIN_HPP
#define RITOBIN_HPP

#include <cinttypes>
#include <cstddef>
#include <string>
#include <string_view>
#include <variant>
#include <array>
#include <vector>
#include <map>

namespace ritobin {
    enum class Type {
        NONE = 0,
        BOOL = 1,
        I8 = 2,
        U8 = 3,
        I16 = 4,
        U16 = 5,
        I32 = 6,
        U32 = 7,
        I64 = 8,
        U64 = 9,
        F32 = 10,
        VEC2 = 11,
        VEC3 = 12,
        VEC4 = 13,
        MTX44 = 14,
        RGBA = 15,
        STRING = 16,
        HASH = 17,
        LIST = 18,
        POINTER = 19,
        EMBED = 20,
        LINK = 21,
        OPTION = 22,
        MAP = 23,
        FLAG = 24,
    };

    inline constexpr uint32_t fnv1a(std::string_view str) noexcept {
        uint32_t h = 0x811c9dc5;
        for (uint32_t c : str) {
            c = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
            h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF;
        }
        return h;
    }

    struct NameHash {
        uint32_t value;
        std::string unhashed;

        inline uint32_t hash() const noexcept {
            return !unhashed.empty() ? fnv1a(unhashed) : value;
        }
    };

    struct StringHash {
        uint32_t value;
        std::string unhashed;

        inline uint32_t hash() const noexcept {
            return !unhashed.empty() ? fnv1a(unhashed) : value;
        }
    };

    struct None {
        static inline constexpr Type type = Type::NONE;
    };
    
    struct Bool {
        static inline constexpr Type type = Type::BOOL;
        bool value;
    };
    
    struct I8 {
        static inline constexpr Type type = Type::I8;
        int8_t value;
    };

    struct U8 {
        static inline constexpr Type type = Type::U8;
        uint8_t value;
    };

    struct I16 {
        static inline constexpr Type type = Type::I16;
        int16_t value;
    };

    struct U16 {
        static inline constexpr Type type = Type::U16;
        uint16_t value;
    };

    struct I32 {
        static inline constexpr Type type = Type::I32;
        int32_t value;
    };

    struct U32 {
        static inline constexpr Type type = Type::U32;
        uint32_t value;
    };

    struct I64 {
        static inline constexpr Type type = Type::I64;
        int64_t value;
    };

    struct U64 {
        static inline constexpr Type type = Type::U64;
        uint64_t value;
    };

    struct F32 {
        static inline constexpr Type type = Type::F32;
        float value;
    };

    struct Vec2 {
        static inline constexpr Type type = Type::VEC2;
        std::array<float, 2> value;
    };

    struct Vec3 {
        static inline constexpr Type type = Type::VEC3;
        std::array<float, 3> value;
    };

    struct Vec4 {
        static inline constexpr Type type = Type::VEC4;
        std::array<float, 4> value;
    };

    struct Mtx44 {
        static inline constexpr Type type = Type::MTX44;
        std::array<float, 16> value;
    };

    struct RGBA {
        static inline constexpr Type type = Type::RGBA;
        std::array<uint8_t, 4> value;
    };

    struct String {
        static inline constexpr Type type = Type::STRING;
        std::string value;
    };

    struct Hash {
        static inline constexpr Type type = Type::HASH;
        StringHash value;
    };

    struct List {
        static inline constexpr Type type = Type::LIST;
        Type valueType;
    };

    struct Pointer {
        static inline constexpr Type type = Type::POINTER;
        NameHash value;
    };

    struct Embed {
        static inline constexpr Type type = Type::EMBED;
        NameHash value;
    };

    struct Link {
        static inline constexpr Type type = Type::LINK;
        StringHash value;
    };

    struct Option {
        static inline constexpr Type type = Type::OPTION;
        Type valueType;
    };

    struct Map {
        static inline constexpr Type type = Type::MAP;
        Type keyType;
        Type valueType;
    };

    struct Flag {
        static inline constexpr Type type = Type::FLAG;
        bool value;
    };

    using Value = std::variant<
        None,
        Bool,
        I8,
        U8,
        I16,
        U16,
        I32,
        U32,
        I64,
        U64,
        F32,
        Vec2,
        Vec3,
        Vec4,
        Mtx44,
        RGBA,
        String,
        Hash,
        List,
        Pointer,
        Embed,
        Link,
        Option,
        Map,
        Flag
    >;

    inline constexpr Type Value_type(Value const& value) noexcept {
        return std::visit([](auto&& value){
            return value.type;
        }, value);
    }

    inline Value Value_from_type(Type type) noexcept {
        return []<typename...T>(std::variant<T...> const&, Type type) -> Value {
            Value result;
            ((T::type == type ? (result = T{}, true) : false) || ... );
            return result;
        } (Value{}, type);
    }

    struct Section {
        std::string name;
        Value value;
    };

    struct Item {
        uint32_t index;
        Value value;
    };

    struct Field {
        NameHash name;
        Value value;
    };

    struct Pair {
        Value key;
        Value value;
    };

    struct NestedEnd {
        Type type;
        uint32_t count;
    };

    using Node = std::variant <
        Section,
        Item,
        Field,
        Pair,
        NestedEnd
    >;

    using NodeList = std::vector<Node>;
    using LookupTable = std::map<uint32_t, std::string>;


    extern bool binary_read(std::string_view data, NodeList& result, std::string& error) noexcept;
    extern bool text_read(std::string_view data, NodeList& result, std::string& error) noexcept;

    extern bool binary_write(NodeList const& nodes, std::vector<char>& result, std::string& error) noexcept;
    extern bool text_write(NodeList const& nodes, std::vector<char>& result, std::string& error) noexcept;

    extern void lookup_load_CDTB(std::string const& filename, LookupTable& lookup) noexcept;

    extern void lookup_unhash(LookupTable const& lookup, NodeList& nodes) noexcept;
}

#endif // RITOBIN_HPP
