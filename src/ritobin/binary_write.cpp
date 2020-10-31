#include <stdexcept>
#include "bin.hpp"


namespace ritobin {
    struct BinaryWriter {
        std::vector<char>& buffer_;

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

        void write(Type type) noexcept {
            write(static_cast<uint8_t>(type));
        }

        void write(std::string const& value) {
            write(static_cast<uint16_t>(value.size()));
            buffer_.insert(buffer_.end(), value.data(), value.data() + value.size());
        }

        void write(FNV1a const& value) {
            write(value.hash());
        }

        inline size_t position() const noexcept {
            return buffer_.size();
        }
    };

    struct BinBinaryWriter {
        Bin const& bin;
        BinaryWriter writer;
        std::vector<std::string> error;

        #define bin_assert(...) do { \
            if(!(__VA_ARGS__)) { \
                return fail_msg(#__VA_ARGS__ "\n"); \
            } } while(false)

        bool process() noexcept {
            error.clear();
            writer.buffer_.clear();
            bin_assert(write_header());
            bin_assert(write_entries());
            return true;
        }
    private:
        bool fail_msg(char const* msg) noexcept {
            error.emplace_back(msg);
            return false;
        }

        bool write_header() noexcept {
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

            if (version->value < 2) {
                return true;
            }

            auto linked_section = bin.sections.find("linked");
            bin_assert(linked_section != bin.sections.end());
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
    
        bool write_entries() noexcept {
            auto entries_section = bin.sections.find("entries");
            bin_assert(entries_section != bin.sections.end());
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
                writer.write(ValueHelper::to_type(item));
                bin_assert(write_value(item));
            }
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
                writer.write(ValueHelper::to_type(item));
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
                writer.write(ValueHelper::to_type(item));
                bin_assert(write_value(item));
            }
            writer.write_at(position, writer.position() - position - 4);
            return true;
        }

        bool write_value_visit(List const& value) noexcept {
            bin_assert(!is_container(value.valueType));
            writer.write(value.valueType);
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
            bin_assert(!is_container(value.valueType));
            writer.write(value.valueType);
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
            bin_assert(is_primitive(value.keyType));
            bin_assert(!is_container(value.valueType));
            writer.write(value.keyType);
            writer.write(value.valueType);
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
            bin_assert(value.valueType != Type::MAP);
            bin_assert(value.valueType != Type::LIST);
            bin_assert(value.valueType != Type::OPTION);
            bin_assert(value.items.size() <= 1);
            writer.write(value.valueType);
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
    };

    void Bin::write_binary(std::vector<char>& out) const {
        BinBinaryWriter writer = { *this, { out }, {} };
        if (!writer.process()) {
            std::string error;
            for(auto e = writer.error.crbegin(); e != writer.error.crend(); e++) {
                error.append(*e);
            }
            throw std::runtime_error(std::move(error));
        }
    }
}
