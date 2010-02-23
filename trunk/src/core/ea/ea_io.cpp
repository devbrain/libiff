#include <string.h>
#include "core/ea/ea_io.hpp"


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

typedef uint32_t word_t;

static word_t read  (std::istream& is)
{
  uint32_t v;
  is.read ((char*)&v, 4);
  return  reverse_int32(v);
}
// -----------------------------------------------------------------
static void write (std::ostream& os, word_t v)
{
  uint32_t w = reverse_int32 (v);
  os.write ((char*)&w, 4);
}

namespace iff
{
  namespace ea
  {
    
    // -----------------------------------------------------------------
    bool io_c::has_header ()
    {
      return false;
    }
    // -----------------------------------------------------------------
    unsigned io_c::bytes_in_header ()
    {
      return 4;
    }
    // -----------------------------------------------------------------
    bool io_c::check_header (const char* hdr)
    {
      if (!hdr)
	{
	  return false;
	}

      word_t w;
      memcpy (&w, hdr, bytes_in_header ());
      
      id_c id (reverse_int32(w));
      return is_group (id);
    }
    // -----------------------------------------------------------------
    bool io_c::is_group (const id_c& id)
    {
      static const id_c FORM ('F', 'O', 'R', 'M');
      static const id_c LIST ('L', 'I', 'S', 'T');
      static const id_c CAT  ('C', 'A', 'T', ' ');
      
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
    // -----------------------------------------------------------------
    std::streamsize io_c::real_size (size_type_t size)
    {
      if (size % 2 == 0)
	{
	  return size;
	}
      return size + 1;
    }
    // -----------------------------------------------------------------
    bool io_c::group_has_tag ()
    {
      return true;
    }
    // -----------------------------------------------------------------
    bool io_c::should_start_with_group ()
    {
      return true;
    }
    // -----------------------------------------------------------------
    bool io_c::read_group_header (std::istream& is, id_t& id, size_type_t& size,
				  std::streamsize& total_size)
    {
      word_t i = read (is);
      word_t s = read (is);
      if (!is.good ())
	{
	  return false;
	}
      id   = id_c (i);
      size = s;
      total_size = sizeof (i) + sizeof (s);
      return true;
    }
    // -----------------------------------------------------------------
    bool io_c::read_group_id (std::istream& is, id_t& id, std::streamsize& size)
    {
      word_t i = read (is);
      if (!is.good ())
	{
	  return false;
	}
      id   = id_c (i);
      size = sizeof (word_t);
      return true;
    }
    // -----------------------------------------------------------------
    std::streamsize io_c::size_of_id ()
    {
      return sizeof (word_t);
    }
  } // ns ea
} // ne iff
