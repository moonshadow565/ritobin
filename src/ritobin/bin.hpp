#ifndef RITOBIN_HPP
#define RITOBIN_HPP

#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <variant>
#include <array>
#include <vector>
#include <unordered_map>

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
        LIST = 18,
        LIST2 = 19,
        POINTER = 19 + 1,
        EMBED = 20 + 1,
        LINK = 21 + 1,
        OPTION = 22 + 1,
        MAP = 23 + 1,
        FLAG = 24 + 1,
    };

    inline constexpr bool is_container(Type type) noexcept {
        return type == Type::MAP || type == Type::LIST || type == Type::LIST2 || type == Type::OPTION;
    }

    struct FNV1a {
    private:
        uint32_t hash_ = 0;
        std::string str_ = {};
        static inline constexpr uint32_t fnv1a(std::string_view str) noexcept {
            uint32_t h = 0x811c9dc5;
            for (uint32_t c : str) {
                c = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
                h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF;
            }
            return h;
        }
    public:
        inline FNV1a() noexcept = default;

        inline FNV1a(std::string str) noexcept : hash_(fnv1a(str)), str_(std::move(str)) {}

        inline FNV1a(uint32_t h) noexcept : hash_(h), str_() {}

        inline FNV1a& operator=(std::string str) noexcept {
            hash_ = fnv1a(str);
            str_ = std::move(str);
            return *this;
        }

        inline FNV1a& operator=(uint32_t h) noexcept {
            hash_ = h;
            str_.clear();
            return *this;
        }
        
        inline uint32_t hash() const noexcept {
            return hash_;
        }

        inline std::string_view str() const& noexcept {
            return str_;
        }

        inline std::string str() && noexcept {
            return std::move(str_);
        }
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
    };

    struct Bool {
        static inline constexpr Type type = Type::BOOL;
        static inline constexpr char type_name[] = "bool";
        bool value;
    };

    struct I8 {
        static inline constexpr Type type = Type::I8;
        static inline constexpr char type_name[] = "i8";
        int8_t value;
    };

    struct U8 {
        static inline constexpr Type type = Type::U8;
        static inline constexpr char type_name[] = "u8";
        uint8_t value;
    };

    struct I16 {
        static inline constexpr Type type = Type::I16;
        static inline constexpr char type_name[] = "i16";
        int16_t value;
    };

    struct U16 {
        static inline constexpr Type type = Type::U16;
        static inline constexpr char type_name[] = "u16";
        uint16_t value;
    };

    struct I32 {
        static inline constexpr Type type = Type::I32;
        static inline constexpr char type_name[] = "i32";
        int32_t value;
    };

    struct U32 {
        static inline constexpr Type type = Type::U32;
        static inline constexpr char type_name[] = "u32";
        uint32_t value;
    };

    struct I64 {
        static inline constexpr Type type = Type::I64;
        static inline constexpr char type_name[] = "i64";
        int64_t value;
    };

    struct U64 {
        static inline constexpr Type type = Type::U64;
        static inline constexpr char type_name[] = "u64";
        uint64_t value;
    };

    struct F32 {
        static inline constexpr Type type = Type::F32;
        static inline constexpr char type_name[] = "f32";
        float value;
    };

    struct Vec2 {
        static inline constexpr Type type = Type::VEC2;
        static inline constexpr char type_name[] = "vec2";
        std::array<float, 2> value;
    };

    struct Vec3 {
        static inline constexpr Type type = Type::VEC3;
        static inline constexpr char type_name[] = "vec3";
        std::array<float, 3> value;
    };

    struct Vec4 {
        static inline constexpr Type type = Type::VEC4;
        static inline constexpr char type_name[] = "vec4";
        std::array<float, 4> value;
    };

    struct Mtx44 {
        static inline constexpr Type type = Type::MTX44;
        static inline constexpr char type_name[] = "mtx44";
        std::array<float, 16> value;
    };

    struct RGBA {
        static inline constexpr Type type = Type::RGBA;
        static inline constexpr char type_name[] = "rgba";
        std::array<uint8_t, 4> value;
    };

    struct String {
        static inline constexpr Type type = Type::STRING;
        static inline constexpr char type_name[] = "string";
        std::string value;
    };

    struct Hash {
        static inline constexpr Type type = Type::HASH;
        static inline constexpr char type_name[] = "hash";
        FNV1a value;
    };

    struct List {
        static inline constexpr Type type = Type::LIST;
        static inline constexpr char type_name[] = "list";
        Type valueType;
        ElementList items;
    };

    struct List2 {
        static inline constexpr Type type = Type::LIST2;
        static inline constexpr char type_name[] = "list2";
        Type valueType;
        ElementList items;
    };

    struct Pointer {
        static inline constexpr Type type = Type::POINTER;
        static inline constexpr char type_name[] = "pointer";
        FNV1a name;
        FieldList items;
    };

    struct Embed {
        static inline constexpr Type type = Type::EMBED;
        static inline constexpr char type_name[] = "embed";
        FNV1a name;
        FieldList items;
    };

    struct Link {
        static inline constexpr Type type = Type::LINK;
        static inline constexpr char type_name[] = "link";
        FNV1a value;
    };

    struct Option {
        static inline constexpr Type type = Type::OPTION;
        static inline constexpr char type_name[] = "option";
        Type valueType;
        ElementList items;
    };

    struct Map {
        static inline constexpr Type type = Type::MAP;
        static inline constexpr char type_name[] = "map";
        Type keyType;
        Type valueType;
        PairList items;
    };

    struct Flag {
        static inline constexpr Type type = Type::FLAG;
        static inline constexpr char type_name[] = "flag";
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
        List2,
        Pointer,
        Embed,
        Link,
        Option,
        Map,
        Flag
    >;

    struct Element {
        Value value;
    };

    struct Pair {
        Value key;
        Value value;
    };

    struct Field {
        FNV1a key;
        Value value;
    };

    template<typename> struct ValueHelperImpl;

    template<typename...T> struct ValueHelperImpl<std::variant<T...>> {

        static inline Type to_type(Value const& value) noexcept {
            return std::visit([](auto&& value) { return value.type; }, value);
        }

        static inline Value from_type(Type type) noexcept {
            Value value = None{};
            ((T::type == type ? (value = T{}, true) : false) || ...);
            return value;
        }

        static inline std::string_view to_type_name(Value const& value) noexcept {
            return std::visit([](auto&& value) { return value.type_name; }, value);
        }

        static inline std::string_view to_type_name(Type type) noexcept {
            std::string_view type_name = {};
            ((T::type == type ? (type_name = T::type_name, true) : false) || ...);
            return type_name;
        }

        static inline Value from_type_name(std::string_view type_name) noexcept {
            Value value = None{};
            ((type_name == T::type_name ? (value = T{}, true) : false) || ...);
            return value;
        }

        static inline bool from_type_name(std::string_view type_name, Type& type) noexcept {
            return ((type_name == T::type_name ? (type = T::type, true) : false) || ...);
        }
    };
    using ValueHelper = ValueHelperImpl<Value>;

    struct Bin {
        std::unordered_map<std::string, Value> sections;

        void read_binary(char const* data, size_t size);
        void read_binary_file(std::string const& filename);
        void write_binary(std::vector<char>& out) const;
        void write_binary_file(std::string const& filename) const;

        void read_text(char const* data, size_t size);
        void read_text_file(std::string const& filename);
        void write_text(std::vector<char>& out, size_t ident_size = 2) const;
        void write_text_file(std::string const& filename, size_t ident_size = 2) const;
    };

    using HashTable = std::unordered_map<uint32_t, std::string>;
    struct BinUnhasher {
        HashTable lookup;

        void unhash(Bin& bin) const noexcept;
        bool load_CDTB(std::string const& filename) noexcept;
    };
}

#endif // RITOBIN_HPP
