#include "bin_hash.hpp"
#include <bit>

uint32_t ritobin::FNV1a::fnv1a(std::string_view str) noexcept {
    uint32_t h = 0x811c9dc5;
    for (uint32_t c : str) {
        c = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
        h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF;
    }
    return h;
}

uint64_t ritobin::XXH64::xxh64(std::string_view str, uint64_t seed) noexcept {
    auto data = str.data();
    auto const size = str.size();
    auto const end = data + size;
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

