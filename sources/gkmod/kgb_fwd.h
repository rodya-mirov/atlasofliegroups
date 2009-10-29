/*!
\file
\brief Class declaration and type definitions for class KGB.
*/
/*
  This is kgb_fwd.h

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#ifndef KGB_FWD_H  /* guard against multiple inclusions */
#define KGB_FWD_H

#include "bitset_fwd.h"

namespace atlas {

namespace kgb {


/******** type declarations *************************************************/

  class KGB;

  typedef unsigned int KGBElt;
  typedef std::vector<KGBElt> KGBEltList;

  typedef std::pair<KGBElt,KGBElt> KGBEltPair;
  typedef std::vector<KGBEltPair> KGBEltPairList;

  typedef bitset::RankFlags Descent;

/******** constant declarations *********************************************/

static const KGBElt UndefKGB = ~0u;

} // |namespace kgb|

} // |namespace atlas|

#endif
