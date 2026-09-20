#include "bin_unhash_dynamic.hpp"

namespace ritobin {
    BinUnhasherDynamic::~BinUnhasherDynamic() noexcept = default;
    void BinUnhasherDynamic::destroy() const noexcept {}

    struct BinUnhasherDynamic::Stub final : BinUnhasherDynamic {
        std::vector<std::unique_ptr<BinUnhasherDynamic, BinUnhasherBaseDeleter>> dynamic;

        void destroy() const noexcept override {
        }

        std::string unhash_hash_fnv1a(uint32_t hash) const noexcept override {
            for (const auto& d: dynamic) {
                if (auto result = d->unhash_hash_fnv1a(hash); !result.empty()) {
                    return result;
                }
            }
            return {};
        }

        std::string unhash_hash_xxh64(uint64_t hash) const noexcept override {
            for (const auto& d: dynamic) {
                if (auto result = d->unhash_hash_xxh64(hash); !result.empty()) {
                    return result;
                }
            }
            return {};
        }
    };

    BinUnhasherDynamic* BinUnhasherDynamic::create_stub() noexcept {
        static BinUnhasherDynamic::Stub stub_instance;
        return &stub_instance;
    }
}
