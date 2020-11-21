#include "bin_io.hpp"
#include "bin_types.hpp"
#include "bin_types_helper.hpp"
#include "bin_numconv.hpp"
#define JSON_NOEXCEPTION
#include <json.hpp>
#include <optional>

#define bin_json_assert(...) \
    do { \
        if (!(__VA_ARGS__)) { \
            return ErrorStack { type_name,  #__VA_ARGS__ };  \
        } \
    } while(false)

#define bin_json_rethrow(key, ...) \
    do { \
        if (auto error = (__VA_ARGS__)) { \
            error->push(key); \
            return error; \
        } \
    } while(false)


namespace ritobin::io::json_impl {
    using json = nlohmann::json;

    struct ErrorStack {
        char const* type = {};
        char const* message = {};
        std::string path = {};

        void push(std::string apath) noexcept {
            apath.append(path);
            path = std::move(apath);
        }

        std::string trace() const noexcept {
            std::string result = {};
            result += "read ";
            result += this->type;
            result += " ";
            result += this->message;
            result += " at ";
            result += this->path;
            return result;
        }
    };
    using ErrorStackOption = std::optional<ErrorStack>;

    static void value_to_json_info(Value const& value, json& json) noexcept;
    static void value_to_json(Value const& value, json& json) noexcept;
    [[nodiscard]] static ErrorStackOption value_from_json(Value& value, json const& json) noexcept;

    static std::string to_index(std::string name) noexcept {
        return "['" + name +"']";
    };

    static std::string to_index(std::size_t index) noexcept {
        return "[" + std::to_string(index) + "]";
    }

    template<typename T>
    static void hash_to_json(T const& value, json& json) noexcept {
        if (value.str().empty()) {
            json = value.hash();
        } else {
            json = value.str();
        }
    }

    template<typename T>
    static void hash_to_json_info(T const& value, json& json) noexcept {
        if (value.str().empty()) {
            std::string str;
            from_num(str, value.hash(), 16);
            json = "0x" + str;
        } else {
            json = value.str();
        }
    }

    template<typename T>
    static bool hash_from_json(T& value, json const& json) noexcept {
        using hash_t = std::remove_cvref_t<decltype(value.hash())>;
        if (json.is_number()) {
            hash_t hash = json;
            value = hash;
            return true;
        } else if (json.is_string()){
            std::string str = json;
            value = str;
            return true;
        } else {
            return false;
        }
    }

    static ErrorStackOption item_from_json(Element& value, json const& json) noexcept {
        static constexpr char const * type_name = "element";
        bin_json_rethrow("", value_from_json(value.value, json));
        return std::nullopt;
    }

    static ErrorStackOption item_from_json(Pair& value, json const& json) noexcept {
        static constexpr char const * type_name = "pair";
        bin_json_assert(json.is_object());
        bin_json_assert(json.contains("key"));
        bin_json_assert(json.contains("value"));
        bin_json_rethrow(to_index("key"), value_from_json(value.key, json["key"]));
        bin_json_rethrow(to_index("value"), value_from_json(value.value, json["value"]));
        return std::nullopt;
    }

    static ErrorStackOption typed_from_json(Value& value, json const& json) noexcept {
        static constexpr char const * type_name = "value";
        bin_json_assert(json.is_object());
        bin_json_assert(json.contains("type"));
        bin_json_assert(json.contains("value"));
        bin_json_assert(json["type"].is_string());
        Type type = Type::NONE;
        std::string valueType_name = json["type"];
        bin_json_assert(ValueHelper::try_type_name_to_type(valueType_name, type));
        value = ValueHelper::type_to_value(type);
        bin_json_rethrow(to_index("value"), value_from_json(value, json["value"]));
        return std::nullopt;
    }

    static ErrorStackOption item_from_json(Field& value, json const& json) noexcept {
        static constexpr char const * type_name = "field";
        bin_json_assert(json.is_object());
        bin_json_assert(json.contains("key"));
        bin_json_assert(hash_from_json(value.key, json["key"]));
        bin_json_rethrow("", typed_from_json(value.value, json));
        return std::nullopt;
    }

    template<typename T, Category = T::category>
    struct json_value_impl;

    template<typename T>
    struct json_value_impl<T, Category::NONE> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const&, json& json) noexcept {
            json = nullptr;
        }

        static void to_json_info(T const&, json& json) noexcept {
            json = nullptr;
        }

        static ErrorStackOption from_json(T&, json const& json) noexcept {
            bin_json_assert(json.is_null());
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::NUMBER> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            json = value.value;
        }

        static void to_json_info(T const& value, json& json) noexcept {
            json = value.value;
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            using value_t = std::remove_cvref_t<decltype(value.value)>;
            if constexpr (std::is_same_v<value_t, bool>) {
                bin_json_assert(json.is_boolean());
            } else {
                bin_json_assert(json.is_number());
            }
            value.value = json;
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::VECTOR> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            json = value.value;
        }

        static void to_json_info(T const& value, json& json) noexcept {
            json = value.value;
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            bin_json_assert(json.is_array());
            bin_json_assert(json.size() <= value.value.size());
            size_t i = 0;
            for (auto const& item: json) {
                value.value[i++] = item;
            }
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::STRING> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            json = value.value;
        }

        static void to_json_info(T const& value, json& json) noexcept {
            json = value.value;
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            bin_json_assert(json.is_string());
            value.value = json;
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::HASH> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            hash_to_json(value.value, json);
        }

        static void to_json_info(T const& value, json& json) noexcept {
            hash_to_json_info(value.value, json);
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            bin_json_assert(hash_from_json(value.value, json));
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::OPTION> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            json = json::object();
            json["valueType"] =  ValueHelper::type_to_type_name(value.valueType);
            auto& json_items = json["items"];
            json_items = json::array();
            if (!value.items.empty()) {
                value_to_json(value.items.front().value, json_items.emplace_back());
            }
        }

        static void to_json_info(T const& value, json& json) noexcept {
            if (value.items.empty()) {
                json = nullptr;
            } else {
                value_to_json(value.items.front().value, json);
            }
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            bin_json_assert(json.is_object());
            bin_json_assert(json.contains("valueType"));
            bin_json_assert(json.contains("items"));
            bin_json_assert(json["valueType"].is_string());
            bin_json_assert(json["items"].is_array());
            std::string valueType_name = json["valueType"];
            bin_json_assert(ValueHelper::try_type_name_to_type(valueType_name, value.valueType));
            auto const& json_items = json["items"];
            if (!json_items.empty()) {
                auto& value_item = value.items.emplace_back(ValueHelper::type_to_value(value.valueType));
                bin_json_rethrow(to_index("items") + to_index(0),
                                 item_from_json(value_item, json_items.front()));
            }
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::LIST> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            json = json::object();
            json["valueType"] = ValueHelper::type_to_type_name(value.valueType);
            auto& json_items = json["items"];
            json_items = json::array();
            for (auto const& value_item: value.items) {
                value_to_json(value_item.value, json_items.emplace_back());
            }
        }

        static void to_json_info(T const& value, json& json) noexcept {
            json = json::array();
            for (auto const& item: value.items) {
                value_to_json_info(item.value, json.emplace_back());
            }
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            bin_json_assert(json.is_object());
            bin_json_assert(json.contains("valueType"));
            bin_json_assert(json.contains("items"));
            bin_json_assert(json["valueType"].is_string());
            bin_json_assert(json["items"].is_array());
            std::string valueType_name = json["valueType"];
            bin_json_assert(ValueHelper::try_type_name_to_type(valueType_name, value.valueType));
            auto const& json_items = json["items"];
            size_t index = 0;
            for (auto const& json_item: json_items) {
                auto& value_item = value.items.emplace_back(ValueHelper::type_to_value(value.valueType));
                bin_json_rethrow(to_index("items") + to_index(index),
                                 item_from_json(value_item, json_item));
                ++index;
            }
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::MAP> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            json = json::object();
            json["keyType"] = ValueHelper::type_to_type_name(value.keyType);
            json["valueType"] = ValueHelper::type_to_type_name(value.valueType);
            auto& json_items = json["items"];
            json_items = json::array();
            for (auto const& value_item: value.items) {
                auto& json_item = json_items.emplace_back();
                json_item = json::object();
                value_to_json(value_item.key, json_item["key"]);
                value_to_json(value_item.value, json_item["value"]);
            }
        }

        static void to_json_info(T const& value, json& json) noexcept {
            json = json::object();
            for (auto const& item: value.items) {
                nlohmann::json key_json = {};
                value_to_json_info(item.key, key_json);
                std::string key;
                if (key_json.is_string()) {
                    key = key_json;
                } else {
                    key = key_json.dump();
                }
                value_to_json_info(item.value, json[key]);
            }
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            bin_json_assert(json.is_object());
            bin_json_assert(json.contains("valueType"));
            bin_json_assert(json.contains("keyType"));
            bin_json_assert(json.contains("items"));
            bin_json_assert(json["valueType"].is_string());
            bin_json_assert(json["keyType"].is_string());
            bin_json_assert(json["items"].is_array());
            std::string keyType_name = json["keyType"];
            std::string valueType_name = json["valueType"];
            bin_json_assert(ValueHelper::try_type_name_to_type(keyType_name, value.keyType));
            bin_json_assert(ValueHelper::try_type_name_to_type(valueType_name, value.valueType));
            auto const& json_items = json["items"];
            size_t index = 0;
            for (auto const& json_item: json_items) {
                auto& value_item = value.items.emplace_back(ValueHelper::type_to_value(value.keyType),
                                                            ValueHelper::type_to_value(value.valueType));
                bin_json_rethrow(to_index("items") + to_index(index),
                                 item_from_json(value_item, json_item));
                ++index;
            }
            return std::nullopt;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::CLASS> {
        static constexpr char const * type_name = T::type_name;

        static void to_json(T const& value, json& json) noexcept {
            json = json::object();
            hash_to_json(value.name, json["name"]);
            auto& json_items = json["items"];
            json_items = json::array();
            for (auto const& value_item: value.items) {
                auto& json_item = json_items.emplace_back();
                json_item = json::object();
                hash_to_json(value_item.key, json_item["key"]);
                json_item["type"] = ValueHelper::value_to_type_name(value_item.value);
                value_to_json(value_item.value, json_item["value"]);
            }
        }

        static void to_json_info(T const& value, json& json) noexcept {
            json = json::object();
            hash_to_json_info(value.name, json["~class"]);
            for (auto const& item: value.items) {
                nlohmann::json key_json = {};
                hash_to_json_info(item.key, key_json);
                std::string key;
                if (key_json.is_string()) {
                    key = key_json;
                } else {
                    key = key_json.dump();
                }
                value_to_json_info(item.value, json[key]);
            }
        }

        static ErrorStackOption from_json(T& value, json const& json) noexcept {
            bin_json_assert(json.is_object());
            bin_json_assert(json.contains("name"));
            bin_json_assert(json.contains("items"));
            bin_json_assert(hash_from_json(value.name, json["name"]));
            bin_json_assert(json["items"].is_array());
            auto const& json_items = json["items"];
            size_t index = 0;
            for (auto const& json_item: json_items) {
                auto& value_item = value.items.emplace_back();
                bin_json_rethrow(to_index("items") + to_index(index),
                                 item_from_json(value_item, json_item));
                ++index;
            }
            return std::nullopt;
        }
    };

    void value_to_json(Value const& value, json& json) noexcept {
        std::visit([&json](auto const& value) noexcept {
            using value_t = std::remove_cvref_t<decltype(value)>;
            json_value_impl<value_t>::to_json(value, json);
        }, value);
    }

    void value_to_json_info(Value const& value, json& json) noexcept {
        std::visit([&json](auto const& value) noexcept {
            using value_t = std::remove_cvref_t<decltype(value)>;
            json_value_impl<value_t>::to_json_info(value, json);
        }, value);
    }

    ErrorStackOption value_from_json(Value& value, json const& json) noexcept {
        return std::visit([&json](auto& value) noexcept {
            using value_t = std::remove_cvref_t<decltype(value)>;
            return json_value_impl<value_t>::from_json(value, json);
        }, value);
    }

    static void bin_to_json(Bin const& value, std::vector<char>& out, int indent) noexcept {
        json json = json::object();
        for (auto const& section: value.sections) {
            auto& json_item = json[section.first];
            json_item = json::object();
            json_item["type"] = ValueHelper::value_to_type_name(section.second);
            value_to_json(section.second, json_item["value"]);
        }
        auto tmp = json.dump(indent);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    static void bin_to_json_info(Bin const& value, std::vector<char>& out, int indent) noexcept {
        json json = json::object();
        for (auto const& section: value.sections) {
            value_to_json_info(section.second, json[section.first]);
        }
        auto tmp = json.dump(indent);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    static ErrorStackOption bin_from_json(Bin& bin, std::span<char const> data) noexcept {
        static constexpr char const * type_name = "bin";
        json json = json::parse(data, nullptr, false, true);
        if (json.is_discarded()) {
            return ErrorStack { "bin", "Bad json", "/" };
        }
        bin_json_assert(json.is_object());
        for (auto const& [json_key, json_item]: json.items()) {
            auto& section = bin.sections[json_key];
            bin_json_rethrow("bin" + to_index(json_key),
                             typed_from_json(section, json_item));
        }
        return std::nullopt;
    }
}

namespace ritobin::io {
    using namespace json_impl;

    std::string read_json(Bin& value, std::span<char const> data) noexcept {
        if (auto error = bin_from_json(value, data)) {
            return error->trace();
        } else {
            return {};
        }
    }

    std::string write_json(Bin const& value, std::vector<char>& out, int indent_size) noexcept {
        bin_to_json(value, out, indent_size);
        return {};
    }

    std::string write_json_info(Bin const& value, std::vector<char>& out, int indent_size) noexcept {
        bin_to_json_info(value, out, indent_size);
        return {};
    }
}
