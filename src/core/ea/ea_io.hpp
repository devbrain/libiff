#ifndef __IFF_CORE_EA_IO_HPP__
#define __IFF_CORE_EA_IO_HPP__

#include "core/iff_types.hpp"
#include "core/ea/id.hpp"

namespace iff
{
  namespace ea
  {
    class io_c 
    {
    public:
      typedef uint32_t size_type_t;
      typedef id_c     id_t;
    public:
      static bool     has_header ();
      static unsigned bytes_in_header ();
      static bool     check_header (const char* hdr);
      static bool     should_start_with_group ();
      static bool     is_group     (const id_c& id);
      static bool     group_has_tag ();

      static std::streamsize real_size (size_type_t size);
      static std::streamsize size_of_id ();

      static bool read_group_header (std::istream& is, id_t& id, size_type_t& size, 
				     std::streamsize& total_size);
      static bool read_group_id     (std::istream& is, id_t& id, std::streamsize& size);
      
    };
  } // ns ea
} // ns ea
#endif
