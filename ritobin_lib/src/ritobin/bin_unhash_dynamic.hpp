#pragma once

#include "bin_types.hpp"

#include <memory>
#include <optional>
#include <system_error>

namespace ritobin {
    struct BinUnhasherDynamic {
    protected:
        BinUnhasherDynamic() noexcept = default;

        struct Mirmir;
        struct Lhdb;
        struct Stub;

        static BinUnhasherDynamic* create_stub() noexcept;
    public:
        virtual ~BinUnhasherDynamic() noexcept = 0;
        virtual void destroy() const noexcept = 0;
        virtual std::string unhash_hash_fnv1a(uint32_t hash) const noexcept = 0;
        virtual std::string unhash_hash_xxh64(uint64_t hash) const noexcept = 0;

        static BinUnhasherDynamic* create_mirmir(std::string const& filename, std::errc& ec) noexcept;
        static BinUnhasherDynamic* create_lhdb(std::string const& filename, std::errc& ec) noexcept;
    };

    struct BinUnhasherBaseDeleter {
        void operator()(BinUnhasherDynamic* ptr) const noexcept {
            if (ptr) {
                ptr->destroy();
            }
        }
    };
}
