/*
  This is basic_io.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#include "basic_io.h"

#include <iostream>
#include <stdexcept>

#include "lattice.h"
#include "lietype.h"

/*****************************************************************************

        Chapter I -- operator<< functions

  NOTE: there are more definitions, given as templates in basic_io_def.h

******************************************************************************/

namespace atlas {


/*
  Synopsis: output of a lattice element.

  Uses the template seqPrint. It is output as a bracket-enclosed,
  comma-separated list.
*/
std::ostream& operator<< (std::ostream& strm,
			  const latticetypes::LatticeElt& v)
{
  latticetypes::LatticeElt::const_iterator first = v.begin();
  latticetypes::LatticeElt::const_iterator last = v.end();

  return basic_io::seqPrint(strm, first, last, ",", "[", "]");
}

/******** from lietype *******************************************************/


/*
  Synopsis: outputs a simple Lie type.
*/
std::ostream& operator<< (std::ostream& strm,
			  const lietype::SimpleLieType& slt)
{
  lietype::TypeLetter x = slt.type();
  size_t l = slt.rank();

  return strm << x << l;
}

std::ostream& operator<< (std::ostream& strm, const lietype::LieType& lt)
{
  lietype::LieType::const_iterator first = lt.begin();
  lietype::LieType::const_iterator last = lt.end();

  return basic_io::seqPrint(strm,first,last,".");
}

/******** from weyl **********************************************************/


/*
  Synopsis: outputs w as a string of digits

  Originally this was without separators, with the following
  NOTE: this is satisfactory only if the rank is < 10; otherwise, use
  prettyprint::printWeylWord, that will give dot-separated lists.

  Unfortunately this operator is often called automatically, by template
  functions, so there is no place to insert that distinction into the code.
  Therefore we have inserted commas here. MvL
*/
std::ostream& operator<< (std::ostream& strm, const weyl::WeylWord& w)
{
  if (w.size()==0)
    return strm << 'e';

  for (size_t j = 0; j < w.size(); ++j) {
    unsigned a = w[j]+1;
    strm << a;
    if (j+1<w.size()) strm << ',';
  }

  return strm;
}

namespace basic_io {

unsigned long long read_var_bytes(unsigned int n,std::istream& in)
{ switch(n)
  { case 1: return read_bytes<1>(in);
    case 2: return read_bytes<2>(in);
    case 3: return read_bytes<3>(in);
    case 4: return read_bytes<4>(in);
    case 5: return read_bytes<5>(in);
    case 6: return read_bytes<6>(in);
    case 7: return read_bytes<7>(in);
    case 8: return read_bytes<8>(in);
  default: throw std::runtime_error("Illegal read_var_bytes");
  }
}

void put_int (unsigned int val, std::ostream& out) { write_bytes<4>(val,out); }
void write_bytes(unsigned int n, unsigned long long val, std::ostream& out)
{ switch(n)
  { case 1: return write_bytes<1>(val,out);
    case 2: return write_bytes<2>(val,out);
    case 3: return write_bytes<3>(val,out);
    case 4: return write_bytes<4>(val,out);
    case 5: return write_bytes<5>(val,out);
    case 6: return write_bytes<6>(val,out);
    case 7: return write_bytes<7>(val,out);
    case 8: return write_bytes<8>(val,out);
    default: throw std::runtime_error("Illegal write__bytes");
  }
}

} // namespace basic_io

} // namespace atlas
