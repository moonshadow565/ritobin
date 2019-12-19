#include <stdexcept>
#include <fstream>
#include <charconv>
#include <string>
#include "bin.hpp"

namespace ritobin {
    template<typename=void>
    struct BinUnhasherImpl {
        static inline void unhash(HashTable const& lookup, FNV1a& value) noexcept {
            if (value.str().empty() && value.hash() != 0) {
                if (auto i = lookup.find(value.hash()); i != lookup.end()) {
                    value = FNV1a(i->second);
                }
            }
        }

        static inline void unhash(HashTable const& lookup, Link& value) noexcept {
            unhash(lookup, value.value);
        }

        static inline void unhash(HashTable const& lookup, Hash& value) noexcept {
            unhash(lookup, value.value);
        }

        static inline void unhash(HashTable const& lookup, Embed& value) noexcept {
            unhash(lookup, value.name);
            for (auto& item : value.items) {
                unhash(lookup, item.key);
                unhash(lookup, item.value);
            }
        }

        static inline void unhash(HashTable const& lookup, Pointer& value) noexcept {
            unhash(lookup, value.name);
            for (auto& item : value.items) {
                unhash(lookup, item.key);
                unhash(lookup, item.value);
            }
        }

        static inline void unhash(HashTable const& lookup, List& value) noexcept {
            for (auto& item : value.items) {
                unhash(lookup, item.value);
            }
        }

        static inline void unhash(HashTable const& lookup, Map& value) noexcept {
            for (auto& item : value.items) {
                unhash(lookup, item.key);
                unhash(lookup, item.value);
            }
        }

        static inline void unhash(HashTable const& lookup, Option& value) noexcept {
            for (auto& item : value.items) {
                unhash(lookup, item.value);
            }
        }

        static inline void unhash(HashTable const& lookup, Value& value) noexcept {
            std::visit([&lookup](auto& value) {
                using value_t = std::remove_const_t<std::remove_reference_t<decltype(value)>>;
                unhash(lookup, value);
                }, value);
        }

        template<typename T>
        static inline void unhash(HashTable const& lookup, T&) noexcept {}
    };

    void BinUnhasher::unhash(Bin& bin) const noexcept {
        for (auto& [key, value] : bin.sections) {
            BinUnhasherImpl<>::unhash(lookup, value);
        }
    }

    bool BinUnhasher::load_CDTB(std::string const& filename) noexcept {
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
                lookup[hash] = { beg + space + 1, length - space - 1 };
            }
        }
        return true;
    }
}