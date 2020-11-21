#include "bin_morph.hpp"
#include "bin_types_helper.hpp"

namespace ritobin {
    // Conversion spectialization struct
    template<typename T, Category = T::category>
    struct morph_type_value_impl;

    template<typename T>
    struct morph_type_value_impl<T, Category::NONE> {
        static MorphResult morph([[maybe_unused]] T& value, [[maybe_unused]] Type newType) {
            return MorphResult::UNCHANGED;
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::NUMBER> {
        static MorphResult morph([[maybe_unused]] T& value, [[maybe_unused]] Type newType) {
            return MorphResult::UNCHANGED;
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::VECTOR> {
        static MorphResult morph([[maybe_unused]] T& value, [[maybe_unused]] Type newType) {
            return MorphResult::UNCHANGED;
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::STRING> {
        static MorphResult morph([[maybe_unused]] T& value, [[maybe_unused]] Type newType) {
            return MorphResult::UNCHANGED;
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::HASH> {
        static MorphResult morph([[maybe_unused]] T& value, [[maybe_unused]] Type newType) {
            return MorphResult::UNCHANGED;
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::OPTION> {
        static MorphResult morph(T& value, Type newType) {
            if (value.valueType == newType) {
                return MorphResult::UNCHANGED;
            } else if (ValueHelper::is_container(newType)) {
                return MorphResult::FAIL;
            } else {
                value.valueType = newType;
                auto worst_result = MorphResult::UNCHANGED;
                for (auto& item: value.items) {
                    if (auto const result = morph_value(item.value, newType); result < worst_result) {
                        worst_result = result;
                    }
                }
                return worst_result;
            }
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::LIST> {
        static MorphResult morph(T& value, Type newType) {
            if (value.valueType == newType) {
                return MorphResult::UNCHANGED;
            } else if (ValueHelper::is_container(newType)) {
                return MorphResult::FAIL;
            } else {
                value.valueType = newType;
                auto worst_result = MorphResult::UNCHANGED;
                for (auto& item: value.items) {
                    if (auto const result = morph_value(item.value, newType); result < worst_result) {
                        worst_result = result;
                    }
                }
                return worst_result;
            }
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::MAP> {
        static MorphResult morph(T& value, Type newType) {
            if (value.valueType == newType) {
                return MorphResult::UNCHANGED;
            } else if (ValueHelper::is_container(newType)) {
                return MorphResult::FAIL;
            } else {
                value.valueType = newType;
                auto worst_result = MorphResult::UNCHANGED;
                for (auto& item: value.items) {
                    if (auto const result = morph_value(item.value, newType); result < worst_result) {
                        worst_result = result;
                    }
                }
                return worst_result;
            }
        }
    };

    template<typename T>
    struct morph_type_value_impl<T, Category::CLASS> {
        static MorphResult morph([[maybe_unused]] T& value, [[maybe_unused]] Type newType) {
            return MorphResult::UNCHANGED;
        }
    };

    MorphResult morph_type_value(Value& value, [[maybe_unused]] Type newType) {
        return std::visit([newType] (auto& value) {
            using value_t = std::remove_cvref_t<decltype(value)>;
            return morph_type_value_impl<value_t>::morph(value, newType);
        }, value);
    }
}
