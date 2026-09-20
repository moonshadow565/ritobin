#include "bin_unhash_dynamic.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <stdexcept>
#include <vector>
#include <zstd.h>

namespace {
    enum class HashKind : uint8_t {
        // consumers fall back on key width: u64 -> xxh64, u32 -> fnv1a32
        UNKNOWN = 0,
        // game, lcu, rst
        XXH64 = 1,
        // binentries, bintypes, binfields, binhashes
        FNV1A = 2,
        // rst v5+
        XXH3 = 3,
    };

    struct MirmirHeader {
        static constexpr std::array<char, 8> MAGIC = {'H', 'A', 'S', 'H', 'D', 'B', '\0', '\0'};
        static constexpr uint16_t VERSION_MIN = 1;
        static constexpr uint16_t VERSION_MAX = 1;
        // arena is a zstd stream instead of raw bytes
        static constexpr uint8_t FLAG_ARENA_COMPRESSED = 0x1u;
        // keys hash the ascii lowercased path
        static constexpr uint8_t FLAG_CASE_INSENSITIVE = 0x2u;
        // any flag outside of this rejects the file
        static constexpr uint8_t FLAGS_KNOWN = FLAG_ARENA_COMPRESSED | FLAG_CASE_INSENSITIVE;

        // shoulde be equal to MAGIC
        std::array<char, 8> magic;
        // currently 1
        uint16_t version;
        // algorithm that produced the keys, see below
        HashKind hash_kind;
        // required flags: bit0 arena_compressed, bit1 case_insensitive (see below); any other bit set must reject the file
        uint8_t flags;
        // 4 = u32 table, 8 = u64 table
        uint8_t key_width;
        // 4 or 8; the writer picks 4 while arena_decompressed_size fits in a u32, else 8; the reader honors whatever is declared
        uint8_t offset_width;
        // optional flags: none defined yet; an unrecognized bit must be ignored
        uint8_t opt_flags;
        // bytes per entry in the arena-order section, 1..=8; 0 when there is none
        uint8_t arena_order_width;
        // number of entries in the table
        uint64_t entry_count;
        // file offset of the keys section; writers must 8-align it (the reference writer emits 80), readers bounds-check and honor the declared value
        uint64_t keys_offset;
        // file offset of the offsets section; writers must offset_width-align it
        uint64_t offsets_offset;
        // file offset of the arena
        uint64_t arena_offset;
        // raw (decompressed) arena length
        uint64_t arena_decompressed_size;
        // arena bytes on disk; == decompressed if raw
        uint64_t arena_compressed_size;
        // xxh3-64 of keys || offsets || lengths || arena, each as stored on disk (inter-section padding excluded)
        uint64_t checksum;
        // file offset of the arena-order section; 0 = the file carries none
        uint64_t arena_order_offset;
    };

    struct ZstdSeekFrame {
        uint32_t compressed_size;
        uint32_t decompressed_size;
        uint64_t compressed_offset;
        uint64_t decompressed_offset;
    };

    // The maximum number of entries that can be represented in a mirmir file, 2^32.
    // If you bump this, make sure that no multiplication following it overflows.
    constexpr uint64_t MIRMIR_MAX_ENTRY_COUNT = uint64_t{1} << 32;

    constexpr uint64_t MIRMIR_FOOTER_SIZE = 9;
    constexpr uint32_t MIRMIR_SKIPPABLE_MAGIC = 0x184D2A5Eu;
    constexpr uint32_t MIRMIR_SEEKABLE_MAGIC = 0x8F92EAB1u;
    constexpr uint64_t MIRMIR_SKIPPABLE_HEADER_SIZE = 8;
    constexpr uint64_t MIRMIR_FRAME_DECOMPRESSED_MAX = 1ull << 30;

    static_assert(offsetof(MirmirHeader, magic) == 0);
    static_assert(offsetof(MirmirHeader, version) == 8);
    static_assert(offsetof(MirmirHeader, hash_kind) == 10);
    static_assert(offsetof(MirmirHeader, flags) == 11);
    static_assert(offsetof(MirmirHeader, key_width) == 12);
    static_assert(offsetof(MirmirHeader, offset_width) == 13);
    static_assert(offsetof(MirmirHeader, opt_flags) == 14);
    static_assert(offsetof(MirmirHeader, arena_order_width) == 15);
    static_assert(offsetof(MirmirHeader, entry_count) == 16);
    static_assert(offsetof(MirmirHeader, keys_offset) == 24);
    static_assert(offsetof(MirmirHeader, offsets_offset) == 32);
    static_assert(offsetof(MirmirHeader, arena_offset) == 40);
    static_assert(offsetof(MirmirHeader, arena_decompressed_size) == 48);
    static_assert(offsetof(MirmirHeader, arena_compressed_size) == 56);
    static_assert(offsetof(MirmirHeader, checksum) == 64);
    static_assert(offsetof(MirmirHeader, arena_order_offset) == 72);
    static_assert(sizeof(MirmirHeader) == 80);

    // whether [offset, offset + size) fits within a file of file_size bytes
    bool in_bounds(uint64_t offset, uint64_t size, uint64_t file_size) noexcept {
        return offset <= file_size && size <= file_size - offset;
    }

    // a section of little endian integers, as the width its header field declared
    template <typename T>
    std::span<T const> section_as(std::span<char const> data) noexcept {
        return {reinterpret_cast<T const*>(data.data()), data.size() / sizeof(T)};
    }
}

namespace ritobin {
    struct BinUnhasherDynamic::Mirmir final : BinUnhasherDynamic {
    private:
        void* mapped_impl = nullptr;
        ZSTD_DCtx* dctx = nullptr;
        HashKind hash_kind = HashKind::UNKNOWN;
        std::span<char const> mapped = {};
        std::span<char const> keys = {};
        std::span<char const> offsets = {};
        std::span<uint16_t const> lengths = {};
        uint8_t key_width = 0;
        uint8_t offset_width = 0;
        std::span<char const> arena = {};
        uint64_t arena_decompressed_size = 0;
        std::vector<ZstdSeekFrame> frames = {};
        mutable std::vector<char> frame = {};
        mutable size_t frame_index = std::string::npos;
    public:
        void destroy() const noexcept override;
        std::string unhash_hash_fnv1a(uint32_t hash) const noexcept override;
        std::string unhash_hash_xxh64(uint64_t hash) const noexcept override;
        std::errc load(std::string const& filename) noexcept;
    private:
        std::errc mmap_impl(std::string const& filename) noexcept;
        void unmmap_impl() const noexcept;

        template <typename KeyT>
        std::string lookup(KeyT hash) const noexcept;

        // index of the frame holding offset, frames.size() when there is none
        size_t frame_of(uint64_t offset) const noexcept;

        // decompress frames[index] into frame, which is kept for the next lookup
        bool decompress_frame(size_t index) const noexcept;
    };

    void BinUnhasherDynamic::Mirmir::destroy() const noexcept {
        unmmap_impl();
        if (dctx) {
            ZSTD_freeDCtx(dctx);
        }
        delete this;
    }

    std::errc BinUnhasherDynamic::Mirmir::load(std::string const& filename) noexcept {
        if (auto const ec = mmap_impl(filename); ec != std::errc{}) {
            return ec;
        }
        if (mapped.size() < sizeof(MirmirHeader)) {
            return std::errc::illegal_byte_sequence;
        }
        auto header = MirmirHeader{};
        std::memcpy(&header, mapped.data(), sizeof(header));
        if (header.magic != MirmirHeader::MAGIC) {
            return std::errc::illegal_byte_sequence;
        }
        if (header.version < MirmirHeader::VERSION_MIN || header.version > MirmirHeader::VERSION_MAX) {
            return std::errc::not_supported;
        }
        if (header.flags & ~MirmirHeader::FLAGS_KNOWN) {
            return std::errc::not_supported;
        }
        if (header.key_width != 4 && header.key_width != 8) {
            return std::errc::not_supported;
        }
        if (header.offset_width != 4 && header.offset_width != 8) {
            return std::errc::not_supported;
        }
        if (header.entry_count > MIRMIR_MAX_ENTRY_COUNT) {
            return std::errc::value_too_large;
        }
        switch (header.hash_kind) {
        case HashKind::UNKNOWN:
            hash_kind = header.key_width == 8 ? HashKind::XXH64 : HashKind::FNV1A;
            break;
        case HashKind::FNV1A:
            if (header.key_width != 4) {
                return std::errc::not_supported;
            }
            hash_kind = header.hash_kind;
            break;
        case HashKind::XXH64:
        case HashKind::XXH3:
            if (header.key_width != 8) {
                return std::errc::not_supported;
            }
            hash_kind = header.hash_kind;
            break;
        default:
            return std::errc::not_supported;
        }

        auto const compressed = (header.flags & MirmirHeader::FLAG_ARENA_COMPRESSED) != 0;
        if (!compressed && header.arena_compressed_size != header.arena_decompressed_size) {
            return std::errc::illegal_byte_sequence;
        }

        auto const keys_size = header.entry_count * header.key_width;
        auto const offsets_size = header.entry_count * header.offset_width;
        auto const lengths_size = header.entry_count * sizeof(uint16_t);

        // the lengths section has no header field, it sits right after the offsets
        auto const lengths_offset = header.offsets_offset + offsets_size;
        if (!in_bounds(header.keys_offset, keys_size, mapped.size())) {
            return std::errc::illegal_byte_sequence;
        }
        if (!in_bounds(header.offsets_offset, offsets_size, mapped.size())) {
            return std::errc::illegal_byte_sequence;
        }
        if (!in_bounds(lengths_offset, lengths_size, mapped.size())) {
            return std::errc::illegal_byte_sequence;
        }
        if (!in_bounds(header.arena_offset, header.arena_compressed_size, mapped.size())) {
            return std::errc::illegal_byte_sequence;
        }

        key_width = header.key_width;
        offset_width = header.offset_width;
        keys = mapped.subspan(static_cast<size_t>(header.keys_offset), static_cast<size_t>(keys_size));
        offsets = mapped.subspan(static_cast<size_t>(header.offsets_offset), static_cast<size_t>(offsets_size));
        lengths = {reinterpret_cast<uint16_t const*>(mapped.data() + lengths_offset), static_cast<size_t>(header.entry_count)};
        arena = mapped.subspan(static_cast<size_t>(header.arena_offset), static_cast<size_t>(header.arena_compressed_size));
        arena_decompressed_size = header.arena_decompressed_size;

        if (!compressed) {
            return std::errc{};
        }

        auto const read_u32 = [](char const* data) noexcept {
            auto value = uint32_t{};
            std::memcpy(&value, data, sizeof(value));
            return value;
        };

        if (arena.size() < MIRMIR_FOOTER_SIZE) {
            return std::errc::illegal_byte_sequence;
        }
        auto const footer = arena.data() + arena.size() - MIRMIR_FOOTER_SIZE;
        if (read_u32(footer + 5) != MIRMIR_SEEKABLE_MAGIC) {
            return std::errc::illegal_byte_sequence;
        }
        auto const descriptor = static_cast<uint8_t>(footer[4]);
        // bit7 adds a checksum to every entry, bits 6-2 are reserved and must be zero,
        // bits 1-0 are unused and must not be interpreted
        if (descriptor & 0x7Cu) {
            return std::errc::not_supported;
        }
        auto const frame_entry_size = uint64_t{descriptor & 0x80u ? 12u : 8u};
        auto const frame_count = read_u32(footer);
        auto const table_size = frame_count * frame_entry_size + MIRMIR_FOOTER_SIZE;
        if (arena.size() < table_size + MIRMIR_SKIPPABLE_HEADER_SIZE) {
            return std::errc::illegal_byte_sequence;
        }
        auto const table_offset = arena.size() - table_size - MIRMIR_SKIPPABLE_HEADER_SIZE;
        if (read_u32(arena.data() + table_offset) != MIRMIR_SKIPPABLE_MAGIC) {
            return std::errc::illegal_byte_sequence;
        }
        if (read_u32(arena.data() + table_offset + 4) != table_size) {
            return std::errc::illegal_byte_sequence;
        }

        // Create frame offset Lookup map.
        {
            auto const entries = arena.data() + table_offset + MIRMIR_SKIPPABLE_HEADER_SIZE;
            auto compressed_offset = uint64_t{};
            auto decompressed_offset = uint64_t{};
            try {
                frames.resize(frame_count);
            } catch (std::bad_alloc const&) {
                return std::errc::not_enough_memory;
            }
            for (auto i = uint32_t{}; i != frame_count; ++i) {
                auto const entry = entries + i * frame_entry_size;
                auto const frame_compressed = read_u32(entry);
                auto const frame_decompressed = read_u32(entry + 4);
                if (frame_decompressed > MIRMIR_FRAME_DECOMPRESSED_MAX) {
                    return std::errc::not_supported;
                }
                if (!in_bounds(compressed_offset, frame_compressed, table_offset)) {
                    return std::errc::illegal_byte_sequence;
                }
                frames[i].compressed_size = frame_compressed;
                frames[i].decompressed_size = frame_decompressed;
                frames[i].compressed_offset = compressed_offset;
                frames[i].decompressed_offset = decompressed_offset;
                compressed_offset += frame_compressed;
                decompressed_offset += frame_decompressed;
            }
            if (decompressed_offset != arena_decompressed_size) {
                return std::errc::illegal_byte_sequence;
            }
        }

        dctx = ZSTD_createDCtx();
        if (!dctx) {
            return std::errc::not_enough_memory;
        }
        return std::errc{};
    }

    std::string BinUnhasherDynamic::Mirmir::unhash_hash_fnv1a(uint32_t hash) const noexcept {
        if (hash_kind != HashKind::FNV1A) {
            return {};
        }
        // load only accepts fnv1a with a 4 byte key width
        return lookup<uint32_t>(hash);
    }

    std::string BinUnhasherDynamic::Mirmir::unhash_hash_xxh64(uint64_t hash) const noexcept {
        if (hash_kind != HashKind::XXH64) {
            return {};
        }
        // load only accepts xxh64 with an 8 byte key width
        return lookup<uint64_t>(hash);
    }

    template <typename KeyT>
    std::string BinUnhasherDynamic::Mirmir::lookup(KeyT hash) const noexcept try {
        auto const typed = section_as<KeyT>(keys);
        // a miss is decided here and never touches the arena
        auto const found = std::lower_bound(typed.begin(), typed.end(), hash);
        if (found == typed.end() || *found != hash) {
            return {};
        }
        auto const index = static_cast<size_t>(found - typed.begin());
        auto const offset = offset_width == 4 ? uint64_t{section_as<uint32_t>(offsets)[index]} : section_as<uint64_t>(offsets)[index];
        auto const length = lengths[index];
        // entry extents are not validated on load, every read checks its own
        if (!in_bounds(offset, length, arena_decompressed_size)) {
#ifndef NDEBUG
            throw std::out_of_range("mirmir: entry extent outside of the arena");
#endif
            return {};
        }
        if (frames.empty()) {
            return std::string(arena.data() + offset, length);
        }

        // the arena is cut into frames at fixed sizes, an entry can straddle two of them
        auto result = std::string(length, '\0');
        auto written = size_t{};
        auto index_frame = frame_of(offset);
        while (written != length) {
            if (index_frame == frames.size() || !decompress_frame(index_frame)) {
                return {};
            }
            auto const start = offset + written - frames[index_frame].decompressed_offset;
            if (start >= frame.size()) {
                return {};
            }
            auto const take = std::min<size_t>(frame.size() - start, length - written);
            std::memcpy(result.data() + written, frame.data() + start, take);
            written += take;
            ++index_frame;
        }
        return result;
    } catch (std::bad_alloc const&) {
        return {};
    }

    size_t BinUnhasherDynamic::Mirmir::frame_of(uint64_t offset) const noexcept {
        auto const starts_after = [](uint64_t value, ZstdSeekFrame const& candidate) noexcept {
            return value < candidate.decompressed_offset;
        };
        auto const found = std::upper_bound(frames.begin(), frames.end(), offset, starts_after);
        if (found == frames.begin()) {
            return frames.size();
        }
        return static_cast<size_t>(found - frames.begin()) - 1;
    }

    bool BinUnhasherDynamic::Mirmir::decompress_frame(size_t index) const noexcept try {
        if (frame_index == index) {
            return true;
        }
        frame_index = std::string::npos;
        auto const& current = frames[index];
        frame.resize(current.decompressed_size);
        auto const src = arena.data() + current.compressed_offset;
        auto const result = ZSTD_decompressDCtx(dctx, frame.data(), frame.size(), src, current.compressed_size);
        if (ZSTD_isError(result) || result != frame.size()) {
            return false;
        }
        frame_index = index;
        return true;
    } catch (std::bad_alloc const&) {
        return false;
    }

    BinUnhasherDynamic* BinUnhasherDynamic::create_mirmir(std::string const& filename, std::errc& ec) noexcept {
        auto result = new (std::nothrow) Mirmir();
        if (!result) {
            ec = std::errc::not_enough_memory;
            return nullptr;
        }
        if (auto const ec_load = result->load(filename); ec_load != std::errc{}) {
            ec = ec_load;
            result->destroy();
            return create_stub();
        }
        ec = std::errc{};
        return result;
    }
}

#define RITOBIN_IMPL_MIRMIR_REFERENCE 0
#define RITOBIN_IMPL_MIRMIR_WINDOWS 1
#define RITOBIN_IMPL_MIRMIR_UNIX 2
// uncomment to force reference implementation
// #define RITOBIN_IMPL_MIRMIR RITOBIN_IMPL_MIRMIR_REFERENCE

#ifndef RITOBIN_IMPL_MIRMIR
#if defined(_WIN32)
#define RITOBIN_IMPL_MIRMIR RITOBIN_IMPL_MIRMIR_WINDOWS
#elif defined(__unix__) || defined(__APPLE__)
#define RITOBIN_IMPL_MIRMIR RITOBIN_IMPL_MIRMIR_UNIX
#else
#define RITOBIN_IMPL_MIRMIR RITOBIN_IMPL_MIRMIR_REFERENCE
#endif
#endif

#if RITOBIN_IMPL_MIRMIR == RITOBIN_IMPL_MIRMIR_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

std::errc ritobin::BinUnhasherDynamic::Mirmir::mmap_impl(std::string const& filename) noexcept {
    auto const file = ::CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!file || file == INVALID_HANDLE_VALUE) {
        return std::errc::no_such_file_or_directory;
    }
    auto size = LARGE_INTEGER{};
    if (!::GetFileSizeEx(file, &size)) {
        ::CloseHandle(file);
        return std::errc::io_error;
    }
    // the mapping keeps the file alive, the view keeps the mapping alive
    auto const mapping = ::CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    ::CloseHandle(file);
    if (!mapping) {
        return std::errc::io_error;
    }
    auto const data = ::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    ::CloseHandle(mapping);
    if (!data) {
        return std::errc::io_error;
    }
    mapped = std::span<char const>(static_cast<char const*>(data), static_cast<size_t>(size.QuadPart));
    mapped_impl = data;
    return std::errc{};
}

void ritobin::BinUnhasherDynamic::Mirmir::unmmap_impl() const noexcept {
    if (mapped_impl) {
        ::UnmapViewOfFile(mapped_impl);
    }
}

#elif RITOBIN_IMPL_MIRMIR == RITOBIN_IMPL_MIRMIR_UNIX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

std::errc ritobin::BinUnhasherDynamic::Mirmir::mmap_impl(std::string const& filename) noexcept {
    auto const fd = ::open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
        return std::errc::no_such_file_or_directory;
    }
    struct ::stat info = {};
    if (::fstat(fd, &info) == -1) {
        ::close(fd);
        return std::errc::io_error;
    }
    auto const size = static_cast<size_t>(info.st_size);
    // the mapping keeps the file alive
    auto const data = ::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    ::close(fd);
    if (data == MAP_FAILED) {
        return std::errc::io_error;
    }
    mapped = std::span<char const>(static_cast<char const*>(data), size);
    mapped_impl = data;
    return std::errc{};
}

void ritobin::BinUnhasherDynamic::Mirmir::unmmap_impl() const noexcept {
    if (mapped_impl) {
        ::munmap(mapped_impl, mapped.size());
    }
}

#else
#include <fstream>

std::errc ritobin::BinUnhasherDynamic::Mirmir::mmap_impl(std::string const& filename) noexcept {
    auto file = std::ifstream(filename, std::ios::binary);
    if (!file) {
        return std::errc::no_such_file_or_directory;
    }
    file.seekg(0, std::ios::end);
    auto const end = file.tellg();
    file.seekg(0, std::ios::beg);
    auto const beg = file.tellg();
    auto const size = static_cast<size_t>(end - beg);
    char* mapped_storage = nullptr;
    try {
        mapped_storage = new char[size];
    } catch (std::bad_alloc const&) {
        return std::errc::not_enough_memory;
    }
    if (!file.read(mapped_storage, static_cast<std::streamsize>(size))) {
        delete[] mapped_storage;
        return std::errc::io_error;
    }
    mapped = std::span<char const>(mapped_storage, size);
    mapped_impl = mapped_storage;
    return std::errc{};
}

void ritobin::BinUnhasherDynamic::Mirmir::unmmap_impl() const noexcept {
    if (mapped_impl) {
        delete[] static_cast<char*>(mapped_impl);
    }
}

#endif
