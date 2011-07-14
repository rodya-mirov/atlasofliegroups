/*!
\file
  This is arithmetic_fwd.h
*/
/*
  Copyright (C) 2011 Marc van Leeuwen
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#ifndef ARITHMETIC_FWD_H  /* guard against multiple inclusions */
#define ARITHMETIC_FWD_H

#include <vector>

/******** forward type declarations ******************************************/

namespace atlas {

namespace arithmetic {

  class Rational;
  typedef std::vector<Rational> RationalList;


} // |namespace arithmetic|

} // |namespace atlas|

#endif