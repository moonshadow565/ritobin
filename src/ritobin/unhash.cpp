#include <stdexcept>
#include <fstream>
#include <charconv>
#include <string>
#include "bin.hpp"

namespace ritobin {
    struct BinUnhasherImpl {
        BinUnhasher const& unhasher;

        void unhash(FNV1a& value) const noexcept {
            if (value.str().empty() && value.hash() != 0) {
                if (auto i = unhasher.fnv1a.find(value.hash()); i != unhasher.fnv1a.end()) {
                    value = FNV1a(i->second);
                }
            }
        }

        void unhash(XXH64& value) const noexcept {
            if (value.str().empty() && value.hash() != 0) {
                if (auto i = unhasher.xxh64.find(value.hash()); i != unhasher.xxh64.end()) {
                    value = XXH64(i->second);
                }
            }
        }

        template<typename T>
        std::enable_if_t<std::is_arithmetic_v<T>> unhash(T&) const noexcept {}

        template<typename T, size_t S>
        void unhash(std::array<T, S>&) const noexcept {}

        void unhash(std::string&) const noexcept {}

        void unhash_value_visit(None&) const noexcept {}

        void unhash_value_visit(Embed& value) const noexcept {
            unhash(value.name);
            for (auto& item : value.items) {
                unhash(item.key);
                unhash_value(item.value);
            }
        }

        void unhash_value_visit(Pointer& value) const noexcept {
            unhash(value.name);
            for (auto& item : value.items) {
                unhash(item.key);
                unhash_value(item.value);
            }
        }

        void unhash_value_visit(List& value) const noexcept {
            for (auto& item : value.items) {
                unhash_value(item.value);
            }
        }

        void unhash_value_visit(List2& value) const noexcept {
            for (auto& item : value.items) {
                unhash_value(item.value);
            }
        }

        void unhash_value_visit(Map& value) const noexcept {
            for (auto& item : value.items) {
                unhash_value(item.key);
                unhash_value(item.value);
            }
        }

        void unhash_value_visit(Option& value) const noexcept {
            for (auto& item : value.items) {
                unhash_value(item.value);
            }
        }

        template<typename T>
        void unhash_value_visit(T& value) const noexcept {
            unhash(value.value);
        }

        void unhash_value(Value& value) const noexcept {
            std::visit([this](auto& value) {
                unhash_value_visit(value);
                }, value);
        }
    };

    void BinUnhasher::unhash(Bin& bin) const noexcept {
        BinUnhasherImpl impl { *this };
        for (auto& [key, value] : bin.sections) {
            impl.unhash_value(value);
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
