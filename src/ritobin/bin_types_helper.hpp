#ifndef BIN_TYPE_HELPER_HPP
#define BIN_TYPE_HELPER_HPP

#include "bin_types.hpp"

namespace ritobin {
    template<typename> struct ValueHelperImpl;

    template<typename...T> struct ValueHelperImpl<std::variant<T...>> {
        using ValuePtr = std::variant<T*...>;
        using ValuePtrConst = std::variant<T const*...>;

        static inline Type value_to_type(Value const& value) noexcept {
            return std::visit([](auto&& value) { return value.type; }, value);
        }

        static inline Value type_to_value(Type type) noexcept {
            Value value = None{};
            (void)((T::type == type ? (value = T{}, true) : false) || ...);
            return value;
        }

        static inline std::string_view value_to_type_name(Value const& value) noexcept {
            return std::visit([](auto&& value) { return value.type_name; }, value);
        }

        static constexpr std::string_view type_to_type_name(Type type) noexcept {
            std::string_view type_name = {};
            (void)((T::type == type ? (type_name = T::type_name, true) : false) || ...);
            return type_name;
        }

        static inline Value type_name_to_value(std::string_view type_name) noexcept {
            Value value = None{};
            (void)((type_name == T::type_name ? (value = T{}, true) : false) || ...);
            return value;
        }

        static constexpr bool try_type_name_to_type(std::string_view type_name, Type& type) noexcept {
            return ((type_name == T::type_name ? (type = T::type, true) : false) || ...);
        }

        static constexpr Category type_to_category(Type type) noexcept {
            Category category = Category::NONE;
            (void)((type == T::type ? (category = T::category, true) : false) || ...);
            return category;
        }

        // FIXME: unhardcode this
        static constexpr inline auto MAX_PRIMITIVE = Type::FILE;

        // FIXME: unhardcode this
        static constexpr inline auto MAX_COMPLEX = Type::FLAG;

        static constexpr bool is_primitive(Type type) noexcept {
            return !((uint8_t)type & 0x80);
        }

        static constexpr bool is_container(Type type) noexcept {
            auto const category = type_to_category(type);
            return category == Category::OPTION || category == Category::LIST || category == Category::MAP;
        }
    };

    using ValueHelper = ValueHelperImpl<Value>;
    using ValuePtr = ValueHelper::ValuePtr;
    using ValuePtrConst = ValueHelper::ValuePtrConst;
}

#endif // BIN_TYPE_HELPER_HPP
