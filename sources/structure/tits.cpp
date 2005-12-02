/*
  This is tits.cpp
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.4 

  See file main.cpp for full copyright notice
*/

#include "tits.h"

#include "lattice.h"
#include "rootdata.h"

/*
  This module contains an implementation of a slight variant of the Tits group
  (also called extended Weyl group) as defined in J. Tits, J. of Algebra 4 
  (1966), pp. 96-116.

  The slight variant is that we include all elements of order two in the torus,
  instead of just the subgroup generated by the m_alpha (denoted h_alpha in
  Tits's paper.) The actual Tits group may be defined by generators 
  sigma_alpha, alpha simple, subject to the braid relations and sigma_alpha^2=
  m_alpha; to get our group we just add a basis of elements of T(2) as 
  additional generators, and express the m_alpha in this basis.

  On a practical level, because the sigma_alpha satisfy the braid relations,
  any element of the Weyl group has a canonical lifting in the Tits group;
  so we may uniquly represent any element of the Tits group as a pair (w,t),
  with t in T(2) and w in W. The multiplication rules have to be thoroughly
  changed, though, to take into account the new relations.

  We have not tried to be optimally efficient here, as it is not expected that
  Tits computations will be significant computationally.
*/

namespace atlas {

namespace {

using namespace tits;

void pause() {;}

}

/****************************************************************************

        Chapter II -- The TitsGroup class

  ... explain here when it is stable

*****************************************************************************/

namespace tits {

TitsGroup::TitsGroup(const rootdata::RootDatum& rd, 
		     const latticetypes::LatticeMatrix& d)
  :d_rank(rd.rank())

/*
  Synopsis : constructs the Tits group corresponding to the root datum rd, and
  the fundamental involution d.
*/

{
  using namespace lattice;
  using namespace latticetypes;
  using namespace rootdata;
  using namespace weyl;

  pause();

  LatticeMatrix c;
  cartanMatrix(c,rd);

  makeTwist(d_twist,d,rd);

  d_weyl = new WeylGroup(c,&d_twist);

  d_simpleRoot.resize(rd.semisimpleRank());
  d_simpleCoroot.resize(rd.semisimpleRank());

  for (size_t j = 0; j < rd.semisimpleRank(); ++j) {
    mod2(d_simpleRoot[j],rd.simpleRoot(j));
    mod2(d_simpleCoroot[j],rd.simpleCoroot(j));
  }
}

TitsGroup::~TitsGroup()

{
  delete d_weyl;
}

void TitsGroup::invConjugate(TorusPart& x, const weyl::WeylElt& w) const

/*
  Synopsis: x -> w^{-1}.x.w in the Tits group.

  Algorithm: take a reduced expression for w, and apply the chain of reflexions
  left-to-right.

  Note: a more sophisticated algorithm would involve precomputing the 
  conjugation matrices for the various pieces of w. Don't do it unless it turns
  out to really matter.
*/

{
  using namespace weyl;

  WeylWord ww;
  d_weyl->out(ww,w);

  for (size_t j = 0; j < ww.size(); ++j)
    reflection(x,ww[j]);

  return;
}

void TitsGroup::leftProd(TitsElt& a, weyl::Generator s) const

/*
  Synopsis: a -> s*a.

  Algorithm: similar to prod, but using weyl::leftProd. The main difference
  is that now the m_s comes out on the _left_, so we must conjugate it through
  a.w().
*/

{
  unsigned long l = length(a);

  d_weyl->leftProd(a.w(),s);

  if (length(a) < l) { // need to conjugate a torus part
    TorusPart x = d_simpleCoroot[s];
    invConjugate(x,a.w());
    a += x;
  }

  return;
}

void TitsGroup::prod(TitsElt& a, weyl::Generator s) const

/*
  Synopsis: a *= s.

  Algorithm: This is surprisingly simple: multiplying by s just amounts to 
  reflecting a.t() through s, then doing w -> ws in the Weyl group. If the 
  length goes down, we notice that the m_alpha that comes out on the right is 
  necessarily m_s (just because we may write a.w() = v'.s (reduced), so 
  v.s = v'.m_s in the Tits group.) So we just need to add m_s to a.t() in that 
  case. Finally of course we need to add y to the resulting x.

  The upshot is a multiplication algorithm that is about as fast as in the
  Weyl group!
*/

{
  using namespace weyl;

  reflection(a.t(),s);
  if (d_weyl->prod(a.w(),s) == -1) // length reduction
    a += d_simpleCoroot[s];

  return;
}

void TitsGroup::prod(TitsElt& a, const TitsElt& b) const

/*
  Synopsis: a *= b.

  Algorithm: The algorith is to mutliply a successively by the various 
  generators in a reduced expression for b.w(), and then adding b.t().
*/

{
  using namespace weyl;

  WeylWord bw;
  d_weyl->out(bw,b.w());

  for (size_t j = 0; j < bw.size(); ++j) {
    Generator s = bw[j];
    prod(a,s);
  }

  a += b.t();

  return;
}

/******** manipulators *******************************************************/

void TitsGroup::swap(TitsGroup& other)

{
  d_simpleRoot.swap(other.d_simpleRoot);
  d_simpleCoroot.swap(other.d_simpleCoroot);

  std::swap(d_weyl,other.d_weyl);
  std::swap(d_rank,other.d_rank);

  // STL swap doesn't work for arrays!

  weyl::Generator tmp[constants::RANK_MAX];

  memcpy(tmp,d_twist,constants::RANK_MAX);
  memcpy(d_twist,other.d_twist,constants::RANK_MAX);
  memcpy(other.d_twist,tmp,constants::RANK_MAX);

  return;
}

}

/****************************************************************************

        Chapter II -- Functions declared in tits.h

  ... explain here when it is stable

*****************************************************************************/

namespace tits {

void makeTwist(weyl::Twist& twist, const latticetypes::LatticeMatrix& d,
	       const rootdata::RootDatum& rd)

/*
  Synopsis: puts in twist the twist defined by d.

  Precondition: d holds an involution of the root datum which fixes the
  positive Weyl chamber.
*/

{
  using namespace latticetypes;
  using namespace rootdata;

  size_t n = rd.semisimpleRank();
  
  WeightList sr(n);
  copy(rd.beginSimpleRoot(),rd.endSimpleRoot(),sr.begin());

  for (size_t j = 0; j < n; ++j) {
    Weight v(rd.rank());
    d.apply(v,rd.simpleRoot(j));
    twist[j] = std::find(sr.begin(),sr.end(),v) - sr.begin();
  }

  return;
}

}

}

