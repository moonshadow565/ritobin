#include <stdexcept>
#include <fstream>
#include <charconv>
#include <string>
#include "bin.hpp"

namespace ritobin {
    struct BinUnhasherVisit {
        static void value(BinUnhasher const&, None const&) noexcept {}

        static void value(BinUnhasher const&, Bool const&) noexcept {}

        static void value(BinUnhasher const&, I8 const&) noexcept {}

        static void value(BinUnhasher const&, U8 const&) noexcept {}

        static void value(BinUnhasher const&, I16 const&) noexcept {}

        static void value(BinUnhasher const&, U16 const&) noexcept {}

        static void value(BinUnhasher const&, I32 const&) noexcept {}

        static void value(BinUnhasher const&, U32 const&) noexcept {}

        static void value(BinUnhasher const&, I64 const&) noexcept {}

        static void value(BinUnhasher const&, U64 const&) noexcept {}

        static void value(BinUnhasher const&, F32 const&) noexcept {}

        static void value(BinUnhasher const&, Vec2 const&) noexcept {}

        static void value(BinUnhasher const&, Vec3 const&) noexcept {}

        static void value(BinUnhasher const&, Vec4 const&) noexcept {}

        static void value(BinUnhasher const&, Mtx44 const&) noexcept {}

        static void value(BinUnhasher const&, RGBA const&) noexcept {}

        static void value(BinUnhasher const&, String const&) noexcept {}

        static void value(BinUnhasher const& unhasher, Hash& value) noexcept {
            unhasher.unhash(value.value);
        }

        static void value(BinUnhasher const& unhasher, File& value) noexcept {
            unhasher.unhash(value.value);
        }

        static void value(BinUnhasher const& unhasher, List& value) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash(item.value);
            }
        }

        static void value(BinUnhasher const& unhasher, List2& value) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash(item.value);
            }
        }

        static void value(BinUnhasher const& unhasher, Pointer& value) noexcept {
            unhasher.unhash(value.name);
            for (auto& item : value.items) {
                unhasher.unhash(item.key);
                unhasher.unhash(item.value);
            }
        }

        static void value(BinUnhasher const& unhasher, Embed& value) noexcept {
            unhasher.unhash(value.name);
            for (auto& item : value.items) {
                unhasher.unhash(item.key);
                unhasher.unhash(item.value);
            }
        }

        static void value(BinUnhasher const& unhasher, Link& value) noexcept {
            unhasher.unhash(value.value);
        }

        static void value(BinUnhasher const& unhasher, Option& value) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash(item.value);
            }
        }

        static void value(BinUnhasher const& unhasher, Map& value) noexcept {
            for (auto& item : value.items) {
                unhasher.unhash(item.key);
                unhasher.unhash(item.value);
            }
        }

        static void value(BinUnhasher const&, Flag const&) noexcept {}
    };

    void BinUnhasher::unhash(FNV1a& value) const noexcept {
        if (value.str().empty() && value.hash() != 0) {
            if (auto i = fnv1a.find(value.hash()); i != fnv1a.end()) {
                value = FNV1a(i->second);
            }
        }
    }

    void BinUnhasher::unhash(XXH64& value) const noexcept {
        if (value.str().empty() && value.hash() != 0) {
            if (auto i = xxh64.find(value.hash()); i != xxh64.end()) {
                value = XXH64(i->second);
            }
        }
    }

    void BinUnhasher::unhash(Value &value) const noexcept {
        std::visit([this](auto& value) {
            BinUnhasherVisit::value(*this, value);
        }, value);
    }

    void BinUnhasher::unhash(Bin& bin) const noexcept {
        for (auto& [key, value] : bin.sections) {
            unhash(value);
        }
    }

    bool BinUnhasher::load_fnv1a_CDTB(std::string const& filename) noexcept {
        auto file = std::ifstream{ filename };
        if (!file) {
            return false;
        }
        auto line = std::string{};
        while (std::getline(file, line)) {
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

    bool BinUnhasher::load_xxh64_CDTB(std::string const& filename) noexcept {
        auto file = std::ifstream{ filename };
        if (!file) {
            return false;
        }
        auto line = std::string{};
        while (std::getline(file, line)) {
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
}
