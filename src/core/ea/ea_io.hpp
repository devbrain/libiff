#ifndef __EA_IO_HPP__
#define __EA_IO_HPP__

#include "core/iff_types.hpp"
#include "core/ea/id.hpp"

class ea_io_c 
{
public:
  typedef uint32_t word_t;
  typedef id_c     id_t;
public:
  static word_t read  (std::istream& is);
  static void   write (std::ostream& os, word_t v);
  
  static bool     has_header ();
  static unsigned words_in_header ();
  static bool     check_header (word_t* hdr);
  static bool     should_start_with_group ();
  static bool     is_group     (const id_c& id);
  static bool     group_has_tag ();
  static std::streamsize real_size (word_t size);
};

#endif
