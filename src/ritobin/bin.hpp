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
#include <bit>

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

    constexpr inline auto MAX_PRIMITIVE = Type::FILE;

    constexpr inline auto MAX_COMPLEX = Type::FLAG;

    inline constexpr bool is_primitive(Type type) noexcept {
        return !((uint8_t)type & 0x80);
    }

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
            if (hash_ != h) {
                hash_ = h;
                str_.clear();
            }
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

    struct XXH64 {
    private:
        uint64_t hash_ = {};
        std::string str_ = {};
        static constexpr uint64_t xxh64(char const* data, size_t size, uint64_t seed = 0) noexcept {
            constexpr uint64_t Prime1 = 11400714785074694791U;
            constexpr uint64_t Prime2 = 14029467366897019727U;
            constexpr uint64_t Prime3 =  1609587929392839161U;
            constexpr uint64_t Prime4 =  9650029242287828579U;
            constexpr uint64_t Prime5 =  2870177450012600261U;
            constexpr auto Char = [](char c) constexpr -> uint64_t {
                return static_cast<uint8_t>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
            };
            constexpr auto HalfBlock = [Char](char const* data) constexpr -> uint64_t {
                return Char(*data)
                        | (Char(*(data + 1)) << 8)
                        | (Char(*(data + 2)) << 16)
                        | (Char(*(data + 3)) << 24);
            };
            constexpr auto Block = [Char](char const* data) constexpr -> uint64_t {
                return Char(*data)
                        | (Char(*(data + 1)) << 8)
                        | (Char(*(data + 2)) << 16)
                        | (Char(*(data + 3)) << 24)
                        | (Char(*(data + 4)) << 32)
                        | (Char(*(data + 5)) << 40)
                        | (Char(*(data + 6)) << 48)
                        | (Char(*(data + 7)) << 56);
            };
            constexpr auto ROL = [](uint64_t value, int ammount) -> uint64_t {
                return std::rotl(value, ammount);
            };
            char const* const end = data + size;
            uint64_t result = 0;
            if (size >= 32u) {
                uint64_t s1 = seed + Prime1 + Prime2;
                uint64_t s2 = seed + Prime2;
                uint64_t s3 = seed;
                uint64_t s4 = seed - Prime1;
                for(; data + 32 <= end; data += 32) {
                    s1 = ROL(s1 + Block(data) * Prime2, 31) * Prime1;
                    s2 = ROL(s2 + Block(data + 8) * Prime2, 31) * Prime1;
                    s3 = ROL(s3 + Block(data + 16) * Prime2, 31) * Prime1;
                    s4 = ROL(s4 + Block(data + 24) * Prime2, 31) * Prime1;
                }
                result  = ROL(s1,  1) +
                          ROL(s2,  7) +
                          ROL(s3, 12) +
                          ROL(s4, 18);
                result ^= ROL(s1 * Prime2, 31) * Prime1;
                result = result * Prime1 + Prime4;
                result ^= ROL(s2 * Prime2, 31) * Prime1;
                result = result * Prime1 + Prime4;
                result ^= ROL(s3 * Prime2, 31) * Prime1;
                result = result * Prime1 + Prime4;
                result ^= ROL(s4 * Prime2, 31) * Prime1;
                result = result * Prime1 + Prime4;
            } else {
                result = seed + Prime5;
            }
            result += static_cast<uint64_t>(size);
            for(; data + 8 <= end; data += 8) {
                result ^= ROL(Block(data) * Prime2, 31) * Prime1;
                result = ROL(result, 27) * Prime1 + Prime4;
            }
            for(; data + 4 <= end; data += 4) {
                result ^= HalfBlock(data) * Prime1;
                result = ROL(result, 23) * Prime2 + Prime3;
            }
            for(; data != end; ++data) {
                result ^= Char(*data) * Prime5;
                result = ROL(result, 11) * Prime1;
            }
            result ^= result >> 33;
            result *= Prime2;
            result ^= result >> 29;
            result *= Prime3;
            result ^= result >> 32;
            return result;
        }
        static constexpr uint64_t xxh64(std::string_view data, uint64_t seed = 0) noexcept {
            return xxh64(data.data(), data.size(), seed);
        }
    public:
        inline XXH64() noexcept = default;

        inline XXH64(std::string str) noexcept : hash_(xxh64(str)), str_(std::move(str)) {}

        inline XXH64(uint64_t h) noexcept : hash_(h), str_() {}

        inline XXH64& operator=(std::string str) noexcept {
            hash_ = xxh64(str);
            str_ = std::move(str);
            return *this;
        }

        inline XXH64& operator=(uint64_t h) noexcept {
            if (hash_ != h) {
                hash_ = h;
                str_.clear();
            }
            return *this;
        }

        inline uint64_t hash() const noexcept {
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

    struct File {
        static inline constexpr Type type = Type::FILE;
        static inline constexpr char type_name[] = "file";
        XXH64 value;
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

    struct BinUnhasher {
        std::unordered_map<uint32_t, std::string> fnv1a;
        std::unordered_map<uint64_t, std::string> xxh64;

        void unhash(FNV1a& value) const noexcept;
        void unhash(XXH64& value) const noexcept;
        void unhash(Value& value) const noexcept;
        void unhash(Bin& bin) const noexcept;
        bool load_fnv1a_CDTB(std::string const& filename) noexcept;
        bool load_xxh64_CDTB(std::string const& filename) noexcept;
    };
}

#endif // RITOBIN_HPP
