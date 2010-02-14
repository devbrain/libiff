/*
 * id.hpp
 *
 *  Created on: 07/02/2010
 *      Author: igorgu
 */

#ifndef __IFF_CORE_EA_ID_HPP__
#define __IFF_CORE_EA_ID_HPP__

#include <iostream>
#include "core/iff_types.hpp"

namespace iff
{
  namespace ea
  {
    class id_c;

    bool operator == (const id_c& a, const id_c& b);
    bool operator != (const id_c& a, const id_c& b);
    bool operator <  (const id_c& a, const id_c& b);
    bool operator <= (const id_c& a, const id_c& b);
    bool operator >  (const id_c& a, const id_c& b);
    bool operator >= (const id_c& a, const id_c& b);

    class id_c
    {
      friend bool operator == (const id_c& a, const id_c& b);
      friend bool operator != (const id_c& a, const id_c& b);
      friend bool operator <  (const id_c& a, const id_c& b);
      friend bool operator <= (const id_c& a, const id_c& b);
      friend bool operator >  (const id_c& a, const id_c& b);
      friend bool operator >= (const id_c& a, const id_c& b);

    public:
      id_c(char a, char b, char c, char d);
      ~id_c();
      id_c (iff_id_t id);
      
      std::string to_string () const;
    private:
      iff_id_t m_id;
    };

  } // ns ea
} // ns iff
#endif 
