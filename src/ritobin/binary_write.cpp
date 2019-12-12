#include <stack>
#include <vector>
#include <algorithm>
#include "bin.hpp"

namespace ritobin {
    struct ByteWriter {
        std::vector<char>& data;

        template<typename T>
        bool write_at(size_t value, size_t pos) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            auto const tmp = static_cast<T>(value);
            if (pos + sizeof(T) > position()) {
                return false;
            }
            memcpy(data.data() + pos, &tmp, sizeof(T));
            return true;
        }

        template<typename T, size_t S>
        void write(std::array<T, S> const& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            char buffer[sizeof(T) * S];
            memcpy(buffer, value.data(), sizeof(T) * S);
            data.insert(data.end(), buffer, buffer + sizeof(T) * S);
        }

        template<typename T>
        void write(std::vector<T> const& value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            auto const pos = position();
            auto const size = sizeof(T) * value.size();
            data.insert(data.end(), size, '\x00');
            memcpy(data.data() + pos, value.data(), size);
        }

        template<typename T>
        void write(T value) noexcept {
            static_assert(std::is_arithmetic_v<T>);
            char buffer[sizeof(value)];
            memcpy(buffer, &value, sizeof(value));
            data.insert(data.end(), buffer, buffer + sizeof(buffer));
        }

        void write(bool value) noexcept {
            write(static_cast<uint8_t>(value));
        }

        void write(Type type) noexcept {
            write(static_cast<uint8_t>(type));
        }

        void write(std::string const& value) {
            write(static_cast<uint16_t>(value.size()));
            data.insert(data.end(), value.data(), value.data() + value.size());
        }

        void write(NameHash const& value) {
            write(value.hash());
        }

        void write(StringHash const& value) {
            write(value.hash());
        }
    
        inline size_t position() const noexcept {
            return data.size();
        }
    };


    struct WriteBinaryImpl {
        std::vector<Node> const & nodes_;
        std::vector<char>& result_;
        std::string& error_;


        std::vector<std::string> linked_ = {};
        std::vector<uint32_t> entries_ = {};
        std::vector<char> bufferEntries_ = {};
        ByteWriter writerEntries_ = { bufferEntries_ };
        ByteWriter writerResult_ = { result_ };
        std::stack<size_t> offsets_ = {};

        enum class State {
            SectionType,
            SectionVersion,
            SectionLinkedStart,
            SectionLinked,
            SectionEntriesStart,
            SectionEntries,
            SectionEnd,
        } state = {};


        bool process() noexcept {
            result_.clear();
            error_.clear();
            if (!process_sections()) {
                error_.append("Failed to read node!");
                return false;
            }
            return true;
        }
    private:
        inline bool fail_fast() noexcept {
            return false;
        }
#define bin_assert(...) do { if(!(__VA_ARGS__)) { return fail_fast(); } } while(false)

        bool process_section_end() noexcept {
            bin_assert(state == State::SectionEnd);
            bin_assert(offsets_.empty());
            writerResult_.write(static_cast<uint32_t>(linked_.size()));
            for (auto const& linked : linked_) {
                writerResult_.write(linked);
            }
            writerResult_.write(static_cast<uint32_t>(entries_.size()));
            writerResult_.write(entries_);
            writerResult_.write(bufferEntries_);
            return true;
        }

        bool process_sections() noexcept {
            for (auto const& node : nodes_) {
                bin_assert(state != State::SectionEnd);
                switch (state) {
                case State::SectionType:
                    bin_assert(process_section_type(node));
                    break;
                case State::SectionVersion:
                    bin_assert(process_section_version(node));
                    break;
                case State::SectionLinkedStart:
                    bin_assert(process_section_linked_start(node));
                    break;
                case State::SectionLinked:
                    bin_assert(process_section_linked(node));
                    break;
                case State::SectionEntriesStart:
                    bin_assert(process_section_entries_start(node));
                    break;
                case State::SectionEntries:
                    bin_assert(process_section_entries(node));
                    break;
                default: break;
                }
            }
            bin_assert(process_section_end());
            return true;
        }

        bool process_section_type(Node const& node) noexcept {
            auto section = std::get_if<Section>(&node);
            bin_assert(section);
            bin_assert(section->name == "type");
            auto value = std::get_if<String>(&section->value);
            bin_assert(value);
            bin_assert(value->value == "PROP" || value->value == "PTCH");
            if (value->value == "PTCH") {
                writerResult_.write(std::array{ 'P', 'T', 'C', 'H' });
                writerResult_.write(uint32_t{ 1 });
                writerResult_.write(uint32_t{ 0 });
            }
            writerResult_.write(std::array{ 'P', 'R', 'O', 'P' });
            state = State::SectionVersion;
            return true;
        }

        bool process_section_version(Node const& node) noexcept {
            auto section = std::get_if<Section>(&node);
            bin_assert(section);
            bin_assert(section->name == "version");
            auto value = std::get_if<U32>(&section->value);
            bin_assert(value);
            writerResult_.write(value->value);
            state = State::SectionLinkedStart;
            return true;
        }

        bool process_section_linked_start(Node const& node) noexcept {
            auto section = std::get_if<Section>(&node);
            bin_assert(section);
            bin_assert(section->name == "linked");
            auto value = std::get_if<List>(&section->value);
            bin_assert(value);
            bin_assert(value->valueType == Type::STRING);
            state = State::SectionLinked;
            return true;
        }

        bool process_section_linked(Node const& node) noexcept {
            if (std::holds_alternative<NestedEnd>(node)) {
                auto const& end = std::get<NestedEnd>(node);
                bin_assert(end.count == linked_.size());
                bin_assert(end.type == Type::LIST);
                state = State::SectionEntriesStart;
                return true;
            }

            auto item = std::get_if<Item>(&node);
            bin_assert(item);
            auto value = std::get_if<String>(&item->value);
            bin_assert(value);
            linked_.push_back(value->value);
            return true;
        }

        bool process_section_entries_start(Node const& node) noexcept {
            auto section = std::get_if<Section>(&node);
            bin_assert(section);
            bin_assert(section->name == "entries");
            auto value = std::get_if<Map>(&section->value);
            bin_assert(value);
            bin_assert(value->keyType == Type::HASH);
            bin_assert(value->valueType == Type::EMBED);
            state = State::SectionEntries;
            return true;
        }

        bool process_section_entries(Node const& node) noexcept {
            if (offsets_.empty()) {
                if (std::holds_alternative<NestedEnd>(node)) {
                    auto const& end = std::get<NestedEnd>(node);
                    bin_assert(end.count == entries_.size());
                    bin_assert(end.type == Type::MAP);
                    state = State::SectionEnd;
                    return true;
                }
                auto const entry = std::get_if<Pair>(&node);
                bin_assert(entry);
                auto const key = std::get_if<Hash>(&entry->key);
                auto const value = std::get_if<Embed>(&entry->value);
                bin_assert(key && value);
                entries_.push_back(value->value.hash());
                offsets_.push(writerEntries_.position());
                writerEntries_.write(uint32_t{ 0 });
                writerEntries_.write(uint32_t{ key->value.hash() });
                writerEntries_.write(uint16_t{ 0 });
                return true;
            }

            if (offsets_.size() == 1) {
                if (std::holds_alternative<NestedEnd>(node)) {
                    auto const& end = std::get<NestedEnd>(node);
                    bin_assert(end.type == Type::EMBED);
                    auto const offset = offsets_.top();
                    auto const position = writerEntries_.position();
                    bin_assert(writerEntries_.write_at<uint32_t>(position - offset - 4, offset));
                    bin_assert(writerEntries_.write_at<uint16_t>(end.count, offset + 8));
                    offsets_.pop();
                    return true;
                }
                // TODO: extra check here ?
            }

            return std::visit([this](auto&& node) -> bool {
                return handle_node_visit(std::forward<decltype(node)>(node));
            }, node);
        }


        bool handle_node_visit(Item const& item) noexcept {
            bin_assert(handle_value(item.value));
            return true;
        }

        bool handle_node_visit(Field const& field) noexcept {
            writerEntries_.write(field.name.hash());
            writerEntries_.write(Value_type(field.value));
            bin_assert(handle_value(field.value));
            return true;
        }

        bool handle_node_visit(Pair const& pair) noexcept {
            bin_assert(handle_value(pair.key));
            bin_assert(handle_value(pair.value));
            return true;
        }

        bool handle_node_visit(NestedEnd const& end) noexcept {
            auto const offset = offsets_.top();
            auto const position = writerEntries_.position();
            switch (end.type) {
            case Type::POINTER:
            case Type::EMBED:
                bin_assert(writerEntries_.write_at<uint32_t>(position - offset - 4, offset));
                bin_assert(writerEntries_.write_at<uint16_t>(end.count, offset + 4));
                break;
            case Type::MAP:
            case Type::LIST:
                bin_assert(writerEntries_.write_at<uint32_t>(position - offset - 4, offset));
                bin_assert(writerEntries_.write_at<uint32_t>(end.count, offset + 4));
                break;
            case Type::OPTION:
                bin_assert(end.count < 2);
                bin_assert(writerEntries_.write_at<uint8_t>(end.count, offset));
                break;
            default:
                bin_assert(!"Invalid nest end type!");
                break;
            }
            offsets_.pop();
            return true;
        }

        bool handle_node_visit(Section const&) noexcept {
            bin_assert(!"Section not allowed here!");
            return true;
        }



        bool handle_value(Value const& value) noexcept {
            return std::visit([this](auto&& value) {
                return handle_value_visit(std::forward<decltype(value)>(value));
            }, value);
        }

        bool handle_value_visit(None const&) noexcept {
            return true;
        }

        template<typename T>
        bool handle_value_visit(T const& value) noexcept {
            writerEntries_.write(value.value);
            return true;
        }

        bool handle_value_visit(Map const& value) noexcept {
            writerEntries_.write(value.keyType);
            writerEntries_.write(value.valueType);
            offsets_.push(writerEntries_.position());
            writerEntries_.write(uint32_t{ 0 });
            writerEntries_.write(uint32_t{ 0 });
            return true;
        }

        bool handle_value_visit(List const& value) noexcept {
            writerEntries_.write(value.valueType);
            offsets_.push(writerEntries_.position());
            writerEntries_.write(uint32_t{ 0 });
            writerEntries_.write(uint32_t{ 0 });
            return true;
        }

        bool handle_value_visit(Option const& value) noexcept {
            writerEntries_.write(value.valueType);
            offsets_.push(writerEntries_.position());
            writerEntries_.write(uint8_t{ 0 });
            return true;
        }

        bool handle_value_visit(Pointer const& value) noexcept {
            if (value.value.value == 0 && value.value.unhashed.empty()) {
                writerEntries_.write(uint32_t{ 0 });
            } else {
                writerEntries_.write(value.value.hash());
                offsets_.push(writerEntries_.position());
                writerEntries_.write(uint32_t{ 0 });
                writerEntries_.write(uint16_t{ 0 });
            }
            return true;
        }

        bool handle_value_visit(Embed const& value) noexcept {
            writerEntries_.write(value.value.hash());
            offsets_.push(writerEntries_.position());
            writerEntries_.write(uint32_t{ 0 });
            writerEntries_.write(uint16_t{ 0 });
            return true;
        }
#undef bin_assert
    };

    bool binary_write(NodeList const& nodes, std::vector<char>& result, std::string& error) noexcept {
        WriteBinaryImpl writer = {
            nodes, result, error
        };
        return writer.process();
    }
}