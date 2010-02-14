/*
 * id.hpp
 *
 *  Created on: 07/02/2010
 *      Author: igorgu
 */

#ifndef ID_HPP_
#define ID_HPP_

#include <iostream>
#include "core/iff_types.hpp"

class iff_reader_c;
class id_c;

bool operator == (const id_c& a, const id_c& b);
bool operator != (const id_c& a, const id_c& b);
bool operator <  (const id_c& a, const id_c& b);
bool operator <= (const id_c& a, const id_c& b);
bool operator >  (const id_c& a, const id_c& b);
bool operator >= (const id_c& a, const id_c& b);

std::ostream& operator << (std::ostream& os, const id_c& b);

class id_c
{
	friend class iff_reader_c;

	friend bool operator == (const id_c& a, const id_c& b);
	friend bool operator != (const id_c& a, const id_c& b);
	friend bool operator <  (const id_c& a, const id_c& b);
	friend bool operator <= (const id_c& a, const id_c& b);
	friend bool operator >  (const id_c& a, const id_c& b);
	friend bool operator >= (const id_c& a, const id_c& b);
	friend std::ostream& operator << (std::ostream& os, const id_c& b);

public:
	id_c(char a, char b, char c, char d);
	~id_c();
	id_c (iff_id_t id);
private:
	iff_id_t m_id;
};

#endif /* ID_HPP_ */
