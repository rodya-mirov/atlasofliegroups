/*!
\file
  This is arithmetic.h
*/
/*
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  Modified to add a class modular_int for modular arithmetic,
  for a modulus up tp 256, by Marc van Leeuwen

  See file main.cpp for full copyright notice
*/

#ifndef ARITHMETIC_H  /* guard against multiple inclusions */
#define ARITHMETIC_H


#include <iostream>

/******** function declarations **********************************************/

namespace atlas {

namespace arithmetic {

  class modular_int
    { static unsigned int modulus;
      unsigned char d_val; // value, a remainder mod modulus

    public:
      static void set_modulus(unsigned int n) { modulus=n; }

      modular_int() : d_val(0) {}
      modular_int(unsigned int n) : d_val(n<modulus ? n : n % modulus) {}

      void operator+=(modular_int b)
        { unsigned s=d_val+b.d_val; d_val= s<modulus ? s : s-modulus; }
      modular_int operator+(modular_int b) const
	{ modular_int a(*this); a+=b; return a; }
      modular_int operator-() const
	{  modular_int a; if (d_val!=0) a.d_val=modulus-d_val; return a;  }
      modular_int operator-(modular_int b) const
	{ modular_int a(*this); a+= -b; return a; }
      void operator*=(modular_int b)
        { unsigned s=d_val*b.d_val; d_val=s % modulus; }

      bool operator<(modular_int b) const { return d_val<b.d_val; }
      bool operator==(modular_int b) const { return d_val==b.d_val; }
      bool operator!=(modular_int b) const { return d_val!=b.d_val; }

      operator unsigned char () { return d_val; }
      unsigned int unsigned_value() { return d_val; }
    };

  // inlined
  unsigned long gcd (unsigned long, unsigned long);

  unsigned long lcm (unsigned long, unsigned long);

  // inlined
  unsigned long& modAdd(unsigned long&, unsigned long, unsigned long);

  unsigned long& modProd(unsigned long&, unsigned long, unsigned long);

  template<typename C> unsigned long remainder(C, unsigned long);

}

/******** inline function definitions ***************************************/

namespace arithmetic {

  inline unsigned long gcd (long a, unsigned long b) {
    if (a < 0)
      return gcd(static_cast<unsigned long>(-a),b);
    else
      return gcd(static_cast<unsigned long>(a),b);
  }

  inline unsigned long& modAdd(unsigned long& a, unsigned long b,
			      unsigned long n) {
    if (a < n-b)
      a += b;
    else
      a -= n-b;
    return a;
  }

  inline std::ostream& operator<<(std::ostream& out, modular_int a)
    { out << a.unsigned_value(); return out; }

}

}

#include "arithmetic_def.h"

#endif
