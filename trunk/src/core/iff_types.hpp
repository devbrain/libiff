/*
 * iff_types.hpp
 *
 *  Created on: 07/02/2010
 *      Author: igorgu
 */

#ifndef IFF_TYPES_HPP_
#define IFF_TYPES_HPP_

#if defined(_MSC_VER)
#define uint64_t unsigned __int64;
#define uint32_t unsigned int
#define uint16_t unsigned short
#define uint8_t  unsigned char
#else
#include <stdint.h>
#endif
#include <fstream>

typedef uint32_t iff_id_t;
typedef uint32_t iff_size_t;

typedef std::streamoff  offset_t;



#endif /* IFF_TYPES_HPP_ */
