#include <fstream>
#include <charconv>
#include <string>
#include <filesystem>
#include "bin_unhash.hpp"

namespace ritobin {
    struct BinUnhasherVisit {
        static void value(BinUnhasher const&, None const&, int) noexcept {}

        static void value(BinUnhasher const&, Bool const&, int) noexcept {}

        static void value(BinUnhasher const&, I8 const&, int) noexcept {}

        static void value(BinUnhasher const&, U8 const&, int) noexcept {}

        static void value(BinUnhasher const&, I16 const&, int) noexcept {}

        static void value(BinUnhasher const&, U16 const&, int) noexcept {}

        static void value(BinUnhasher const&, I32 const&, int) noexcept {}

        static void value(BinUnhasher const&, U32 const&, int) noexcept {}

        static void value(BinUnhasher const&, I64 const&, int) noexcept {}

        static void value(BinUnhasher const&, U64 const&, int) noexcept {}

        static void value(BinUnhasher const&, F32 const&, int) noexcept {}

        static void value(BinUnhasher const&, Vec2 const&, int) noexcept {}

        static void value(BinUnhasher const&, Vec3 const&, int) noexcept {}

        static void value(BinUnhasher const&, Vec4 const&, int) noexcept {}

        static void value(BinUnhasher const&, Mtx44 const&, int) noexcept {}

        static void value(BinUnhasher const&, RGBA const&, int) noexcept {}

        static void value(BinUnhasher const&, String const&, int) noexcept {}

        static void value(BinUnhasher const& unhasher, Hash& value, int) noexcept {
            unhasher.unhash_hash(value.value);
        }

        static void value(BinUnhasher const& unhasher, File& value, int) noexcept {
            unhasher.unhash_hash(value.value);
        }

        static void value(BinUnhasher const& unhasher, List& value, int max_depth) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash_value(item.value, max_depth);
            }
        }

        static void value(BinUnhasher const& unhasher, List2& value, int max_depth) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash_value(item.value, max_depth);
            }
        }

        static void value(BinUnhasher const& unhasher, Pointer& value, int max_depth) noexcept {
            unhasher.unhash_hash(value.name);
            for (auto& item : value.items) {
                unhasher.unhash_hash(item.key);
                unhasher.unhash_value(item.value, max_depth);
            }
        }

        static void value(BinUnhasher const& unhasher, Embed& value, int max_depth) noexcept {
            unhasher.unhash_hash(value.name);
            for (auto& item : value.items) {
                unhasher.unhash_hash(item.key);
                unhasher.unhash_value(item.value, max_depth);
            }
        }

        static void value(BinUnhasher const& unhasher, Link& value, int) noexcept {
            unhasher.unhash_hash(value.value);
        }

        static void value(BinUnhasher const& unhasher, Option& value, int max_depth) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash_value(item.value, max_depth);
            }
        }

        static void value(BinUnhasher const& unhasher, Map& value, int max_depth) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash_value(item.key, max_depth);
                unhasher.unhash_value(item.value, max_depth);
            }
        }

        static void value(BinUnhasher const&, Flag const&, int) noexcept {}
    };

    void BinUnhasher::unhash_hash(FNV1a& value) const noexcept {
        if (value.str().empty() && value.hash() != 0) {
            if (auto i = fnv1a.find(value.hash()); i != fnv1a.end()) {
                value = FNV1a(i->second);
            }
        }
    }

    void BinUnhasher::unhash_hash(XXH64& value) const noexcept {
        if (value.str().empty() && value.hash() != 0) {
            if (auto i = xxh64.find(value.hash()); i != xxh64.end()) {
                value = XXH64(i->second);
            }
        }
    }

    void BinUnhasher::unhash_value(Value &value, int max_depth) const noexcept {
        if (max_depth > 0) {
            std::visit([this, max_depth] (auto& value) {
                BinUnhasherVisit::value(*this, value, max_depth - 1);
            }, value);
        }
    }

    void BinUnhasher::unhash_bin(Bin& bin, int max_depth) const noexcept {
        for (auto& [key, value] : bin.sections) {
            unhash_value(value, max_depth);
        }
    }

    bool BinUnhasher::load_fnv1a_CDTB(std::istream& istream) noexcept {
        if (!istream) {
            return false;
        }
        auto line = std::string{};
        while (std::getline(istream, line)) {
            if (line.empty()) {
                break;
            }
            auto const beg = line.data();
            auto const space = line.find_first_of(' ');
            auto const length = line.size();
            if (space != length) {
                auto hash = uint32_t{};
                std::from_chars(beg, beg + space, hash, 16);
                fnv1a[hash] = { beg + space + 1, length - space - 1 };
            }
        }
        return true;
    }

    bool BinUnhasher::load_fnv1a_CDTB(std::string const& filename) noexcept {
        if (std::ifstream file(filename); load_fnv1a_CDTB(file)) {
            return true;
        }
        bool had_some = false;
        for (size_t i = 0;;i++) {
            if (std::ifstream file(filename + "." + std::to_string(i)); !load_fnv1a_CDTB(file)) {
                break;
            }
            had_some = true;
        }
        return had_some;
    }

    bool BinUnhasher::load_xxh64_CDTB(std::istream& istream) noexcept {
        if (!istream) {
            return false;
        }
        auto line = std::string{};
        while (std::getline(istream, line)) {
            if (line.empty()) {
                break;
            }
            auto const beg = line.data();
            auto const space = line.find_first_of(' ');
            auto const length = line.size();
            if (space != length) {
                auto hash = uint64_t{};
                std::from_chars(beg, beg + space, hash, 16);
                xxh64[hash] = { beg + space + 1, length - space - 1 };
            }
        }
        return true;
    }

    bool BinUnhasher::load_xxh64_CDTB(std::string const& filename) noexcept {
        if (std::ifstream file(filename); load_xxh64_CDTB(file)) {
            return true;
        }
        bool had_some = false;
        for (size_t i = 0;;i++) {
            if (std::ifstream file(filename + "." + std::to_string(i)); !load_xxh64_CDTB(file)) {
                break;
            }
            had_some = true;
        }
        return had_some;
    }
}
