#ifndef __IFF_CORE_IO_HPP__
#define __IFF_CORE_IO_HPP__

#include <iostream>
#include "core/iff_types.hpp"

namespace iff
{
  enum endianity_t 
    {
      eLITTLE_ENDIAN,
      eBIG_ENDIAN
    };

  uint64_t read_64 (std::istream& is, endianity_t e);
  uint32_t read_32 (std::istream& is, endianity_t e);
  uint16_t read_16 (std::istream& is, endianity_t e);
  uint8_t  read_8  (std::istream& is, endianity_t e);

  void write_64 (std::ostream& os, uint64_t v, endianity_t e);
  void write_32 (std::ostream& os, uint32_t v, endianity_t e);
  void write_16 (std::ostream& os, uint16_t v, endianity_t e);
  void write_8  (std::ostream& os, uint8_t  v, endianity_t e);
} // ns iff


#endif
