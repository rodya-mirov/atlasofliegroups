/*
  This is bruhat.cpp
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.4 

  See file main.cpp for full copyright notice
*/

#include "bruhat.h"

#include <algorithm>

/*

*/

namespace atlas {

namespace {

  void pause() {;}
}

/*****************************************************************************

        Chapter I -- The BruhatOrder class

******************************************************************************/

namespace bruhat {

BruhatOrder::BruhatOrder(const std::vector<set::SetEltList>& hd)
  :d_hasse(hd)

/*
  Synopsis: constructs the Bruhat ordering from the datum of the Hasse diagram.

  Algorithm: easy poset construction from the hasse list.

  NOTE: it is slightly unpleasant to copy the hasse list.
  NOTE: might throw, most likely from memory overflow.
*/

{
  using namespace poset;

  SymmetricPoset p(d_hasse); // could throw

  // commit
  d_poset.swap(p);
}

}

}
