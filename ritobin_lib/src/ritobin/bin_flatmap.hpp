#pragma once
#include <vector>
#include <utility>
#include <stdexcept>

namespace ritobin {
    template <typename K, typename V>
    struct flatmap {
        auto begin() { return values_.begin(); }
        auto end() { return values_.end(); }
        const auto begin() const { return values_.begin(); }
        const auto end() const { return values_.end(); }
        const auto cbegin() const { return values_.cbegin(); }
        const auto cend() const { return values_.cend(); }

        template <typename U>
        V& operator[] (U&& what) {
            for (auto& i: values_) if (i.first == what) return i.second;
            return values_.emplace_back(std::forward<U>(what), V{}).second;
        }

        template <typename U>
        const V& operator[] (U&& what) const {
            for (auto& i: values_) if (i.first == what) return i.second;
            throw std::out_of_range("flatmap::operator[]");
        }

        template <typename U>
        auto find(U&& what) {
            auto i = this->begin();
            const auto e = this->end();
            while (i != e && i->first != what) ++i;
            return i;
        }

        template <typename U>
        auto find(U&& what) const {
            auto i = this->begin();
            const auto e = this->end();
            while (i != e && i->first != what) ++i;
            return i;
        }

        template <typename UK, typename UV>
        auto& emplace(UK&& key, UV&& value) {
            auto i = find(key);
            if (i != this->end()) {
                i->second = std::forward<UV>(value);
                return *i;
            }
            return values_.emplace_back(std::forward<UK>(key), std::forward<UV>(value));
        }

        void clear() {
            values_.clear();
        }
    private:
        std::vector<std::pair<K, V>> values_;
    };
}
