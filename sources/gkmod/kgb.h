/*!
\file
\brief Class definition and function declarations for the class KGB
representing orbits of K on G/B.
*/
/*
  This is kgb.h

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  See file main.cpp for full copyright notice
*/

#ifndef KGB_H  /* guard against multiple inclusions */
#define KGB_H

#include "kgb_fwd.h"

#include "bitmap_fwd.h"
#include "bruhat_fwd.h"
#include "realredgp_fwd.h"
#include "weyl_fwd.h"
#include "weyl.h"

#include "gradings.h"

namespace atlas {

/******** function declarations *********************************************/

/******** constant declarations *********************************************/

namespace kgb {

const KGBElt UndefKGB = ~0ul;

}

/******** type definitions **************************************************/

namespace kgb {

struct KGBInfo // per KGB element information
{
  gradings::Status status; ///< status of each simple root for this element
  unsigned int length; ///< dimension of the K orbit on G/B, minus minimal one
  unsigned int cartan; ///< records to which Cartan class this element belongs
  Descent desc; ///< flags which simple reflections give a descent

  KGBInfo(unsigned int l, unsigned int c) : status(),length(l),cartan(c),desc()
  {} // set length explicitly, both other fields set to their default value
};

  /*!
\brief Represents the orbits of K on G/B for a particular real form.

Each orbit x defines an involution theta_x of H, coming from the
extended Weyl group.  The collection of orbits defining the same
involution is parametrized using the Fiber class: in the end it is a
particular orbit of the imaginary Weyl group on the fiber group.

These orbits are needed first of all for the parametrization of
irreducible representations of the real form (see the class Block).

In this class, an orbit is represented by KGBElt, which is a number specifying
the position of the orbit on a list. For each number, the involution theta_x
is retained (as d_involution[KGBElt]), but not the fiber information
distinguishing different orbits with the same involution. Instead, the class
retains the cross action of (each simple reflection in) W on orbits, and the
Cayley transform. Each of these is stored as a vector (indexed by orbit
numbers) of lists of KGBElt's, one for each simple reflection (often the
KGBElt UndefKGB = ~0 in the case of the Cayley transform)

The actual construction of the orbit is carried out by the (no longer
derived!) class KGBHelper. It is that class that works with actual elements of
the Tits group (which encode both a twisted involution and an element of the
associlated fiber group). After the construction, some of that detailed
information is discarded. What is retained by the class KGB is mainly the
Cayley transforms (always going from more compact to less compact involutions)
and cross actions; the associated twisted involution and |KGBInfo| record is
stored as well. The inverse Cayley transforms are deduced from the forward
ones and are also stored. This information is all that is used by the
Kazhdan-Lusztig algorithm.
*/

class KGB {

  enum State { BruhatConstructed, NumStates };

/*!
\brief Semisimple rank of the underlying group.
*/
  size_t d_rank;

/*!
 \brief The list d_cross[j] lists the images of the cross action
of simple root \#j on the KGB elements.
*/
  std::vector<KGBEltList> d_cross;

/*!
\brief The list |d_cayley[j]| lists the images of the Cayley transform
action of the simple root |\#j| on KGB elements, or |UndefKGB| if not defined
*/
std::vector<KGBEltList> d_cayley;

/*! \brief The list d_inverseCayley[j] lists the images of the inverse Cayley
transform of simple root \#j on KGB elements.

For fixed |j| each KGB element can have up to two inverse Cayley images. For
those for which simple root \#j is not real, both components will be
|UndefKGB|, and otherwise the second component might still be |UndefKGB|.
*/
  std::vector<KGBEltPairList> d_inverseCayley;

/*!
\brief Information about individual KGB elements
*/
  std::vector<KGBInfo> d_info;

/*!
\brief Twisted Weyl group element defining Cartan involution for KGB
element \#j.
*/
  weyl::TwistedInvolutionList d_involution;

/*!
\brief Bit 0 flags whether or not the Bruhat order on KGB has already
been constructed.
*/
  bitset::BitSet<NumStates> d_state;

/*!
\brief Pointer to the Bruhat order on KGB.

The class BruhatOrder provides the symmetrized bitmatrix of the
partial order, and the Hasse diagram of covering relations.
*/
bruhat::BruhatOrder* d_bruhat;

/*!
\brief Pointer (non owned) to the (twisted) Weyl group.
*/
  const weyl::WeylGroup* d_weylGroup;

 public:

// constructors and destructors
  explicit KGB(realredgp::RealReductiveGroup&);

  ~KGB();

// copy, assignment and swap

// accessors

/*!
\brief Pointer to the Bruhat order on KGB.

The class BruhatOrder as written by Fokko provided the symmetrized
bitmatrix of the partial order, and the Hasse diagram of covering
relations.  Marc van Leeuwen and I can see no application for the
symmetrized bitmatrix at present, so this is now replaced by the
bitmatrix of the relation x <= y, which is half the size. DV.
*/
  const bruhat::BruhatOrder& bruhatOrder() const {
    return *d_bruhat;
  }

/*! \brief Takes the Cayley transform of KGB element \#x in the direction of
simple root \#s; returns |UndefKGB| unless that root was noncompact imaginary.
*/
  KGBElt cayley(size_t s, KGBElt x) const {
    return d_cayley[s][x];
  }

/*!
  \brief Returns whether involution(x) < involution(y).

  Explanation: the ordering is involution-length first, then weyl-length, then
  order by representative Weyl elements (weyl::TwistedInvolution::operator<).
  This is only a partial ordering, that does not distinguish elements of a
  fiber over one same twisted involution.
*/
  bool compare(KGBElt x, KGBElt y) const {
  if      (length(x) != length(y))
    return length(x) < length(y);
  else if (weylLength(x) != weylLength(y))
    return weylLength(x) < weylLength(y);
  else
    return involution(x) < involution(y);
}


/*!
 \brief Applies the cross action of simple root \#s to KGB element \#x.
*/
  KGBElt cross(size_t s, KGBElt x) const {
    return d_cross[s][x];
  }

/*!
\brief Applies the inverse Cayley transform in simple root \#s to
KGB element \#x.

If simple root \#s is not real, returns a pair of UndefKGB.
*/
  KGBEltPair inverseCayley(size_t s, KGBElt x) const {
    return d_inverseCayley[s][x];
  }

/*!
\brief Semisimple rank of the underlying group.
*/
  size_t rank() const {
    return d_rank;
  }


/*!
\brief Number of K orbits on G/B.
*/
  size_t size() const {
    return d_info.size();
  }


/*! \brief Returns a Bitset flagging the simple roots which are descents for
KGB element \#x.
*/
  const Descent& descent(KGBElt x) const {
    return d_info[x].desc;
  }
/*!
\brief Length (dimension of the K orbit on G/B, minus minimal orbit dimension)
for KGB element \#x.
*/
  size_t length(KGBElt x) const {
    return d_info[x].length;
  }

/*!
\brief Cartan class associated to KGB element \#x.
*/
  size_t Cartan_class(KGBElt x) const {
    return d_info[x].cartan;
  }



/*!
\brief Flag telling whether each simple root is real, complex, imaginary
compact, or imaginary noncompact for KGB element \#x.
 */
  const gradings::Status& status(KGBElt x) const {
    return d_info[x].status;
  }


/*!
\brief Tells whether simple root \#s is real, complex, imaginary compact,
or imaginary noncompact for KGB element \#x.
 */
  gradings::Status::Value status(size_t s, KGBElt x) const {
    return status(x)[s];
  }

/*!
  \brief Tells whether simple root \# s is a descent generator for KGB
  element \#x.

  This only depends on the twisted involution \f$\tau\f$ associated to |x|: it
  holds whenever the simple reflection for |s| either makes \f$\tau\f$ shorter
  by twisted conjugation (|s| is a complex descent for |tau|), or if it
  twisted commutes by left multiplication (|s| is real for |tau|).
*/
bool isDescent(size_t s, KGBElt x) const
{
  return d_info[x].desc.test(s);
}

/*!
  \brief Tells whether simple root \#s is an ascent generator for KGB
  element \#x.

  This does not depend only on the twisted involution: it holds for all
  complex roots that are not descents, but for imaginary roots only if they
  noncompact.
*/
bool isAscent(size_t s, KGBElt x) const
{
  return not isDescent(s,x)
     and not (status(s,x) == gradings::Status::ImaginaryCompact);
}

/*!
  \brief Returns the root datum involution corresponding to z.

  In fact, returns the twisted involution $w$ with \f$w.\delta = \tau\f$.
*/
  const weyl::TwistedInvolution& involution(KGBElt x) const {
    return d_involution[x];
  }

  KGBEltPair tauPacket(const weyl::TwistedInvolution&) const;

/*!
\brief The (twisted) Weyl group.
*/
  const weyl::WeylGroup& weylGroup() const {
    return *d_weylGroup;
  }

  size_t weylLength(KGBElt) const;

// manipulators
  void fillBruhat(); // might throw MemoryOverflow

// private methods, used during construction



};

}

}

#endif
