#include "bin_io.hpp"
#include "bin_types.hpp"
#include "bin_types_helper.hpp"
#include "bin_numconv.hpp"
#include "json.hpp"
#include <stdexcept>

#define bin_json_assert(...) do { if (!(__VA_ARGS__)) { \
        throw std::runtime_error(std::string(T::type_name) + (": " #__VA_ARGS__));  \
    } } while(false)

namespace ritobin::io::json_impl {
    using json = nlohmann::json;


    static void value_to_json_info(Value const& value, json& json);
    static void value_to_json(Value const& value, json& json);
    static void value_from_json(Value& value, json const& json);

    template<typename T>
    inline void hash_to_json(T const& value, json& json) {
        if (value.str().empty()) {
            json = value.hash();
        } else {
            json = value.str();
        }
    }

    template<typename T>
    inline void hash_to_json_info(T const& value, json& json) {
        if (value.str().empty()) {
            std::string str;
            from_num(str, value.hash(), 16);
            json = "0x" + str;
        } else {
            json = value.str();
        }
    }

    template<typename T>
    inline bool hash_from_json(T& value, json const& json) {
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

    template<typename T, Category = T::category>
    struct json_value_impl;

    template<typename T>
    struct json_value_impl<T, Category::NONE> {
        static void to_json(T const&, json& json) {
            json = nullptr;
        }

        static void to_json_info(T const&, json& json) {
            json = nullptr;
        }

        static void from_json(T&, json const& json) {
            bin_json_assert(json.is_null());
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::NUMBER> {
        static void to_json(T const& value, json& json) {
            json = value.value;
        }

        static void to_json_info(T const& value, json& json) {
            json = value.value;
        }

        static void from_json(T& value, json const& json) {
            using value_t = std::remove_cvref_t<decltype(value.value)>;
            if constexpr (std::is_same_v<value_t, bool>) {
                bin_json_assert(json.is_boolean());
            } else {
                bin_json_assert(json.is_number());
            }
            value.value = json;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::VECTOR> {
        static void to_json(T const& value, json& json) {
            json = value.value;
        }

        static void to_json_info(T const& value, json& json) {
            json = value.value;
        }

        static void from_json(T& value, json const& json) {
            bin_json_assert(json.is_array());
            bin_json_assert(json.size() <= value.value.size());
            size_t i = 0;
            for (auto const& item: json) {
                value.value[i++] = item;
            }
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::STRING> {
        static void to_json(T const& value, json& json) {
            json = value.value;
        }

        static void to_json_info(T const& value, json& json) {
            json = value.value;
        }

        static void from_json(T& value, json const& json) {
            bin_json_assert(json.is_string());
            value.value = json;
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::HASH> {
        static void to_json(T const& value, json& json) {
            hash_to_json(value.value, json);
        }

        static void to_json_info(T const& value, json& json) {
            hash_to_json_info(value.value, json);
        }

        static void from_json(T& value, json const& json) {
            bin_json_assert(hash_from_json(value.value, json));
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::OPTION> {
        static void to_json(T const& value, json& json) {
            json = json::object();
            json["valueType"] =  ValueHelper::type_to_type_name(value.valueType);
            auto& json_items = json["items"];
            json_items = json::array();
            if (!value.items.empty()) {
                value_to_json(value.items.front().value, json_items.emplace_back());
            }
        }

        static void to_json_info(T const& value, json& json) {
            if (value.items.empty()) {
                json = nullptr;
            } else {
                value_to_json(value.items.front().value, json);
            }
        }

        static void from_json(T& value, json const& json) {
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
                value_from_json(value_item.value, json_items.front());
            }
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::LIST> {
        static void to_json(T const& value, json& json) {
            json = json::object();
            json["valueType"] = ValueHelper::type_to_type_name(value.valueType);
            auto& json_items = json["items"];
            json_items = json::array();
            for (auto const& value_item: value.items) {
                value_to_json(value_item.value, json_items.emplace_back());
            }
        }

        static void to_json_info(T const& value, json& json) {
            json = json::array();
            for (auto const& item: value.items) {
                value_to_json_info(item.value, json.emplace_back());
            }
        }

        static void from_json(T& value, json const& json) {
            bin_json_assert(json.is_object());
            bin_json_assert(json.contains("valueType"));
            bin_json_assert(json.contains("items"));
            bin_json_assert(json["valueType"].is_string());
            bin_json_assert(json["items"].is_array());
            std::string valueType_name = json["valueType"];
            bin_json_assert(ValueHelper::try_type_name_to_type(valueType_name, value.valueType));
            auto const& json_items = json["items"];
            for (auto const& json_item: json_items) {
                auto& value_item = value.items.emplace_back(ValueHelper::type_to_value(value.valueType));
                value_from_json(value_item.value, json_item);
            }
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::MAP> {
        static void to_json(T const& value, json& json) {
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

        static void to_json_info(T const& value, json& json) {
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

        static void from_json(T& value, json const& json) {
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
            for (auto const& json_item: json_items) {
                bin_json_assert(json_item.is_object());
                bin_json_assert(json_item.contains("key"));
                bin_json_assert(json_item.contains("value"));
                auto& value_item = value.items.emplace_back(ValueHelper::type_to_value(value.keyType),
                                                            ValueHelper::type_to_value(value.valueType));
                value_from_json(value_item.key, json_item["key"]);
                value_from_json(value_item.value, json_item["value"]);
            }
        }
    };

    template<typename T>
    struct json_value_impl<T, Category::CLASS> {
        static void to_json(T const& value, json& json) {
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

        static void to_json_info(T const& value, json& json) {
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

        static void from_json(T& value, json const& json) {
            bin_json_assert(json.is_object());
            bin_json_assert(json.contains("name"));
            bin_json_assert(json.contains("items"));
            bin_json_assert(hash_from_json(value.name, json["name"]));
            bin_json_assert(json["items"].is_array());
            auto const& json_items = json["items"];
            for (auto const& json_item: json_items) {
                bin_json_assert(json_item.is_object());
                bin_json_assert(json_item.contains("type"));
                bin_json_assert(json_item.contains("key"));
                bin_json_assert(json_item.contains("value"));
                bin_json_assert(json_item["type"].is_string());
                auto& value_item = value.items.emplace_back();
                bin_json_assert(hash_from_json(value_item.key, json_item["key"]));
                Type type = Type::NONE;
                std::string type_name = json_item["type"];
                bin_json_assert(ValueHelper::try_type_name_to_type(type_name, type));
                value_item.value = ValueHelper::type_to_value(type);
                value_from_json(value_item.value, json_item["value"]);
            }
        }
    };

    void value_to_json(Value const& value, json& json) {
        std::visit([&json](auto const& value) {
            using value_t = std::remove_cvref_t<decltype(value)>;
            json_value_impl<value_t>::to_json(value, json);
        }, value);
    }

    void value_to_json_info(Value const& value, json& json) {
        std::visit([&json](auto const& value) {
            using value_t = std::remove_cvref_t<decltype(value)>;
            json_value_impl<value_t>::to_json_info(value, json);
        }, value);
    }

    void value_from_json(Value& value, json const& json) {
        std::visit([&json](auto& value) {
            using value_t = std::remove_cvref_t<decltype(value)>;
            json_value_impl<value_t>::from_json(value, json);
        }, value);
    }

    static void bin_to_json(Bin const& value, std::vector<char>& out, int indent) {
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

    static void bin_to_json_info(Bin const& value, std::vector<char>& out, int indent) {
        json json = json::object();
        for (auto const& section: value.sections) {
            value_to_json_info(section.second, json[section.first]);
        }
        auto tmp = json.dump(indent);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    static void bin_from_json(Bin& bin, std::span<char const> data) {
        using T = Bin;
        json json = json::parse(data);
        bin_json_assert(json.is_object());
        for (auto const& [json_key, json_item]: json.items()) {
            bin_json_assert(json_item.contains("type"));
            bin_json_assert(json_item.contains("value"));
            bin_json_assert(json_item["type"].is_string());
            Type type = Type::NONE;
            std::string type_name = json_item["type"];
            bin_json_assert(ValueHelper::try_type_name_to_type(type_name, type));
            auto& section = bin.sections[json_key];
            section = ValueHelper::type_to_value(type);
            value_from_json(section, json_item["value"]);
        }
    }
}

namespace ritobin::io {
    using namespace json_impl;

    std::string read_json(Bin& value, std::span<char const> data) noexcept {
        try {
            bin_from_json(value, data);
            return {};
        } catch (std::exception const& error) {
            return error.what();
        }
    }

    std::string write_json(Bin const& value, std::vector<char>& out, int indent_size) noexcept {
        try {
            bin_to_json(value, out, indent_size);
            return {};
        } catch (std::exception const& error) {
            return error.what();
        }
    }

    std::string write_json_info(Bin const& value, std::vector<char>& out, int indent_size) noexcept {
        try {
            bin_to_json_info(value, out, indent_size);
            return {};
        } catch (std::exception const& error) {
            return error.what();
        }
    }
}

