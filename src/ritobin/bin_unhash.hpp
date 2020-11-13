#ifndef BIN_UNHASH_HPP
#define BIN_UNHASH_HPP

#include "bin_types.hpp"
#include <istream>

namespace ritobin {
    struct BinUnhasher {
        std::unordered_map<uint32_t, std::string> fnv1a;
        std::unordered_map<uint64_t, std::string> xxh64;

        void unhash_bin(Bin& bin, int max_depth = 100) const noexcept;
        void unhash_value(Value& bin, int max_depth) const noexcept;
        void unhash_hash(FNV1a& bin) const noexcept;
        void unhash_hash(XXH64& bin) const noexcept;
        bool load_fnv1a_CDTB(std::istream& istream) noexcept;
        bool load_fnv1a_CDTB(std::string const& filename) noexcept;
        bool load_xxh64_CDTB(std::istream& istream) noexcept;
        bool load_xxh64_CDTB(std::string const& filename) noexcept;
    };
}

#endif // BIN_UNHASH_HPP
