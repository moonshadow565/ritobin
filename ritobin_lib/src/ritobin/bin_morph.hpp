#ifndef BIN_MORPH_HPP
#define BIN_MORPH_HPP

#include "bin_types.hpp"

namespace ritobin {
    enum class MorphResult {
        // Invalid key or value type
        FAIL = -3,
        // New value is't fully initialized
        INCOMPLETE = 2,
        // New value is intialized but only partially preserves old value
        LOSSY = -1,
        // New value is initialized and preserves old value
        OK = 0,
        // New value is exactly same as old value
        UNCHANGED = 1,
    };

    extern MorphResult morph_value(Value& from, Type intoType);
    extern MorphResult morph_type_key(Value& value, Type newType);
    extern MorphResult morph_type_value(Value& value, Type newType);
}

#endif // BIN_MORPH_HPP
