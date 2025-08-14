//
// Created by igor on 12/08/2025.
//

#pragma once

#include <iff/endian.hh>

namespace iff {
    enum class byte_order {
        little, // RIFF, RF64
        big // IFF, RIFX
    };

    inline bool byte_order_native(byte_order bo) {
        switch (bo) {
            case byte_order::little:
                return is_little_endian;
            case byte_order::big:
                return is_big_endian;
        }
        // make compiler happy
        return false;
    }
}
