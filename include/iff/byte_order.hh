/**
 * @file byte_order.hh
 * @brief Byte order (endianness) utilities for IFF/RIFF formats
 * @author Igor
 * @date 12/08/2025
 */

#pragma once

#include <iff/endian.hh>

namespace iff {
    /**
     * @enum byte_order
     * @brief Byte order (endianness) for reading/writing multi-byte values
     */
    enum class byte_order {
        little, ///< Little-endian (used by RIFF, RF64 formats)
        big     ///< Big-endian (used by IFF-85, RIFX formats)
    };

    /**
     * @brief Check if given byte order matches the native system byte order
     * @param bo Byte order to check
     * @return True if the byte order matches the system's native byte order
     * 
     * This is used to determine if byte swapping is needed when reading
     * or writing multi-byte values.
     */
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
