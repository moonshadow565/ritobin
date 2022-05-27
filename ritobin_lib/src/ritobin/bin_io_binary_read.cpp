#include "bin_io.hpp"
#include "bin_types_helper.hpp"

#define bin_assert(...) do { \
    if (auto start = reader.cur_; !(__VA_ARGS__)) { \
        return fail_msg(#__VA_ARGS__, start); \
    } } while(false)

namespace ritobin::io::impl_binary_read {
    struct BinaryReader {
        char const* const beg_;
        char const* cur_;
        char const* const cap_;
        BinCompat const* const compat_;

        inline constexpr size_t position() const noexcept {
            return cur_ - beg_;
        }

        template<typename T>
        inline bool read(T& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            if (cur_ + sizeof(T) > cap_) {
                return false;
            }
            memcpy(&value, cur_, sizeof(T));
            cur_ += sizeof(T);
            return true;
        }

        template<typename T, size_t SIZE>
        bool read(std::array<T, SIZE>& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            if (cur_ + sizeof(T) * SIZE > cap_) {
                return false;
            }
            memcpy(value.data(), cur_, sizeof(T) * SIZE);
            cur_ += sizeof(T) * SIZE;
            return true;
        }

        template<typename T>
        bool read(std::vector<T>& value, size_t size) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            if (cur_ + sizeof(T) * size > cap_) {
                return false;
            }
            value.resize(size);
            memcpy(value.data(), cur_, sizeof(T) * size);
            cur_ += sizeof(T) * size;
            return true;
        }

        bool read(std::string& value) noexcept {
            uint16_t size = {};
            if (!read(size)) {
                return false;
            }
            if (cur_ + size > cap_) {
                return false;
            }
            value = { cur_, size };
            cur_ += size;
            return true;
        }

        bool read(Type& value) noexcept {
            uint8_t raw = {};
            if (!read(raw)) {
                return false;
            }
            return compat_->raw_to_type(raw, value);
        }

        bool read(FNV1a& value) noexcept {
            uint32_t h;
            if (!read(h)) {
                return false;
            }
            value = FNV1a{ h };
            return true;
        }

        bool read(XXH64& value) noexcept {
            uint64_t h;
            if (!read(h)) {
                return false;
            }
            value = XXH64{ h };
            return true;
        }
    };

    struct BinBinaryReader {
        Bin& bin;
        BinaryReader reader;
        std::vector<std::pair<std::string, char const*>> error;

        bool process() noexcept {
            bin.sections.clear();
            bin_assert(read_sections());
            return true;
        }

    private:
        bool fail_msg(char const* msg, char const* pos) noexcept {
            error.emplace_back(msg, pos);
            return false;
        }

        bool read_sections() noexcept {
            std::array<char, 4> magic = {};
            uint32_t version = 0;
            bin_assert(reader.read(magic));
            bool is_patch = false;
            if (magic == std::array{ 'P', 'T', 'C', 'H' }) {
                uint64_t unk = {};
                bin_assert(reader.read(unk));
                bin_assert(reader.read(magic));
                bin.sections.emplace("type", String{ "PTCH" });
                is_patch = true;
            } else {
                bin.sections.emplace("type", String{ "PROP" });
            }
            bin_assert(magic == std::array{ 'P', 'R', 'O', 'P' });
            bin_assert(reader.read(version));
            bin.sections.emplace("version", U32{ version });

            if (version >= 2) {
                bin_assert(read_linked());
            }
            bin_assert(read_entries());
            if (is_patch /*&& version >= 3*/) {
                bin_assert(read_patches());
            }

            bin_assert(reader.cur_ == reader.cap_);
            return true;
        }

        bool read_linked() noexcept {
            List linkedList = { Type::STRING, {} };
            uint32_t linkedFilesCount = {};
            bin_assert(reader.read(linkedFilesCount));
            for (uint32_t i = 0; i != linkedFilesCount; i++) {
                String linked = {};
                bin_assert(reader.read(linked.value));
                linkedList.items.emplace_back(Element{ linked });
            }
            bin.sections.emplace("linked", std::move(linkedList));
            return true;
        }

        bool read_entries() noexcept {
            uint32_t entryCount = 0;
            std::vector<uint32_t> entryNameHashes;
            bin_assert(reader.read(entryCount));
            bin_assert(reader.read(entryNameHashes, entryCount));
            Map entriesMap = { Type::HASH,  Type::EMBED, {} };
            for (uint32_t entryNameHash : entryNameHashes) {
                Hash entryKeyHash = {};
                Embed entry = { { entryNameHash }, {} };
                bin_assert(read_entry(entryKeyHash, entry));
                entriesMap.items.emplace_back(Pair{ std::move(entryKeyHash), std::move(entry) });
            }
            bin.sections.emplace("entries", std::move(entriesMap));
            return true;
        }

        bool read_entry(Hash& entryKeyHash, Embed& entry) noexcept {
            uint32_t entryLength = 0;
            uint16_t count = 0;
            bin_assert(reader.read(entryLength));
            size_t position = reader.position();
            bin_assert(reader.read(entryKeyHash.value));
            bin_assert(reader.read(count));
            for (size_t i = 0; i != count; i++) {
                auto& [name, item] = entry.items.emplace_back();
                Type type = {};
                bin_assert(reader.read(name));
                bin_assert(reader.read(type));
                bin_assert(read_value_of(item, type));
            }
            bin_assert(reader.position() == position + entryLength);
            return true;
        }

        bool read_patches() noexcept {
            uint32_t patchCount = {};
            bin_assert(reader.read(patchCount));
            Map patchMap = { Type::HASH,  Type::EMBED, {} };
            for (size_t i = {}; i != patchCount; i++) {
                Hash entryKeyHash = {};
                Embed entry = { { "patch" }, {} };
                bin_assert(read_patch(entryKeyHash, entry));
                patchMap.items.emplace_back(Pair{ std::move(entryKeyHash), std::move(entry) });
            }
            bin.sections.emplace("patches", std::move(patchMap));
            return true;
        }

        bool read_patch(Hash& patchKeyHash, Embed& patch) {
            uint32_t patchLength = 0;
            bin_assert(reader.read(patchKeyHash.value));
            bin_assert(reader.read(patchLength));
            auto position = reader.position();
            Type type = {};
            String name = {};
            Value value = {};
            bin_assert(reader.read(type));
            bin_assert(reader.read(name.value));
            bin_assert(read_value_of(value, type));
            bin_assert(reader.position() == position + patchLength);
            patch.items.emplace_back(Field { {"path"}, std::move(name) });
            patch.items.emplace_back(Field { {"value"}, std::move(value) });
            return true;
        }

        bool read_value_of(Value& value, Type type) noexcept {
            value = ValueHelper::type_to_value(type);
            return std::visit([this](auto&& value) {
                return read_value_visit(std::forward<decltype(value)>(value));
            }, value);
        }

        bool read_value_visit(None&) noexcept { 
            bin_assert(false);
            return true;
        }

        bool read_value_visit(Embed& value) noexcept {
            uint32_t size = 0;
            uint16_t count = 0;
            bin_assert(reader.read(value.name));
            bin_assert(reader.read(size));
            size_t position = reader.position();
            bin_assert(reader.read(count));
            for (size_t i = 0; i != count; i++) {
                auto& [name, item] = value.items.emplace_back();
                Type type;
                bin_assert(reader.read(name));
                bin_assert(reader.read(type));
                bin_assert(read_value_of(item, type));
            }
            bin_assert(reader.position() == position + size);
            return true;
        }

        bool read_value_visit(Pointer& value) noexcept {
            uint32_t size = 0;
            uint16_t count = 0;
            bin_assert(reader.read(value.name));
            if (value.name.hash() == 0) {
                return true;
            }
            bin_assert(reader.read(size));
            size_t position = reader.position();
            bin_assert(reader.read(count));
            for (size_t i = 0; i != count; i++) {
                auto& [name, item] = value.items.emplace_back();
                Type type = {};
                bin_assert(reader.read(name));
                bin_assert(reader.read(type));
                bin_assert(read_value_of(item, type));
            }
            bin_assert(reader.position() == position + size);
            return true;
        }

        bool read_value_visit(Option& value) noexcept {
            uint8_t count = 0;
            bin_assert(reader.read(value.valueType));
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(reader.read(count));
            if (count != 0) {
                auto& [item] = value.items.emplace_back();
                bin_assert(read_value_of(item, value.valueType));
            }
            return true;
        }

        bool read_value_visit(List& value) noexcept {
            uint32_t size = 0;
            uint32_t count = 0;
            bin_assert(reader.read(value.valueType));
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(reader.read(size));
            size_t position = reader.position();
            bin_assert(reader.read(count));
            for (size_t i = 0; i != count; i++) {
                auto& [item] = value.items.emplace_back();
                bin_assert(read_value_of(item, value.valueType));
            }
            bin_assert(reader.position() == position + size);
            return true;
        }

        bool read_value_visit(List2& value) noexcept {
            uint32_t size = 0;
            uint32_t count = 0;
            bin_assert(reader.read(value.valueType));
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(reader.read(size));
            size_t position = reader.position();
            bin_assert(reader.read(count));
            for (size_t i = 0; i != count; i++) {
                auto& [item] = value.items.emplace_back();
                bin_assert(read_value_of(item, value.valueType));
            }
            bin_assert(reader.position() == position + size);
            return true;
        }

        bool read_value_visit(Map& value) noexcept {
            uint32_t size = 0;
            uint32_t count = 0;
            bin_assert(reader.read(value.keyType));
            bin_assert(ValueHelper::is_primitive(value.keyType));
            bin_assert(reader.read(value.valueType));
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(reader.read(size));
            size_t position = reader.position();
            bin_assert(reader.read(count));
            for (size_t i = 0; i != count; i++) {
                auto& [key, item] = value.items.emplace_back();
                bin_assert(read_value_of(key, value.keyType));
                bin_assert(read_value_of(item, value.valueType));
            }
            bin_assert(reader.position() == position + size);
            return true;
        }
        
        template<typename T>
        bool read_value_visit(T& value) noexcept {
            bin_assert(reader.read(value.value));
            return true;
        }
    public:
        std::string trace_error() noexcept {
            std::string trace;
            for(auto e = error.crbegin(); e != error.crend(); e++) {
                trace.append(e->first);
                trace.append(" @ ");
                trace.append(std::to_string(reader.beg_ - e->second));
                trace.append("\n");
            }
            return trace;
        }
    };
}

namespace ritobin::io {
    using namespace impl_binary_read;

    std::string read_binary(Bin& value, std::span<char const> data, BinCompat const* compat) noexcept {
        auto const begin = data.data();
        auto const end = data.data() + data.size();
        BinBinaryReader reader = { value, { begin, begin, end, compat }, {} };
        if (!reader.process()) {
            return reader.trace_error();
        }
        return {};
    }
}
