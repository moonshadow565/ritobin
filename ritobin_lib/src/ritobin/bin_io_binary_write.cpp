#include "bin_io.hpp"
#include "bin_types_helper.hpp"
#include "bin_numconv.hpp"

#define bin_assert(...) do { \
    if(!(__VA_ARGS__)) { \
        return fail_msg(#__VA_ARGS__ "\n"); \
    } } while(false)


namespace ritobin::io::impl_binary_write {
    struct BinaryWriter {
        std::vector<char>& buffer_;
        BinCompat const* const compat_;

        bool write_at(size_t offset, size_t value) noexcept {
            auto const tmp = static_cast<uint32_t>(value);
            memcpy(buffer_.data() + offset, &tmp, sizeof(uint32_t));
            return true;
        }

        template<typename T, size_t S>
        void write(std::array<T, S> const& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            char buffer[sizeof(T) * S];
            memcpy(buffer, value.data(), sizeof(T) * S);
            buffer_.insert(buffer_.end(), buffer, buffer + sizeof(T) * S);
        }

        void write(std::vector<uint32_t> const& value, size_t offset) noexcept {
            auto const size = sizeof(uint32_t) * value.size();
            memcpy(buffer_.data() + offset, value.data(), size);
        }

        void skip(size_t size) noexcept {
            buffer_.insert(buffer_.end(), size, '\x00');
        }

        template<typename T>
        void write(T value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            char buffer[sizeof(value)];
            memcpy(buffer, &value, sizeof(value));
            buffer_.insert(buffer_.end(), buffer, buffer + sizeof(buffer));
        }

        void write(bool value) noexcept {
            write(static_cast<uint8_t>(value));
        }

        [[nodiscard]] bool write(Type type) noexcept {
            uint8_t raw = 0;
            if (compat_->type_to_raw(type, raw)) {
                write(raw);
                return true;
            } else {
                return false;
            }
        }

        void write(std::string const& value) noexcept {
            write(static_cast<uint16_t>(value.size()));
            buffer_.insert(buffer_.end(), value.data(), value.data() + value.size());
        }

        void write(FNV1a const& value) noexcept {
            write(value.hash());
        }

        void write(XXH64 const& value) noexcept {
            write(value.hash());
        }

        inline size_t position() const noexcept {
            return buffer_.size();
        }
    };

    struct BinBinaryWriter {
        BinaryWriter writer;
        std::vector<std::string> error;

        bool process(Bin const& bin) noexcept {
            error.clear();
            writer.buffer_.clear();
            bin_assert(write_sections(bin));
            return true;
        }

    private:
        bool fail_msg(char const* msg) noexcept {
            error.emplace_back(msg);
            return false;
        }

        bool write_sections(Bin const& bin) noexcept {
            auto type_section = bin.sections.find("type");
            bin_assert(type_section != bin.sections.end());
            auto type = std::get_if<String>(&type_section->second);
            bin_assert(type);
            bin_assert(type->value == "PROP" || type->value == "PTCH");
            if (type->value == "PTCH") {
                writer.write(std::array{ 'P', 'T', 'C', 'H' });
                writer.write(uint32_t{ 1 });
                writer.write(uint32_t{ 0 });
            }
            writer.write(std::array{ 'P', 'R', 'O', 'P' });


            auto version_section = bin.sections.find("version");
            bin_assert(version_section != bin.sections.end());
            auto version = std::get_if<U32>(&version_section->second);
            bin_assert(version);
            writer.write(uint32_t{ version->value });

            if (version->value >= 2) {
                bin_assert(write_links(bin));
            }
            bin_assert(write_entries(bin));
            if (version->value >= 3 && type->value == "PTCH") {
                bin_assert(write_patches(bin));
            }

            return true;
        }

        bool write_links(Bin const& bin) noexcept {
            auto linked_section = bin.sections.find("linked");
            if (linked_section == bin.sections.end()) {
                writer.write(static_cast<uint32_t>(0));
                return true;
            }
            auto linked = std::get_if<List>(&linked_section->second);
            bin_assert(linked);
            bin_assert(linked->valueType == Type::STRING);
            writer.write(static_cast<uint32_t>(linked->items.size()));
            for (auto const& [item] : linked->items) {
                auto link = std::get_if<String>(&item);
                bin_assert(link);
                writer.write(link->value);
            }
            return true;
        }
    
        bool write_entries(Bin const& bin) noexcept {
            auto entries_section = bin.sections.find("entries");
            if (entries_section == bin.sections.end()) {
                writer.write(static_cast<uint32_t>(0));
                return true;
            }
            auto entries = std::get_if<Map>(&entries_section->second);
            bin_assert(entries);
            bin_assert(entries->keyType == Type::HASH);
            bin_assert(entries->valueType == Type::EMBED);

            writer.write(static_cast<uint32_t>(entries->items.size()));

            std::vector<uint32_t> entryNameHashes;
            size_t entryNameHashes_offset = writer.position();
            writer.skip(sizeof(uint32_t) * entries->items.size());
            entryNameHashes.reserve(entries->items.size());

            for (auto const& [entryKey, entryValue] : entries->items) {
                auto key = std::get_if<Hash>(&entryKey);
                auto value = std::get_if<Embed>(&entryValue);
                bin_assert(key);
                bin_assert(value);
                entryNameHashes.push_back(value->name.hash());
                bin_assert(write_entry(*key, *value));
            }
            writer.write(entryNameHashes, entryNameHashes_offset);
            return true;
        }

        bool write_entry(Hash const& entryKey, Embed const& entryValue) noexcept {
            size_t position = writer.position();
            writer.write(uint32_t{});
            writer.write(uint32_t{ entryKey.value.hash() });
            writer.write(static_cast<uint16_t>(entryValue.items.size()));
            for (auto const& [name, item] : entryValue.items) {
                writer.write(name.hash());
                bin_assert(writer.write(ValueHelper::value_to_type(item)));
                bin_assert(write_value(item));
            }
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }


        bool write_patches(Bin const& bin) noexcept {
            auto patches_section = bin.sections.find("patches");
            if (patches_section == bin.sections.end()) {
                writer.write(static_cast<uint32_t>(0));
                return true;
            }
            auto patches = std::get_if<Map>(&patches_section->second);
            bin_assert(patches);
            bin_assert(patches->keyType == Type::HASH);
            bin_assert(patches->valueType == Type::EMBED);
            writer.write(static_cast<uint32_t>(patches->items.size()));
            for (auto const& [entryKey, entryValue] : patches->items) {
                auto key = std::get_if<Hash>(&entryKey);
                auto value = std::get_if<Embed>(&entryValue);
                bin_assert(key);
                bin_assert(value);
                bin_assert(write_patch(*key, *value));
            }
            return true;
        }

        bool write_patch(Hash const& patchKey, Embed const& patchValue) noexcept {
            writer.write(uint32_t{ patchKey.value.hash() });
            size_t position = writer.position();
            writer.write(uint32_t{});
            auto const name = patchValue.find_field({"path"});
            auto const value = patchValue.find_field({"value"});
            bin_assert(name);
            bin_assert(value);
            auto const nameType = ValueHelper::value_to_type(name->value);
            auto const valueType = ValueHelper::value_to_type(value->value);
            bin_assert(nameType == Type::STRING);
            bin_assert(writer.write(valueType));
            writer.write(std::get<String>(name->value).value);
            write_value(value->value);
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }

        bool write_value_visit(Embed const& value) noexcept {
            writer.write(value.name);
            size_t position = writer.position();
            writer.write(uint32_t{ 0 });
            writer.write(static_cast<uint16_t>(value.items.size()));
            for (auto const& [name, item] : value.items) {
                writer.write(name.hash());
                bin_assert(writer.write(ValueHelper::value_to_type(item)));
                bin_assert(write_value(item));
            }
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }

        bool write_value_visit(Pointer const& value) noexcept {
            writer.write(value.name);
            if (value.name.hash() == 0) {
                return true;
            }
            size_t position = writer.position();
            writer.write(uint32_t{ 0 });
            writer.write(static_cast<uint16_t>(value.items.size()));
            for (auto const& [name, item] : value.items) {
                writer.write(name.hash());
                bin_assert(writer.write(ValueHelper::value_to_type(item)));
                bin_assert(write_value(item));
            }
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }

        bool write_value_visit(List const& value) noexcept {
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(writer.write(value.valueType));
            size_t position = writer.position();
            writer.write(uint32_t{ 0 });
            writer.write(static_cast<uint32_t>(value.items.size()));
            for (auto const& [item] : value.items) {
                bin_assert(write_value(item, value.valueType));
            }
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }

        bool write_value_visit(List2 const& value) noexcept {
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(writer.write(value.valueType));
            size_t position = writer.position();
            writer.write(uint32_t{ 0 });
            writer.write(static_cast<uint32_t>(value.items.size()));
            for (auto const& [item] : value.items) {
                bin_assert(write_value(item, value.valueType));
            }
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }

        bool write_value_visit(Map const& value) noexcept {
            bin_assert(ValueHelper::is_primitive(value.keyType));
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(writer.write(value.keyType));
            bin_assert(writer.write(value.valueType));
            size_t position = writer.position();
            writer.write(uint32_t{ 0 });
            writer.write(static_cast<uint32_t>(value.items.size()));
            for (auto const& [key, item] : value.items) {
                bin_assert(write_value(key, value.keyType));
                bin_assert(write_value(item, value.valueType));
            }
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }

        bool write_value_visit(Option const& value) noexcept {
            bin_assert(!ValueHelper::is_container(value.valueType));
            bin_assert(value.items.size() <= 1);
            bin_assert(writer.write(value.valueType));
            writer.write(static_cast<uint8_t>(value.items.size()));
            for (auto const& [item] : value.items) {
                bin_assert(write_value(item, value.valueType));
            }
            return true;
        }

        bool write_value_visit(None const&) noexcept {
            return true;
        }

        template<typename T>
        bool write_value_visit(T const& value) noexcept {
            writer.write(value.value);
            return true;
        }

        bool write_value(Value const& value) noexcept {
            return std::visit([this](auto const& value) {
                return write_value_visit(value);
            }, value);
        }

        bool write_value(Value const& value, Type type) noexcept {
            return std::visit([this, type](auto const& value) {
                bin_assert(value.type == type);
                return write_value_visit(value);
                }, value);
        }
    public:
        std::string trace_error() noexcept {
            std::string trace;
            for(auto e = error.crbegin(); e != error.crend(); e++) {
                trace.append(*e);
            }
            return trace;
        }
    };
}

namespace ritobin::io {
    using namespace impl_binary_write;

    std::string write_binary(Bin const& bin, std::vector<char>& out, BinCompat const* compat) noexcept {
        BinBinaryWriter writer = { { out, compat }, {} };
        if (!writer.process(bin)) {
            return writer.trace_error();
        }
        return {};
    }
}
