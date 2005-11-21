/*
  This is prerootdata.cpp
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.4 

  See file main.cpp for full copyright notice
*/

#include "prerootdata.h"

#include <cassert>

#include "lietype.h"
#include "smithnormal.h"

/*****************************************************************************

  NOTE ... explain here when it's stable ...

  ... explain about choosing the lie type and the lattice, cf. 
  PreRootDatum(Interactive).

******************************************************************************/

namespace atlas {

namespace {

  using namespace latticetypes;
  using namespace lietype;
  using namespace prerootdata;

  void makeCorootBasis(WeightList&, const LatticeMatrix&, const WeightList&);
  void makeRootBasis(WeightList&, const LatticeMatrix&, const WeightList&);

}

/*****************************************************************************

        Chapter I -- The PreRootDatum class

  The PreRootDatum class is here just so that the ingredients for building
  a RootDatum can be gotten conveniently in various ways (for instance, 
  interactively). It contains the rank, and bases for the roots and the
  coroots, expressed in the current lattice.

******************************************************************************/

namespace prerootdata {

/******** constructors and destructors ***************************************/

PreRootDatum::PreRootDatum(const lietype::LieType& lt, 
			   const latticetypes::WeightList& b)
  :d_rank(lietype::rank(lt))

/*
  Constructs the PreRootDatum whose lattice has basis b, expressed in terms
  of the canonical weight lattice basis for lt (the simple weights for the
  semisimple part, the canonical basis for the torus part.)
*/

{  
  // get the Cartan matrix

  LatticeMatrix c;
  cartanMatrix(c,lt);

  // NOTE : the _tranpose_ of c is the matrix of the simple root vectors in
  // the basis of simple weight vectors. I got this wrong the first time!

  c.transpose();

  // write simple roots and coroots

  makeRootBasis(d_roots,c,b);
  makeCorootBasis(d_coroots,c,b);
}

/******** manipulators *******************************************************/

void PreRootDatum::swap(PreRootDatum& other)

{
  using namespace lietype;

  d_roots.swap(other.d_roots);
  d_coroots.swap(other.d_coroots);
  std::swap(d_rank,other.d_rank);
  
  return;
}

}

/*****************************************************************************

        Chapter II -- Functions declared in prerootdata.h

  --- explain here when it is stable ---

******************************************************************************/

namespace prerootdata {

void cartanMatrix(latticetypes::LatticeMatrix& cm, const lietype::LieType& lt)

/*
  Synopsis: puts in cm the Cartan matrix corresponding to the Lie type lt.

  Algorithm: the matrix is constructed blockwise, one block for each simple Lie
  type in lt.
*/

{
  using namespace latticetypes;
  using namespace lietype;

  size_t n = rank(lt);
  cm.resize(n,n,0);
  
  size_t r = 0;

  for (size_t j = 0; j < lt.size(); ++j) {
    LatticeMatrix cmp;
    cartanMatrix(cmp,lt[j]);
    cm.copy(cmp,r,r);
    r += rank(lt[j]);
  }

  return;
}

void cartanMatrix(latticetypes::LatticeMatrix& cm, 
		    const lietype::SimpleLieType& slt)

/*
  Synopsis: puts in cm the Cartan matrix corresponding to the simple Lie type 
  slt.

  Algorithm: case by case.
*/

{
  using namespace latticetypes;
  using namespace lietype;

  cm.resize(rank(slt),rank(slt),0);

  if (type(slt) == 'T') // do nothing
    goto done;

  for (size_t i = 0; i < rank(slt); ++i)
    cm(i,i) = 2;

  switch (type(slt)) {
  case 'A':
    for (size_t i = 1; i < rank(slt); ++i) {
      cm(i-1,i) = -1;
      cm(i,i-1) = -1;
    }
    goto done;
  case 'B': // rank is at least 2
    for (size_t i = 1; i < rank(slt)-1; ++i) {
      cm(i-1,i) = -1;
      cm(i,i-1) = -1;
    }
    cm(rank(slt)-2,rank(slt)-1) = -2;
    cm(rank(slt)-1,rank(slt)-2) = -1;
    goto done;
  case 'C': // rank is at least 2
    for (size_t i = 1; i < rank(slt)-1; ++i) {
      cm(i-1,i) = -1;
      cm(i,i-1) = -1;
    }
    cm(rank(slt)-2,rank(slt)-1) = -1;
    cm(rank(slt)-1,rank(slt)-2) = -2;
    goto done;
  case 'D': // rank is at least 4
    for (size_t i = 1; i < rank(slt)-1; ++i) {
      cm(i-1,i) = -1;
      cm(i,i-1) = -1;
    }
    cm(rank(slt)-3,rank(slt)-1) = -1;
    cm(rank(slt)-1,rank(slt)-3) = -1;
    goto done;
  case 'E':
    switch (rank(slt)) {
    case 6:
      cm(0,2) = -1;
      cm(2,0) = -1;
      cm(1,3) = -1;
      cm(3,1) = -1;
      cm(2,3) = -1;
      cm(3,2) = -1;
      cm(3,4) = -1;
      cm(4,3) = -1;
      cm(4,5) = -1;
      cm(5,4) = -1;
      goto done;
    case 7:
      cm(0,2) = -1;
      cm(2,0) = -1;
      cm(1,3) = -1;
      cm(3,1) = -1;
      cm(2,3) = -1;
      cm(3,2) = -1;
      cm(3,4) = -1;
      cm(4,3) = -1;
      cm(4,5) = -1;
      cm(5,4) = -1;
      cm(5,6) = -1;
      cm(6,5) = -1;
      goto done;
    case 8:
      cm(0,2) = -1;
      cm(2,0) = -1;
      cm(1,3) = -1;
      cm(3,1) = -1;
      cm(2,3) = -1;
      cm(3,2) = -1;
      cm(3,4) = -1;
      cm(4,3) = -1;
      cm(4,5) = -1;
      cm(5,4) = -1;
      cm(5,6) = -1;
      cm(6,5) = -1;
      cm(6,7) = -1;
      cm(7,6) = -1;
      goto done;
    }
  case 'F': // n is 4
    cm(0,1) = -1;
    cm(1,0) = -1;
    cm(1,2) = -2;
    cm(2,1) = -1;
    cm(2,3) = -1;
    cm(3,2) = -1;
    goto done;
  case 'f': // the dual of F; n is 4
    cm(0,1) = -1;
    cm(1,0) = -1;
    cm(1,2) = -1;
    cm(2,1) = -2;
    cm(2,3) = -1;
    cm(3,2) = -1;
    goto done;
  case 'G': // n is 2
    cm(0,1) = -1;
    cm(1,0) = -3;
    goto done;
  case 'g': // the dual of G; n is 2
    cm(0,1) = -3;
    cm(1,0) = -1;
    goto done;
  default: // this cannot happen
    assert(false && "unknown type letter in cartanMatrix");
    break;
  }

 done:
  ;
}

}

/*****************************************************************************

        Chapter III -- Auxiliary functions

  This sections contains the definitions of some ausiliary functions private
  to the present module :

    - void makeCorootBasis(WeightList&, const CartanMatrix&, 
      const WeightList&): writes down the simple coroots in the dual lattice
      basis;
    - void makeRootBasis(WeightList&, const CartanMatrix&, const WeightList&):
      writes down the simple roots in the lattice basis;

******************************************************************************/

namespace {

void makeCorootBasis(WeightList& crb, const LatticeMatrix& c, 
		     const WeightList& lb)

/*
  Given the lattice basis lb, expressed in tems of the simple weight basis,
  and the transposed Cartan matrix c, this function writes in crb the
  simple coroots of the system. In fact, if q is the matrix of lb in the
  simple weight basis, its transpose is the matrix of the dual basis of
  the simple weight basis, i.e. the simple coroot basis, in the dual
  lattice basis. So the only reason we need c at all is for the case where
  there are torus factors, to detect which columns in q correspond to these
  factors.
*/

{
  LatticeMatrix q(lb);
  q.transpose();

  Weight r(q.numRows());

  for (size_t j = 0; j < q.numColumns(); ++j) {
    bool nonZero = false;
    for (size_t i = 0; i < q.numRows(); ++i) {
      r[i] = q(i,j);
      if (c(i,j))
	nonZero = true;
    }
    if (nonZero)
      crb.push_back(r);
  }

  return;
}

void makeRootBasis(WeightList& rb, const LatticeMatrix& c, 
		   const WeightList& lb)

/*
  Given the lattice basis lb, expressed in terms of the simple weight basis,
  and the transposed Cartan matrix c, which may be interpreted as giving the
  coordinates of the simple roots in the simple weight basis, this function
  puts in rb the coordinates of the simple root basis in lb. So it is simply
  a base change. When there are torus factors, we should be careful to not
  push back the null columns from c that would correspond to these.
*/

{
  // multiply c by q^{-1} on the left

  LatticeMatrix q(lb);
  LatticeCoeff d;
  q.invert(d);
  q *= c;
  q /= d;

  // push back non-zero columns on rb

  for (size_t j = 0; j < q.numColumns(); ++j) {
    Weight r;
    q.column(r,j);
    if (!isZero(r))
      rb.push_back(r);
  }

  return;
}

}

}
