#include "bin_unhash_dynamic.hpp"

#include <array>
#include <fstream>
#include <vector>
#include <json.hpp>

namespace {
    using json = nlohmann::json;

    constexpr uint64_t LHDB_SCHEMA_OLDEST = 1;
    constexpr uint64_t LHDB_SCHEMA_SUPPORTED = 1;
    constexpr uint64_t LHDB_MANIFEST_SIZE_MAX = 16 * 1024 * 1024;

    // first hit wins, same precedence as loading the CDTB lists in order
    constexpr std::array<char const*, 4> LHDB_TABLES_FNV1A = {"binfields", "bintypes", "binhashes", "binentries"};
    constexpr std::array<char const*, 2> LHDB_TABLES_XXH64 = {"game", "lcu"};
}

namespace ritobin {
    struct BinUnhasherDynamic::Lhdb final : BinUnhasherDynamic {
    private:
        using Table = std::unique_ptr<BinUnhasherDynamic, BinUnhasherBaseDeleter>;
        std::vector<Table> fnv1a = {};
        std::vector<Table> xxh64 = {};
    public:
        void destroy() const noexcept override;
        std::string unhash_hash_fnv1a(uint32_t hash) const noexcept override;
        std::string unhash_hash_xxh64(uint64_t hash) const noexcept override;
        std::errc load(std::string const& dirname) noexcept;
    private:
        std::errc open_table(json const& tables, std::string const& dirname, char const* name, std::vector<Table>& out);
    };

    void BinUnhasherDynamic::Lhdb::destroy() const noexcept {
        delete this;
    }

    std::string BinUnhasherDynamic::Lhdb::unhash_hash_fnv1a(uint32_t hash) const noexcept {
        for (auto const& table : fnv1a) {
            if (auto result = table->unhash_hash_fnv1a(hash); !result.empty()) {
                return result;
            }
        }
        return {};
    }

    std::string BinUnhasherDynamic::Lhdb::unhash_hash_xxh64(uint64_t hash) const noexcept {
        for (auto const& table : xxh64) {
            if (auto result = table->unhash_hash_xxh64(hash); !result.empty()) {
                return result;
            }
        }
        return {};
    }

    std::errc BinUnhasherDynamic::Lhdb::load(std::string const& dirname) noexcept try {
        auto file = std::ifstream(dirname + "/manifest.json", std::ios::binary);
        if (!file) {
            return std::errc::no_such_file_or_directory;
        }
        file.seekg(0, std::ios::end);
        auto const end = file.tellg();
        file.seekg(0, std::ios::beg);
        auto const beg = file.tellg();
        auto const size = static_cast<uint64_t>(end - beg);
        if (size > LHDB_MANIFEST_SIZE_MAX) {
            return std::errc::value_too_large;
        }
        auto text = std::string(static_cast<size_t>(size), '\0');
        if (!file.read(text.data(), static_cast<std::streamsize>(size))) {
            return std::errc::io_error;
        }
        file.close();

        auto const manifest = json::parse(text, nullptr, false);
        if (!manifest.is_object()) {
            return std::errc::illegal_byte_sequence;
        }
        auto const schema = manifest.find("schema");
        if (schema == manifest.end() || !schema->is_number_unsigned() || schema->get<uint64_t>() < LHDB_SCHEMA_OLDEST) {
            return std::errc::not_supported;
        }
        // a newer schema alone is fine, it only ever adds fields, this is the opt out
        auto const required = manifest.find("min_reader_schema");
        if (required != manifest.end() && required->is_number_unsigned() && required->get<uint64_t>() > LHDB_SCHEMA_SUPPORTED) {
            return std::errc::not_supported;
        }
        auto const tables = manifest.find("tables");
        if (tables == manifest.end() || !tables->is_object()) {
            return std::errc::illegal_byte_sequence;
        }

        // a table that is missing or does not validate costs only its own hashes
        auto ec_first = std::errc{};
        for (auto const name : LHDB_TABLES_FNV1A) {
            if (auto const ec = open_table(*tables, dirname, name, fnv1a); ec_first == std::errc{}) {
                ec_first = ec;
            }
        }
        for (auto const name : LHDB_TABLES_XXH64) {
            if (auto const ec = open_table(*tables, dirname, name, xxh64); ec_first == std::errc{}) {
                ec_first = ec;
            }
        }
        if (fnv1a.empty() && xxh64.empty()) {
            return ec_first != std::errc{} ? ec_first : std::errc::no_such_file_or_directory;
        }
        return std::errc{};
    } catch (std::bad_alloc const&) {
        return std::errc::not_enough_memory;
    }

    std::errc BinUnhasherDynamic::Lhdb::open_table(json const& tables, std::string const& dirname, char const* name, std::vector<Table>& out) {
        auto const entry = tables.find(name);
        if (entry == tables.end()) {
            return std::errc{};
        }
        if (!entry->is_object()) {
            return std::errc::illegal_byte_sequence;
        }
        auto const file = entry->find("file");
        if (file == entry->end() || !file->is_string()) {
            return std::errc::illegal_byte_sequence;
        }
        // a single path component, the manifest does not get to name files outside of the directory
        auto const& filename = file->get_ref<std::string const&>();
        if (filename.empty() || filename.find_first_of("/\\:") != std::string::npos) {
            return std::errc::illegal_byte_sequence;
        }
        auto ec = std::errc{};
        auto table = Table{create_mirmir(dirname + "/" + filename, ec)};
        if (ec != std::errc{}) {
            return ec;
        }
        out.push_back(std::move(table));
        return std::errc{};
    }

    BinUnhasherDynamic* BinUnhasherDynamic::create_lhdb(std::string const& dirname, std::errc& ec) noexcept {
        auto result = new (std::nothrow) Lhdb();
        if (!result) {
            ec = std::errc::not_enough_memory;
            return nullptr;
        }
        if (auto const ec_load = result->load(dirname); ec_load != std::errc{}) {
            ec = ec_load;
            result->destroy();
            return create_stub();
        }
        ec = std::errc{};
        return result;
    }
}
