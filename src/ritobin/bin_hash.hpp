#ifndef BIN_HASH_HPP
#define BIN_HASH_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace ritobin {
    struct FNV1a {
    private:
        uint32_t hash_ = 0;
        std::string str_ = {};
        static uint32_t fnv1a(std::string_view str) noexcept;
    public:
        using storage_t = uint32_t;

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
        static uint64_t xxh64(std::string_view str, uint64_t seed = 0) noexcept;
    public:
        using storage_t = uint64_t;

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
}

#endif // BIN_HASH_HPP
