#include "core/iff_io.hpp"

static const int __is_big_endian_ = 1;
#define is_bigendian() ( (*(char*)&__is_big_endian_) == 0 )


static uint16_t reverse_int32 (uint16_t i)
{
  uint8_t c1, c2;

  c1 = i & 255;
  c2 = (i >> 8) & 255;

  return ((uint16_t)c3 << 8) + c4;
}
// -----------------------------------------------------------------------
static uint32_t reverse_int32 (uint32_t i)
{
  uint8_t c1, c2, c3, c4;

  c1 = i & 255;
  c2 = (i >> 8) & 255;
  c3 = (i >> 16) & 255;
  c4 = (i >> 24) & 255;

  return ((uint32_t)c1 << 24) + ((uint32_t)c2 << 16) + ((uint32_t)c3 << 8) + c4;
}

// -----------------------------------------------------------------------
static uint64_t reverse_int32 (uint64_t i)
{
  uint8_t c1, c2, c3, c4;
  uint8_t c5, c6, c7, c8;


  c1 = i & 255;
  c2 = (i >> 8) & 255;
  c3 = (i >> 16) & 255;
  c4 = (i >> 24) & 255;

  c5 = (i >> 32) & 255;
  c6 = (i >> 40) & 255;
  c7 = (i >> 48) & 255;
  c8 = (i >> 56) & 255;

  return ((uint64_t)c1 << 56) + ((uint64_t)c2 << 48) + ((uint64_t)c3 << 40) + ((uint64_t)c4 << 32) +
    ((uint64_t)c5 << 24) + ((uint64_t)c6 << 16) + ((uint64_t)c7 << 8) + (uint64_t)c8;
    
}

namespace iff 
{
  uint64_t read_64 (std::istream& is, endianity_t e);
  uint32_t read_32 (std::istream& is, endianity_t e);
  uint16_t read_16 (std::istream& is, endianity_t e);
  uint8_t  read_8  (std::istream& is, endianity_t e);

  void write_64 (std::ostream& os, uint64_t v, endianity_t e);
  void write_32 (std::ostream& os, uint32_t v, endianity_t e);
  void write_16 (std::ostream& os, uint16_t v, endianity_t e);
  void write_8  (std::ostream& os, uint8_t  v, endianity_t e);
}
