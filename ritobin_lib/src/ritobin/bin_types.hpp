#ifndef BIN_TYPES_HPP
#define BIN_TYPES_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "bin_hash.hpp"

namespace ritobin {
    enum class Type : uint8_t {
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
        FILE = 18,
        LIST = 0x80 | 0,
        LIST2 = 0x80 | 1,
        POINTER = 0x80 | 2,
        EMBED = 0x80 | 3,
        LINK = 0x80 | 4,
        OPTION = 0x80 | 5,
        MAP = 0x80 | 6,
        FLAG = 0x80 | 7,
    };

    enum class Category {
        NONE,
        NUMBER,
        VECTOR,
        STRING,
        HASH,
        OPTION,
        LIST,
        MAP,
        CLASS,
    };

    struct Element;
    struct Field;
    struct Pair;

    using ElementList = std::vector<Element>;
    using FieldList = std::vector<Field>;
    using PairList = std::vector<Pair>;

    struct None {
        static inline constexpr Type type = Type::NONE;
        static inline constexpr char type_name[] = "none";
        static inline constexpr Category category = Category::NONE;
    };

    struct Bool {
        static inline constexpr Type type = Type::BOOL;
        static inline constexpr char type_name[] = "bool";
        static inline constexpr Category category = Category::NUMBER;
        bool value = {};
    };

    struct I8 {
        static inline constexpr Type type = Type::I8;
        static inline constexpr char type_name[] = "i8";
        static inline constexpr Category category = Category::NUMBER;
        int8_t value = {};
    };

    struct U8 {
        static inline constexpr Type type = Type::U8;
        static inline constexpr char type_name[] = "u8";
        static inline constexpr Category category = Category::NUMBER;
        uint8_t value = {};
    };

    struct I16 {
        static inline constexpr Type type = Type::I16;
        static inline constexpr char type_name[] = "i16";
        static inline constexpr Category category = Category::NUMBER;
        int16_t value = {};
    };

    struct U16 {
        static inline constexpr Type type = Type::U16;
        static inline constexpr char type_name[] = "u16";
        static inline constexpr Category category = Category::NUMBER;
        uint16_t value = {};
    };

    struct I32 {
        static inline constexpr Type type = Type::I32;
        static inline constexpr char type_name[] = "i32";
        static inline constexpr Category category = Category::NUMBER;
        int32_t value = {};
    };

    struct U32 {
        static inline constexpr Type type = Type::U32;
        static inline constexpr char type_name[] = "u32";
        static inline constexpr Category category = Category::NUMBER;
        uint32_t value = {};
    };

    struct I64 {
        static inline constexpr Type type = Type::I64;
        static inline constexpr char type_name[] = "i64";
        static inline constexpr Category category = Category::NUMBER;
        int64_t value = {};
    };

    struct U64 {
        static inline constexpr Type type = Type::U64;
        static inline constexpr char type_name[] = "u64";
        static inline constexpr Category category = Category::NUMBER;
        uint64_t value = {};
    };

    struct F32 {
        static inline constexpr Type type = Type::F32;
        static inline constexpr char type_name[] = "f32";
        static inline constexpr Category category = Category::NUMBER;
        float value = {};
    };

    struct Vec2 {
        static inline constexpr Type type = Type::VEC2;
        static inline constexpr char type_name[] = "vec2";
        static inline constexpr Category category = Category::VECTOR;
        using item_wrapper_type = F32;
        std::array<float, 2> value = {};
    };

    struct Vec3 {
        static inline constexpr Type type = Type::VEC3;
        static inline constexpr char type_name[] = "vec3";
        static inline constexpr Category category = Category::VECTOR;
        using item_wrapper_type = F32;
        std::array<float, 3> value = {};
    };

    struct Vec4 {
        static inline constexpr Type type = Type::VEC4;
        static inline constexpr char type_name[] = "vec4";
        static inline constexpr Category category = Category::VECTOR;
        using item_wrapper_type = F32;
        std::array<float, 4> value = {};
    };

    struct Mtx44 {
        static inline constexpr Type type = Type::MTX44;
        static inline constexpr char type_name[] = "mtx44";
        static inline constexpr Category category = Category::VECTOR;
        using item_wrapper_type = F32;
        std::array<float, 16> value = {};
    };

    struct RGBA {
        static inline constexpr Type type = Type::RGBA;
        static inline constexpr char type_name[] = "rgba";
        static inline constexpr Category category = Category::VECTOR;
        using item_wrapper_type = U8;
        std::array<uint8_t, 4> value = {};
    };

    struct String {
        static inline constexpr Type type = Type::STRING;
        static inline constexpr char type_name[] = "string";
        static inline constexpr Category category = Category::STRING;
        std::string value = {};
    };

    struct Hash {
        static inline constexpr Type type = Type::HASH;
        static inline constexpr char type_name[] = "hash";
        static inline constexpr Category category = Category::HASH;
        FNV1a value = {};
    };

    struct File {
        static inline constexpr Type type = Type::FILE;
        static inline constexpr char type_name[] = "file";
        static inline constexpr Category category = Category::HASH;
        XXH64 value = {};
    };

    struct List {
        static inline constexpr Type type = Type::LIST;
        static inline constexpr char type_name[] = "list";
        static inline constexpr Category category = Category::LIST;
        Type valueType = {};
        ElementList items = {};
    };

    struct List2 {
        static inline constexpr Type type = Type::LIST2;
        static inline constexpr char type_name[] = "list2";
        static inline constexpr Category category = Category::LIST;
        Type valueType = {};
        ElementList items = {};
    };

    struct Pointer {
        static inline constexpr Type type = Type::POINTER;
        static inline constexpr char type_name[] = "pointer";
        static inline constexpr Category category = Category::CLASS;
        FNV1a name = {};
        FieldList items = {};
        inline Field* find_field(FNV1a const& key) noexcept;
        inline Field const* find_field(FNV1a const& key) const noexcept;
    };

    struct Embed {
        static inline constexpr Type type = Type::EMBED;
        static inline constexpr char type_name[] = "embed";
        static inline constexpr Category category = Category::CLASS;
        FNV1a name = {};
        FieldList items = {};
        inline Field* find_field(FNV1a const& key) noexcept;
        inline Field const* find_field(FNV1a const& key) const noexcept;
    };

    struct Link {
        static inline constexpr Type type = Type::LINK;
        static inline constexpr char type_name[] = "link";
        static inline constexpr Category category = Category::HASH;
        FNV1a value = {};
    };

    struct Option {
        static inline constexpr Type type = Type::OPTION;
        static inline constexpr char type_name[] = "option";
        static inline constexpr Category category = Category::OPTION;
        Type valueType = {};
        ElementList items = {};
    };

    struct Map {
        static inline constexpr Type type = Type::MAP;
        static inline constexpr char type_name[] = "map";
        static inline constexpr Category category = Category::MAP;
        Type keyType = {};
        Type valueType = {};
        PairList items = {};
    };

    struct Flag {
        static inline constexpr Type type = Type::FLAG;
        static inline constexpr char type_name[] = "flag";
        bool value = {};
        static inline constexpr Category category = Category::NUMBER;
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
        File,
        List,
        List2,
        Pointer,
        Embed,
        Link,
        Option,
        Map,
        Flag
    >;

    struct Element {
        static inline constexpr char type_name[] = "element";
        Value value = {};
        Element();
        Element(Value value);
    };

    struct Pair {
        static inline constexpr char type_name[] = "pair";
        Value key = {};
        Value value = {};
        Pair();
        Pair(Value key, Value value);
    };

    struct Field {
        static inline constexpr char type_name[] = "field";
        FNV1a key = {};
        Value value = {};
        Field();
        Field(FNV1a key, Value value);
    };

    inline Field* Pointer::find_field(FNV1a const& key) noexcept {
        for (auto& field: items) {
            if (field.key.hash() == key.hash()) {
                return &field;
            }
        }
        return nullptr;
    }

    inline Field const* Pointer::find_field(FNV1a const& key) const noexcept {
        for (auto const& field: items) {
            if (field.key.hash() == key.hash()) {
                return &field;
            }
        }
        return nullptr;
    }

    inline Field* Embed::find_field(FNV1a const& key) noexcept {
        for (auto& field: items) {
            if (field.key.hash() == key.hash()) {
                return &field;
            }
        }
        return nullptr;
    }

    inline Field const* Embed::find_field(FNV1a const& key) const noexcept {
        for (auto const& field: items) {
            if (field.key.hash() == key.hash()) {
                return &field;
            }
        }
        return nullptr;
    }

    inline Element::Element() {}
    inline Element::Element(Value value) : value(std::move(value)) {}

    inline Pair::Pair() {}
    inline Pair::Pair(Value key, Value value) : key(std::move(key)), value(std::move(value)) {}

    inline Field::Field() {}
    inline Field::Field(FNV1a key, Value value) : key(std::move(key)), value(std::move(value)) {}

    struct Bin {
        static inline constexpr char type_name[] = "sections";
        std::unordered_map<std::string, Value> sections;
    };
}

#endif // BIN_TYPES_HPP
