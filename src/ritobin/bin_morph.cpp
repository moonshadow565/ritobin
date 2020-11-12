#include "bin.hpp"

namespace ritobin {
    template<typename FromT, typename ToT, Category = FromT::CATEGORY, Category = ToT::Category>
    struct morph_impl;


}
