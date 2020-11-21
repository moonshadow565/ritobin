#include "bin_morph.hpp"
#include "bin_numconv.hpp"
#include "bin_types_helper.hpp"
#include <limits>

namespace ritobin {
    // Conversion spectialization struct
    template<typename FromT, typename IntoT, Category = FromT::category, Category = IntoT::category>
    struct morph_value_impl;

    /// ---------------------------------------------------------------------------------------------------------------
    /// Helper functions
    /// ---------------------------------------------------------------------------------------------------------------
    static MorphResult morph_value_move(Value& from, ValuePtr into);

    template <typename FromT, typename IntoT>
    static bool convert_number(FromT const& from, IntoT& into) {
        using from_t = std::remove_cvref_t<FromT>;
        using into_t = std::remove_cvref_t<IntoT>;
        static_assert (std::is_arithmetic_v<FromT>);
        static_assert (std::is_arithmetic_v<IntoT>);
        if constexpr (std::is_same_v<from_t, into_t>) {
            into = from;
            return true;
        } else {
            auto const converted = (into_t)from;
            into = converted;
            return (from_t)converted == from;
        }
    }

    template <typename FromT, typename IntoT>
    static bool convert_vector_number(FromT const& from, IntoT& into) {
        using from_t = std::remove_cvref_t<FromT>;
        using into_t = std::remove_cvref_t<IntoT>;
        static_assert (std::is_arithmetic_v<FromT>);
        static_assert (std::is_arithmetic_v<IntoT>);
        constexpr auto is_from_float = std::is_floating_point_v<from_t>;
        constexpr auto is_into_float = std::is_floating_point_v<into_t>;
        if constexpr (!is_from_float && is_into_float) {
            constexpr auto max = (into_t)std::numeric_limits<from_t>::max();
            auto const converted = (into_t)from / max;
            into = converted;
            return (from_t)(converted * max) == from;
        } else if constexpr (is_from_float && !is_into_float) {
            constexpr auto max = (from_t)std::numeric_limits<into_t>::max();
            auto const converted = (into_t)(from * max);
            into = converted;
            return (from_t)converted / max == from;
        } else {
            return convert_number(from, into);
        }
    }

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting None
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting from none to none is allways ok
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::OK;
        }
    };

    // Converting from none to number results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::NUMBER> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting from none to vector results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::VECTOR> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting from none to string results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::STRING> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting from none to hash results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::HASH> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting from none to option results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::OPTION> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting from none to list results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::LIST> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting from none to map results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::MAP> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting from none to class results in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NONE, Category::CLASS> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };


    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting numbers
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting number into none will lose all information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Numbers might be either converted with loss of some information or fully converted
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::NUMBER> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (convert_number(from.value, into.value)) {
                return MorphResult::OK;
            } else {
                return MorphResult::LOSSY;
            }
        }
    };

    // Converting to vector will place the result in first slot of a vector and will allways result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::VECTOR> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (convert_number(from.value, into.value.front())) {
                return MorphResult::INCOMPLETE;
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Numbers can allways be converted to strings
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::STRING> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (from_num(into.value, from.value)) {
                return MorphResult::OK;
            } else {
                return MorphResult::OK;
            }
        }
    };

    // Hash will be created from numerical value with potential loss of information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::HASH> {
        static MorphResult morph (FromT& from, IntoT& into) {
            using storage_t = typename decltype(into.value)::storage_t;
            if (storage_t storage = {}; convert_number(from.value, storage)) {
                into.value = storage;
                return MorphResult::OK;
            } else {
                return MorphResult::LOSSY;
            }
        }
    };

    // Number will be moved into option
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::OPTION> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;;
        }
    };

    // Number will be moved into list
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::LIST> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Moves number to first field of new map with key U32 = 0
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::MAP> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.keyType = Type::U32;
            into.valueType = FromT::type;
            into.items = { Pair { U32 { 0 }, std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Converting to class will be result in incomplete value as we can not split number in key value pair
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::NUMBER, Category::CLASS> {
        static MorphResult morph ([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting vectors
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting vector into none will lose all information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Converting from vector will only take first value and allways result in loss of information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::NUMBER> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (convert_number(from.value.front(), into.value)) {
                return MorphResult::LOSSY;
            } else {
                return MorphResult::LOSSY;
            }
        }
    };

    // Converting beetwen float vector and integral vector will scale value into 0.0 - 1.0 or -1.0 - 1.0
    // If vectors don't share same size the result will be either incomplete or lossy
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::VECTOR> {
        static MorphResult morph (FromT& from, IntoT& into) {
            using from_t = std::remove_cvref_t<decltype(from.value)>;
            using into_t = std::remove_cvref_t<decltype(into.value)>;
            if constexpr (std::is_same_v<from_t, into_t>) {
                into.value = std::move(from.value);
                return MorphResult::OK;
            } else {
                auto const min = std::min(from.value.size(), into.value.size());
                MorphResult result = MorphResult::OK;
                for (size_t i = 0; i != min; ++i) {
                    if (!convert_vector_number(from.value[i], into.value[i])) {
                        result = MorphResult::LOSSY;
                    }
                }
                if (min < from.value.size()) {
                    return MorphResult::LOSSY;
                } else if (min < into.value.size()) {
                    return MorphResult::INCOMPLETE;
                } else {
                    return result;
                }
            }

        }
    };

    // Convert first vector number into string, the result is allways lossy as other numbers are discarded
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::STRING> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (from_num(into.value, from.value.front())) {
                return MorphResult::LOSSY;
            } else {
                return MorphResult::LOSSY;
            }
        }
    };

    // Will place first value from vector into hash and lose other values
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::HASH> {
        static MorphResult morph (FromT& from, IntoT& into) {
            using storage_t = typename decltype(into.value)::storage_t;
            storage_t storage = {};
            if (convert_number(from.value.front(), storage)) {
                into.value = storage;
                return MorphResult::LOSSY;
            } else {
                into.value = storage;
                return MorphResult::LOSSY;
            }
        }
    };

    // Vector will be moved into option
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::OPTION> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Converting from vector to list will fill the list with vectors values
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::LIST> {
        static MorphResult morph (FromT& from, IntoT& into) {
            using item_wrapper_type = typename FromT::item_wrapper_type;
            into.valueType = item_wrapper_type::type;
            for (auto const& value: from.value) {
                into.items.emplace_back(item_wrapper_type { value });
            }
            return MorphResult::OK;
        }
    };

    // CMoves vector to first field of new map with key U32 =0
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::MAP> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.keyType = Type::U32;
            into.valueType = FromT::type;
            into.items = { Pair { U32 { 0 }, std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Converting from vector to class will result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::VECTOR, Category::CLASS> {
        static MorphResult morph ([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting strings
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting string into none will lose all information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Strings might be convertible to numbers
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::NUMBER> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (to_num(from.value, into.value)) {
                return MorphResult::OK;
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Try to convert first string as a number in first value of vector, allways gives incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::VECTOR> {
        static MorphResult morph ([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            if (to_num(from.value, into.value.front())) {
                return MorphResult::INCOMPLETE;
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Strings are allways convertible to strings
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::STRING> {
        static MorphResult morph (FromT& from,  IntoT& into) {
            into.value = std::move(from.value);
            return MorphResult::OK;
        }
    };

    // Hash can allways be built using a string
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::HASH> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.value.empty()) {
                into.value = std::move(from.value);
                return MorphResult::OK;
            } else {
                return MorphResult::OK;
            }
        }
    };

    // String Number will be moved into option
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::OPTION> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Converting a string into list moves it as only element of list
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::LIST> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Moves string into first field of new map with key = U32
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::MAP> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.keyType = Type::U32;
            into.valueType = FromT::type;
            into.items = { Pair { U32 { 0 }, std::move(from) } };
            return MorphResult::OK;
        }
    };

    // String can not be converted to class and will allways result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::STRING, Category::CLASS> {
        static MorphResult morph ([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting hashes
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting hash into none will lose all information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Hashes are convertible into numbers with potential loss of information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::NUMBER> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (auto const storage = from.value.hash(); convert_number(storage, into.value)) {
                return MorphResult::OK;
            } else {
                return MorphResult::LOSSY;
            }
        }
    };

    // Hash will be placed in first value of vector and result in incomplete vector
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::VECTOR> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (convert_number(from.value.hash(), into.value.front())) {
                return MorphResult::INCOMPLETE;
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Hashes could be converted into string
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::STRING> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (auto str = from.value.str(); !str.empty()) {
                into.value = std::move(from.value).str();
                return MorphResult::OK;
            } else if (from.value.hash() == 0) {
                return MorphResult::OK;
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Hash might be converted to another hash with potential lost of information or fully using underlying string
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::HASH> {
        static MorphResult morph (FromT& from, IntoT& into) {
            using from_t = std::remove_cvref_t<decltype(from.value)>;
            using into_t = std::remove_cvref_t<decltype(into.value)>;
            if constexpr (std::is_same_v<from_t, into_t>) {
                into.value = std::move(from.value);
                return MorphResult::OK;
            } else if (auto str = from.value.str(); !str.empty()) {
                into.value = std::move(from.value).str();
                return MorphResult::OK;
            } else {
                using into_storage_t = typename decltype(into.value)::storage_t;
                into_storage_t storage = {};
                if (convert_number(from.value.hash(), storage)) {
                    into.value = storage;
                    return MorphResult::LOSSY;
                } else {
                    into.value = storage;
                    return MorphResult::INCOMPLETE;
                }
            }
        }
    };

    // String hash will be moved into option
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::OPTION> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Converting a hash into list moves it as only element of list
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::LIST> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Moves hash into first field of new map with key U32 = 0
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::MAP> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.keyType = Type::U32;
            into.valueType = FromT::type;
            into.items = { Pair { U32 { 0 }, std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Hash can not be converted to class and will allways result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::HASH, Category::CLASS> {
        static MorphResult morph ([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting options
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting option into none will lose all information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Converting option to number will try to convert internal value to number
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::NUMBER> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK) {
                    return MorphResult::OK;
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Converting option to vector will try to convert internal value to vector
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::VECTOR> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK) {
                    return MorphResult::OK;
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Converting option to string will try to convert internal value to string
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::STRING> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK) {
                    return MorphResult::OK;
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Converting option to hash will try to convert internal value to hash
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::HASH> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK) {
                    return MorphResult::OK;
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Optional value can be trivially converted to another option value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::OPTION> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.valueType = from.valueType;
            into.items = std::move(from.items);
            return MorphResult::OK;
        }
    };

    // Optional value can be trivially converted to list
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::LIST> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.valueType = from.valueType;
            into.items = std::move(from.items);
            return MorphResult::OK;
        }
    };

    // Converting option will place value inside map under U32 key 0
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::MAP> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.keyType = Type::U32;
            into.valueType = from.valueType;
            if (!from.items.empty()) {
                into.items.emplace_back(U32 { 0 }, std::move(from.items.front().value));
            }
            return MorphResult::OK;
        }
    };

    // Converting option to class will try to convert internal value to class
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::OPTION, Category::CLASS> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK) {
                    return MorphResult::OK;
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting lists
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting list into none will lose all information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Converting list to number will try to convert first value to number
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::NUMBER> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK) {
                    if (from.items.size() > 1) {
                        return MorphResult::LOSSY;
                    } else {
                        return MorphResult::OK;
                    }
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // If list contains numbers it will be used to fill up a vector
    // Otherwise try converting first element to vector instead
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::VECTOR> {
        static MorphResult morph (FromT& from, IntoT& into) {
            auto const category = ValueHelper::type_to_category(from.valueType);
            if (category == Category::NUMBER) {
                auto const min = std::min(into.value.size(), from.items.size());
                auto result = MorphResult::OK;
                for (size_t i = 0; i != min; ++i) {
                    std::visit([&] (auto& item) {
                        using item_t = std::remove_cvref_t<decltype(item)>;
                        if constexpr (item_t::category == Category::NUMBER) {
                            if (!convert_number(item.value, into.value[i])) {
                                result = MorphResult::LOSSY;
                            }
                        }
                    }, from.items[i].value);
                }
                if (min < into.value.size()) {
                    return MorphResult::INCOMPLETE;
                } else if (min < from.items.size()) {
                    return MorphResult::LOSSY;
                } else {
                    return result;
                }
            } else if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK) {
                    if (from.items.size() > 1) {
                        return MorphResult::LOSSY;
                    } else {
                        return MorphResult::OK;
                    }
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Converting list to string will try to convert first value to string
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::STRING> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK ) {
                    if (from.items.size() > 1) {
                        return MorphResult::LOSSY;
                    } else {
                        return MorphResult::OK;
                    }
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // Converting list to hash will try to convert first value to hash
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::HASH> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK ) {
                    if (from.items.size() > 1) {
                        return MorphResult::LOSSY;
                    } else {
                        return MorphResult::OK;
                    }
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    // First value in list will be moved to option more values will result in loss of information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::OPTION> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.valueType = from.valueType;
            into.items = std::move(from.items);
            if (into.items.size() > 1) {
                into.items.resize(1);
                return MorphResult::LOSSY;
            } else {
                return MorphResult::OK;
            }
        }
    };

    // Optional value can be trivially converted to another list
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::LIST> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.valueType = from.valueType;
            into.items = std::move(from.items);
            return MorphResult::OK;;
        }
    };

    // Convert list items to map with U32 keys
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::MAP> {
        static MorphResult morph (FromT& from, IntoT& into) {
            into.keyType = Type::U32;
            into.valueType = from.valueType;
            into.items.reserve(from.items.size());
            uint32_t count = 0;
            for (auto& item: from.items) {
                into.items.emplace_back(U32 { count }, std::move(item.value));
                ++count;
            }
            return MorphResult::OK;;
        }
    };

    // Converting list to class will try to convert first value to class
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::LIST, Category::CLASS> {
        static MorphResult morph (FromT& from, IntoT& into) {
            if (!from.items.empty()) {
                auto const result = morph_value_move(from.items.front().value, &into);
                if (result >= MorphResult::OK ) {
                    if (from.items.size() > 1) {
                        return MorphResult::LOSSY;
                    } else {
                        return MorphResult::OK;
                    }
                } else {
                    return result;
                }
            } else {
                return MorphResult::INCOMPLETE;
            }
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting map
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting map into none will lose all information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Converting map into number will result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::NUMBER> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting map into vector will result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::VECTOR> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting map into string will result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::STRING> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting map into hash will result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::HASH> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting map into option will result int loss of information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::OPTION> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.valueType = from.valueType;
            if (!from.items.empty()) {
                into.items = { Element { std::move(from.items.front().value) } };
            }
            return MorphResult::LOSSY;
        }
    };

    // Converting map into list will result in loss of information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::LIST> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.valueType = std::move(from.valueType);
            into.items.reserve(from.items.size());
            for (auto& item: from.items) {
                into.items.emplace_back(std::move(item.value));
            }
            return MorphResult::LOSSY;
        }
    };

    // Converting map into map is trivial
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::MAP> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.keyType = from.keyType;
            into.valueType = from.valueType;
            into.items = std::move(from.items);
            return MorphResult::OK;
        }
    };

    // Converting map in class will atempt to convert all keys to fnv1a hashes
    // There will be no type name so value can be considered allways incomplete!
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::MAP, Category::CLASS> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.items.reserve(from.items.size());
            for (auto& item: from.items) {
                auto key = Hash {};
                morph_value_move(item.key, &key);
                into.items.emplace_back(std::move(key.value), std::move(item.value));
            }
            return MorphResult::INCOMPLETE;
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Converting class
    /// ---------------------------------------------------------------------------------------------------------------

    // Converting class into none will result in loss of information
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::NONE> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::LOSSY;
        }
    };

    // Converting class into number will allways result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::NUMBER> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting class into vector will allways result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::VECTOR> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting class into string will allways result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::STRING> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Converting class into hash will allways result in incomplete value
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::HASH> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            return MorphResult::INCOMPLETE;
        }
    };

    // Moves class into option
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::OPTION> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Moves class into list
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::LIST> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.valueType = FromT::type;
            into.items = { Element { std::move(from) } };
            return MorphResult::OK;
        }
    };

    // Moves class into first field of new map with key type U32
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::MAP> {
        static MorphResult morph(FromT& from, IntoT& into) {
            into.keyType = Type::U32;
            into.valueType = FromT::type;
            into.items = { Pair { U32 { 0 }, std::move(from) } };
            return MorphResult::LOSSY;
        }
    };

    // Converting class into class is allways ok
    template <typename FromT, typename IntoT>
    struct morph_value_impl<FromT, IntoT, Category::CLASS, Category::CLASS> {
        static MorphResult morph([[maybe_unused]] FromT& from, [[maybe_unused]] IntoT& into) {
            into.name = std::move(from.name);
            into.items = std::move(from.items);
            return MorphResult::OK;
        }
    };

    /// ---------------------------------------------------------------------------------------------------------------
    /// Variant wrappers and public API
    /// ---------------------------------------------------------------------------------------------------------------

    MorphResult morph_value_move(Value& from, ValuePtr into) {
        return std::visit([into] (auto& from) {
            return std::visit([&from] (auto into) {
                using from_t = std::remove_cvref_t<decltype(from)>;
                using into_t = std::remove_cvref_t<decltype(*into)>;
                if constexpr (std::is_same_v<from_t, into_t>) {
                    *into = std::move(from);
                    return MorphResult::UNCHANGED;
                } else {
                    return morph_value_impl<from_t, into_t>::morph(from, *into);
                }
            }, into);
        }, from);
    }

    MorphResult morph_value(Value& from, [[maybe_unused]] Type intoType) {
        auto into = ValueHelper::type_to_value(intoType);
        return std::visit([&from] (auto& into) {
            auto result = std::visit([&into] (auto& from) {
                using from_t = std::remove_cvref_t<decltype(from)>;
                using into_t = std::remove_cvref_t<decltype(into)>;
                if constexpr (std::is_same_v<from_t, into_t>) {
                    into = std::move(from);
                    return MorphResult::UNCHANGED;
                } else {
                    return morph_value_impl<from_t, into_t>::morph(from, into);
                }
            }, from);
            from = std::move(into);
            return result;
        }, into);
    }
}
