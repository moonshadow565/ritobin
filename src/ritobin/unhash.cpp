#include <map>
#include <fstream>
#include <charconv>
#include <string>
#include <optional>
#include "bin.hpp"

namespace ritobin {
    void lookup_load_CDTB(std::string const& filename, LookupTable& lookup) noexcept {
        auto file = std::ifstream{ std::string(filename) };
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
    }

    template<typename T>
    inline void unhash(LookupTable const& lookup, T const&) noexcept {}


    inline void unhash(LookupTable const& lookup, NameHash& value) noexcept {
        if (value.value != 0) {
            if (auto i = lookup.find(value.value); i != lookup.end()) {
                value.unhashed = i->second;
            }
        }
    }

    inline void unhash(LookupTable const& lookup, StringHash& value) noexcept {
        if (value.value != 0) {
            if (auto i = lookup.find(value.value); i != lookup.end()) {
                value.unhashed = i->second;
            }
        }
    }

    inline void unhash(LookupTable const& lookup, Link& value) noexcept {
        unhash(lookup, value.value);
    }

    inline void unhash(LookupTable const& lookup, Hash& value) noexcept {
        unhash(lookup, value.value);
    }

    inline void unhash(LookupTable const& lookup, Embed& value) noexcept {
        unhash(lookup, value.value);
    }

    inline void unhash(LookupTable const& lookup, Pointer& value) noexcept {
        unhash(lookup, value.value);
    }

    inline void unhash(LookupTable const& lookup, Value& value) noexcept {
        std::visit([&lookup](auto&& value) {
            using value_t = std::remove_const_t<std::remove_reference_t<decltype(value)>>;
            unhash(lookup, value);
        }, value);
    }
    
    inline void unhash(LookupTable const& lookup, Item& node) noexcept {
        unhash(lookup, node.value);
    }

    inline void unhash(LookupTable const& lookup, Field& node) noexcept {
        unhash(lookup, node.name);
        unhash(lookup, node.value);
    }

    inline void unhash(LookupTable const& lookup, Pair& node) noexcept {
        unhash(lookup, node.key);
        unhash(lookup, node.value);
    }

    void lookup_unhash(LookupTable const& lookup, NodeList& nodes) noexcept {
        for (auto& node : nodes) {
            std::visit([&lookup](auto&& node) {
                using node_t = std::remove_const_t<std::remove_reference_t<decltype(node)>>;
                unhash(lookup, node);
            }, node);
        }
    }
}
