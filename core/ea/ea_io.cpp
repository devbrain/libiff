#include "ea_io.hpp"

static const int __is_big_endian_ = 1;
#define is_bigendian() ( (*(char*)&__is_big_endian_) == 0 )

static int reverse_int32 (int i)
{
  unsigned char c1, c2, c3, c4;

  if (is_bigendian ())
    {
      return i;
    }
  c1 = i & 255;
  c2 = (i >> 8) & 255;
  c3 = (i >> 16) & 255;
  c4 = (i >> 24) & 255;

  return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4;
}


ea_io_c::word_t ea_io_c::read  (std::istream& is)
{
  uint32_t v;
  is.read ((char*)&v, 4);
  return  reverse_int32(v);
}

void ea_io_c::write (std::ostream& os, word_t v)
{
  uint32_t w = reverse_int32 (v);
  os.write ((char*)&w, 4);
}

bool ea_io_c::has_header ()
{
  return false;
}

unsigned ea_io_c::words_in_header ()
{
  return 1;
}


bool ea_io_c::check_header (word_t* hdr)
{
  if (!hdr)
    {
      return false;
    }
  id_c id (hdr [0]);
  return is_group (id);
}

static id_c FORM ('F', 'O', 'R', 'M');
static id_c LIST ('L', 'I', 'S', 'T');
static id_c CAT  ('C', 'A', 'T', ' ');

bool ea_io_c::is_group (const id_c& id)
{
  if (id == FORM)
    {
      return true;
    }
  if (id == LIST)
    {
      return true;
    }
  if (id == CAT)
    {
      return true;
    }
  return false;
}

std::streamsize ea_io_c::real_size (word_t size)
{
  if (size % 2 == 0)
    {
      return size;
    }
  return size + 1;
}

bool ea_io_c::group_has_tag ()
{
  return true;
}

bool ea_io_c::should_start_with_group ()
{
  return true;
}
