/*
  This is rootdata.cpp
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.4 

  See file main.cpp for full copyright notice
*/

#include "rootdata.h"

#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <set>

#include "dynkin.h"
#include "lattice.h"
#include "partition.h"
#include "prerootdata.h"
#include "smithnormal.h"

/*****************************************************************************

  This module contains the implementation of the RootDatum class. What we
  call a root datum in this program is what is usually called a based root
  datum, and we also include the involution which defines the real form
  (written in Cartan form, i.e. the negative of the Galois involution.)

  The root datum defines the complex reductive group entirely, and, together
  with the involution, the full group of real points. The most delicate issue
  which we have to address is the determination of the component group of
  G(R); actually we describe its dual, which we denote dpi0, as a subgroup
  of the dpi0 of the torus. This is explained in detail in the notes.

  Another non-trivial issue is how to get a group interactively from the
  user. Actually this is perhaps a bit overstated here; in fact, when the
  program functions as a library, the interaction with the user will be
  relegated to some higher-up interface; in practice it is likely that
  the data will usually be read from a file. The main issue is to define
  the character lattice of the torus, and the component group of the group.

  In the interactive approach, we start from the abstract real lie type, and
  (implicitly) associate to it the direct product of the simply connected
  semisimple factor, and the torus as it is given. The user then has to
  define a sublattice of finite index, containing the root lattice, and
  stable under the involution. From this sublattice the actual root datum
  is constructed.

******************************************************************************/

namespace atlas {

namespace rootdata {

  using namespace latticetypes;

}

namespace {

  using namespace rootdata;

  void fillRoots(latticetypes::WeightList&, latticetypes::WeightList&, 
		 RootList&, const RootDatum&);
  void fillPositiveRoots(RootList&, const RootDatum&);

}

/*****************************************************************************

        Chapter I -- The RootDatum class

  The root datum class will be one of the central classes in the program.
  Except for the choice of connectivity, everything we plan to do should
  be ultimately constructed from the root datum.

  A based root datum is a quadruple (X,X^,D,D^), where X and X^ are lattices
  dual to each other, and D is a basis for a root system in X, D^ the dual
  basis of the dual root system (more precisely, D and D^ willl be bases
  for root systems in the sub-lattices they generate.)

  The fundamental lattice X~ that we will work in is the character lattice of
  a group G~ x T, where G~ is a simply connected semisimple group, and T
  a torus, both defined over R; our complex reductive group G will be a finite
  quotient of this. More precisely, let Q be the subgroup of our lattice
  generated by D; then the character lattice X of G may be any sublattice of
  full rank of our fundamental lattice containing Q.

  Notice that Q itself is not necessarily of full rank; it will be if and only
  if G is semisimple. We describe the quotient X~/Q as a product of cyclic
  groups (there will be one finite factor for each simple factor, except
  for type D in even rank, where there will be two factors Z/2, and one
  infinite factor for each torus component.) The user is then allowed to
  enter any cofinite subgroup of this quotient, in terms of generators, to
  be X/Q.

******************************************************************************/

namespace rootdata {

RootDatum::RootDatum(const prerootdata::PreRootDatum& prd)
  :d_rank(prd.rank()),
   d_roots(prd.roots()),
   d_coroots(prd.coroots()),
   d_twoRho(prd.rank(),0)

{  
  using namespace lattice;
  using namespace setutils;
  using namespace tags;

  d_semisimpleRank = d_roots.size();

  for (size_t j = 0; j < d_semisimpleRank; ++j)
    d_simpleRoots.push_back(j);

  // get basis of co-radical character lattice

  if (d_semisimpleRank < d_rank) {
    perp(d_coradicalBasis,d_coroots,d_rank);
    perp(d_radicalBasis,d_roots,d_rank);
  }

  // fill in the roots and coroots

  fillRoots(d_roots,d_coroots,d_simpleRoots,*this);
  fillPositiveRoots(d_posRoots,*this);

  d_isPositive.resize(d_roots.size());
  for (size_t j = 0; j < d_posRoots.size(); ++j)
    d_isPositive.insert(d_posRoots[j]);

  d_isSimple.resize(d_roots.size());
  for (size_t j = 0; j < d_simpleRoots.size(); ++j)
    d_isSimple.insert(d_simpleRoots[j]);

  // fill in the weights and coweights

  LatticeMatrix tc(d_semisimpleRank); // Cartan matrix

  for (size_t j = 0; j < d_semisimpleRank; ++j)
    for (size_t i = 0; i < d_semisimpleRank; ++i)
      tc(j,i) = cartan(i,j);

  LatticeCoeff d;
  tc.invert(d);

  // the simple weights are given by the matrix Q.tC^{-1}, where Q is the
  // matrix of the simple roots, tC the transpose Cartan matrix
  LatticeMatrix q(beginSimpleRoot(),endSimpleRoot(),IteratorTag());
  q *= tc;

  for (size_t j = 0; j < d_semisimpleRank; ++j) {
    Weight v;
    q.column(v,j);
    d_weights.push_back(RatWeight(v,d));
  }

  // do the same for coweights
    tc.transpose();
  LatticeMatrix qc(beginSimpleCoroot(),endSimpleCoroot(),IteratorTag());
  qc *= tc;

  for (size_t j = 0; j < d_semisimpleRank; ++j) {
    Weight v;
    qc.column(v,j);
    d_coweights.push_back(RatWeight(v,d));
  }

  // fill in the simple root permutations
  d_rootPermutation.resize(d_semisimpleRank);

  for (size_t j = 0; j < d_semisimpleRank; ++j) {
    Permutation& rp = d_rootPermutation[j];
    rp.resize(numRoots());
    for (size_t i = 0; i < numRoots(); ++i) {
      Weight v = d_roots[i];
      simpleReflection(v,j);
      rp[i] = rootNbr(v);
    }
  }

  // make the d_minus list

  for (size_t j = 0; j < d_roots.size(); ++j) {
    Root v(d_roots[j]);
    RootNbr jm = lower_bound(d_roots.begin(),d_roots.end(),-v)-d_roots.begin();
    d_minus.push_back(jm);
  }

  // fill in the sum of positive roots

  for (WRootIterator i = beginPosRoot(); i != endPosRoot(); ++i)
    d_twoRho += *i;

  // fill in the status

  fillStatus();
}

RootDatum::RootDatum(const RootDatum& rd, tags::DualTag)
  :d_rank(rd.d_rank),
   d_semisimpleRank(rd.d_semisimpleRank),
   d_coradicalBasis(rd.d_radicalBasis),
   d_radicalBasis(rd.d_coradicalBasis),
   d_roots(rd.d_coroots),
   d_coroots(rd.d_roots),
   d_minus(rd.d_minus),
   d_posRoots(rd.d_posRoots),
   d_simpleRoots(rd.d_simpleRoots),
   d_weights(rd.d_coweights),
   d_coweights(rd.d_weights),
   d_rootPermutation(d_semisimpleRank),
   d_isPositive(rd.d_isPositive),
   d_isSimple(rd.d_isSimple),
   d_twoRho(d_rank,0)


/*
  Synopsis: constructs the root system dual to the given one.

  Recall that we are actually realizing both the rootdatum and its dual in
  the _same_ lattice; so it is essentially an issue of interchangeing roots
  and coroots.
*/

{
  using namespace setutils;

  // fill in the sum of positive roots

  for (WRootIterator i = beginPosRoot(); i != endPosRoot(); ++i)
    d_twoRho += *i;

  // fill in the simple root permutations
  for (size_t j = 0; j < d_semisimpleRank; ++j) {
    Permutation& rp = d_rootPermutation[j];
    rp.resize(numRoots());
    for (size_t i = 0; i < numRoots(); ++i) {
      Weight v = d_roots[i];
      simpleReflection(v,j);
      rp[i] = rootNbr(v);
    }
  }

  for (size_t j = 0; j < d_semisimpleRank; ++j)
    assert(d_rootPermutation[j] == rd.d_rootPermutation[j]);
}

RootDatum::~RootDatum()

{}

/******** accessors **********************************************************/

WRootIterator RootDatum::beginPosCoroot() const

/*
  Makes a WRootIterator pointing to the beginning of the list of
  positive coroots.
*/

{
  return WRootIterator(d_coroots,d_posRoots.begin());
}

WRootIterator RootDatum::beginPosRoot() const

/*
  Makes a WRootIterator pointing to the beginning of the list of
  positive roots.
*/

{  
  return WRootIterator(d_roots,d_posRoots.begin());
}

WRootIterator RootDatum::beginSimpleCoroot() const

/*
  Makes a WRootIterator pointing to the beginning of the list of
  simple coroots.
*/

{
  return WRootIterator(d_coroots,d_simpleRoots.begin());
}

WRootIterator RootDatum::beginSimpleRoot() const

/*
  Makes a WRootIterator pointing to the beginning of the list of
  simple roots.
*/

{  
  return WRootIterator(d_roots,d_simpleRoots.begin());
}

void RootDatum::coreflection(Weight& v, RootNbr j) const

/*
  Applies to v the reflection about coroot number j. In other words, cr
  is transformed into v - <\alpha_j,v>\alpha\v_j
*/

{
  using namespace latticetypes;

  LatticeCoeff a = LT::scalarProduct(d_roots[j],v);
  Weight m = d_coroots[j];
  m *= a;
  v -= m;

  return;
}


WRootIterator RootDatum::endPosCoroot() const

/*
  Makes a WRootIterator pointing to the end of the list of positive 
  coroots.
*/

{
  return WRootIterator(d_coroots,d_posRoots.end());
}

WRootIterator RootDatum::endPosRoot() const

/*
  Makes a WRootIterator pointing to the end of the list of positive roots.
*/

{
  return WRootIterator(d_roots,d_posRoots.end());
}

WRootIterator RootDatum::endSimpleCoroot() const

/*
  Makes a WRootIterator pointing to the end of the list of simple 
  coroots.
*/

{
  return WRootIterator(d_coroots,d_simpleRoots.end());
}

WRootIterator RootDatum::endSimpleRoot() const

/*
  Makes a WRootIterator pointing to the end of the list of simple roots.
*/

{
  return WRootIterator(d_roots,d_simpleRoots.end());
}

bool RootDatum::isAdjoint() const

/*
  Synopsis: tells whether the rootdatum is the rootdatum of an adjoint group.

  NOTE: we define a reductive group to be adjoint, if its derived group is
  adjoint. Then the product of the derived group and the radical is
  _automatically_ direct!
*/

{
  return d_status[IsAdjoint];
}

bool RootDatum::isSimplyConnected() const

/*
  Synopsis: tells whether the rootdatum is the rootdatum of a simply connected
  group.

  NOTE: this is the dual condition to being adjoint: hence the derived group
  has to be simply connected, _and_ the product of the group with its radical
  has to be direct.
*/

{
  return d_status[IsSimplyConnected];
}

void RootDatum::reflection(Weight& v, RootNbr j) const

/*
  Applies to v the reflection about root number j. In other words, r is 
  transformed into v - <v,\alpha\v_j>\alpha_j
*/

{
  LatticeCoeff a = LT::scalarProduct(v,d_coroots[j]);
  Weight m = d_roots[j];
  m *= a;
  v -= m;

  return;
}

void RootDatum::rootReflection(LatticeMatrix& q, RootNbr j) const

/*
  Synopsis : puts in q the reflection for root #j.

  NOTE: this is not intended for heavy use. If that is envisioned, it would be
  better to construct the matrices once and for all and return const 
  references.
*/

{
  q.resize(d_rank,d_rank);

  const Root& r = d_roots[j];
  const Root& cr = d_coroots[j];

  for (size_t j = 0; j < d_rank; ++j)
    for (size_t i = 0; i < d_rank; ++i)
      q(i,j) = -r[i]*cr[j];

  for (size_t i = 0; i < d_rank; ++i)
    q(i,i) += 1;

  return;
}

/******** manipulators *******************************************************/

void RootDatum::swap(RootDatum& other)

{
  std::swap(d_rank,other.d_rank);
  std::swap(d_semisimpleRank,other.d_semisimpleRank);
  d_coradicalBasis.swap(other.d_coradicalBasis);
  d_radicalBasis.swap(other.d_radicalBasis);
  d_roots.swap(other.d_roots);
  d_coroots.swap(other.d_coroots);
  d_minus.swap(other.d_minus);
  d_posRoots.swap(other.d_posRoots);
  d_simpleRoots.swap(other.d_simpleRoots);
  d_weights.swap(other.d_weights);
  d_coweights.swap(other.d_coweights);
  d_rootPermutation.swap(other.d_rootPermutation),
  d_isPositive.swap(other.d_isPositive);
  d_isSimple.swap(other.d_isSimple);
  d_twoRho.swap(other.d_twoRho);

  return;
}

/******** private member functions ******************************************/

void RootDatum::fillStatus()

/*
  Synopsis: fills in the status of the rootdatum.
*/

{
  using namespace latticetypes;
  using namespace matrix;
  using namespace smithnormal;
  using namespace tags;

  LatticeMatrix q(beginSimpleRoot(),endSimpleRoot(),IteratorTag());

  WeightList b;
  CoeffList invf;

  initBasis(b,d_rank);
  smithNormal(invf,b.begin(),q);

  d_status.set(IsAdjoint);

  for (size_t j = 0; j < invf.size(); ++j)
    if (invf[j] != 1) {
      d_status.reset(IsAdjoint);
      break;
    }

  LatticeMatrix qd(beginSimpleCoroot(),endSimpleCoroot(),IteratorTag());

  initBasis(b,d_rank);
  invf.clear();
  smithNormal(invf,b.begin(),qd);

  d_status.set(IsSimplyConnected);

  for (size_t j = 0; j < invf.size(); ++j)
    if (invf[j] != 1) {
      d_status.reset(IsSimplyConnected);
      break;
    }

  return;
}

}

/*****************************************************************************

        Chapter II -- Functions declared in rootdata.h

  This section defines the following functions, which were declared in
  rootdata.h :

  ... list here when it is stable ...

******************************************************************************/

namespace rootdata {

void cartanMatrix(latticetypes::LatticeMatrix& c, const RootDatum& rd)

/*
  Synopsis: puts in c the Cartan matrix of the root datum
*/

{
  size_t r = rd.semisimpleRank();

  c.resize(r,r,0);

  for (size_t j = 0; j < r; ++j)
    for (size_t i = 0; i < r; ++i)
      c(i,j) = rd.cartan(i,j);

  return;
}

void cartanMatrix(latticetypes::LatticeMatrix& c, const RootList& rb,
		  const RootDatum& rd)

/*
  Synopsis: puts in c the Cartan matrix of the root basis rb.

  Precondition: rb contains a basis of a sub-root system of the root system
  of rd.
*/

{
  size_t r = rb.size();

  c.resize(r,r,0);

  for (size_t j = 0; j < r; ++j)
    for (size_t i = 0; i < r; ++i)
      c(i,j) = LT::scalarProduct(rd.root(rb[i]),rd.coroot(rb[j]));

  return;
}

void dualBasedInvolution(LT::LatticeMatrix& di, const LT::LatticeMatrix& i, 
			 const RootDatum& rd)

/*
  Synopsis: puts i^\vee in di.

  Precondition: i is an involution of rd as a _based_ root datum;

  Postcondition: di is an involution of the dual based root datum;

  Algorithm: di is w0^t(-i^t).
*/

{
  using namespace latticetypes;

  LatticeMatrix w0;
  longest(w0,rd);

  di = i;
  di *= w0;
  di.negate();
  di.transpose();

  return;
}

void lieType(lietype::LieType& lt, const RootList& rb, const RootDatum& rd)

/*
  Synopsis: puts in lt the Lie type of the root system spanned by rb.

  Precondition: rb contains a basis of a sub-rootsystem of the root system of 
  rd.
*/

{  
  using namespace dynkin;
  using namespace latticetypes;

  LatticeMatrix cm;
  cartanMatrix(cm,rb,rd);
  dynkin::lieType(lt,cm);

  return;
}

void longest(LT::LatticeMatrix& q, const RootDatum& rd)

/*
  Synopsis: puts in q the matrix of the action of w_0;

  Algorithm: w_0 is the element making -twoRho positive.

  NOTE: not intended for heavy use. Should be precomputed if that is the
  case.
*/

{
  using namespace latticetypes;
  using namespace weyl;

  Weight v = rd.twoRho();
  v *= -1;

  WeylWord ww;
  toPositive(ww,v,rd);

  identityMatrix(q,rd.rank());

  for (size_t j = 0; j < ww.size(); ++j) {
    LatticeMatrix r;
    rd.rootReflection(r,rd.simpleRootNbr(ww[j]));
    q *= r;
  }

  return;
}

void makeOrthogonal(RootList& orth, const RootList& rl, const RootList& rs,
		    const RootDatum& rd)

/*
  This function puts in orth the elements of rs which are orthogonal to all
  elements of rl.
*/

{
  orth.resize(0);

  for (unsigned long i = 0; i < rs.size(); ++i) {
    for (unsigned long j = 0; j < rl.size(); ++j)
      if (!rd.isOrthogonal(rs[i],rl[j])) // rs[i] is not orthogonal to rl
	goto not_orthogonal;
    orth.push_back(rs[i]);
  not_orthogonal:
    continue;
  }

  return;
}

void reflectionMatrix(LT::LatticeMatrix& qs, RootNbr n, const RootDatum& rd)

/*
  Synopsis: writes in qs the matrix of the reflection thru root #n.
*/

{
  Weight vc = rd.coroot(n);
  qs.resize(rd.rank(),rd.rank());

  for (size_t j = 0; j < rd.rank(); ++j) {
    LatticeCoeff a = vc[j];
    Weight v = rd.root(n);
    v *= a;
    for (size_t i = 0; i < rd.rank(); ++i)
      qs(i,j) = -v[i];
    qs(j,j) += 1;
  }

  return;
}

void rootBasis(RootList& rb, const RootList& rl, const RootDatum& rd)

/*
  Synopsis: puts in rb the canonical basis of rl.

  Precondition: rl holds the roots in a sub-root system of rd;

  Forwarded to the RootSet version.
*/

{
  using namespace bitmap;

  BitMap rs(rd.numRoots());
  rs.insert(rl.begin(),rl.end());
  rootBasis(rb,rs,rd);

  return;
}

void rootBasis(RootList& rb, const RootSet& rs, const RootDatum& rd)

/*
  Synopsis: puts in rb the canonical basis of rs.

  Precondition: rs holds the roots in a sub-root system of rd;

  Algorithm: compute the sum of the positive roots in rs; extract the
  positive roots whose scalar product with that sum is 2.
*/

{
  using namespace latticetypes;

  RootSet rsl = rs;
  rsl &= rd.posRootSet();

  // compute sum of positive roots

  Weight twoRho(rd.rank(),0);
  RootSet::iterator rsl_end = rsl.end();

  for (RootSet::iterator i = rsl.begin(); i != rsl_end; ++i)
    twoRho += rd.root(*i);

  rb.clear();

  for (RootSet::iterator i = rsl.begin(); i != rsl_end; ++i)
    if (rd.scalarProduct(twoRho,*i) == 2)
      rb.push_back(*i);

  return;
}

void rootPermutation(RootList& rl, const LT::LatticeMatrix& p, 
		     const RootDatum& rd)

/*
  Synopsis: puts in rl the permutation of the root induced by p.

  Precondition: p permutes the roots;
*/

{
  rl.resize(rd.numRoots());

  for (size_t j = 0; j < rd.numRoots(); ++j) {
    Root r(rd.rank());
    p.apply(r,rd.root(j));
    rl[j] = rd.rootNbr(r);
  }

  return;
}

void strongOrthogonalize(RootList& rl, const RootDatum& rd)

/*
  Synopsis: makes rl into a strongly orthogonal system.

  Precondition: rl is a set of pairwise orthogonal roots;

  Algorithm: for i,j in rl, the sum can be a root iff i,j span a subsystem
  of type B2, and are short there; we can then replace i and j by their sum 
  and difference. Note that this will not create new bad pairs, as bad pairs
  are made up of short roots.
*/

{
  for (size_t j = 0; j < rl.size(); ++j)
    for (size_t i = 0; i < j; ++i)
      if (sumIsRoot(rl[i],rl[j],rd)) {
	Weight v = rd.root(rl[i]);
	v += rd.root(rl[j]);
	Weight w = rd.root(rl[i]);
	w -= rd.root(rl[j]);
	rl[i] = rd.rootNbr(v);
	rl[j] = rd.rootNbr(w);
      }

  return;
}

bool sumIsRoot(const Weight& a, const Weight& b, const RootDatum& rd)

/*
  Synopsis: tells if a+b is a root.

  Precondition: a and b are roots;
*/

{
  Weight v = a;
  v += b;
  return rd.isRoot(v);
}

bool sumIsRoot(const RootNbr& a, const RootNbr& b, const RootDatum& rd)

/*
  The same, in terms of root numbers
*/

{
  return sumIsRoot(rd.root(a),rd.root(b),rd);
}

void toDistinguished(LatticeMatrix& q, const RootDatum& rd)

/*
  Synopsis : transforms q into w.q, where w is the unique element such that
  w.q fixes the positive Weyl chamber.

  Note that wq is then automatically an involution; w_0.w.q permutes the simple
  roots.
*/

{
  Weight v(rd.rank());
  q.apply(v,rd.twoRho());

  weyl::WeylWord ww;
  toPositive(ww,v,rd);

  LatticeMatrix p;
  toMatrix(p,ww,rd);

  p *= q;
  q.swap(p);

  return;
}

void toMatrix(LatticeMatrix& q, const weyl::WeylWord& ww, 
	      const RootDatum& rd)

/*
  Synopsis : writes in q the matrix represented by ww.

  NOTE: not intended for heavy use. If that were the case, it would be better
  to use the decomposition of ww into pieces, and multiply those together.
  However, that would be for the ComplexGroup to do, as it is it that has
  access to both the Weyl group and the root datum.
*/

{
  identityMatrix(q,rd.rank());

  for (size_t j = 0; j < ww.size(); ++j) {
    RootNbr rj = rd.simpleRootNbr(ww[j]);
    LatticeMatrix r;
    rd.rootReflection(r,rj);
    q *= r;
  }

  return;
}

void toMatrix(LatticeMatrix& q, const RootList& rl, const RootDatum& rd)

/*
  Synopsis : writes in q the matrix represented by rl (the product of the
  root reflections for the various roots in rl.)

  NOTE: the products are written in the same order as the appearance of
  the roots in rl. In practice this will be used for sequences of orthogonal
  roots, so that the order doesn't matter.
*/

{
  identityMatrix(q,rd.rank());

  for (size_t j = 0; j < rl.size(); ++j) {
    LatticeMatrix r;
    rd.rootReflection(r,rl[j]);
    q *= r;
  }

  return;
}

void toPositive(weyl::WeylWord& ww, const Weight& d_v, const RootDatum& rd)

/*
  Synopsis: puts in ww a list of simple reflections constituting a reduced
  expression of the unique shortest element w in the Weyl group such that
  w.v is in the positive chamber.

  Algorithm: the greedy algorithm -- if v is not positive, there is a
  simple coroot alpha^v such that <v,alpha^v> is < 0; then s_alpha.v takes
  v closer to the positive chamber.
*/

{
  Weight v = d_v;
  ww.clear();

  for (;;) {
    size_t j = 0;
    for (; j < rd.semisimpleRank(); ++j)
      if (LT::scalarProduct(v,rd.simpleCoroot(j)) < 0)
	goto add_reflection;
    goto end;
  add_reflection:
    ww.push_back(j);
    rd.simpleReflection(v,j);
  }

 end:

  // reverse ww
  std::reverse(ww.begin(),ww.end());

  return;
}

void toWeylWord(weyl::WeylWord& ww, RootNbr rn, const RootDatum& rd)

/*
  Synopsis: writes in ww the expression of the reflection about root #rn
  as a product of simple reflections.
*/

{
  Weight v = rd.twoRho();
  rd.reflection(v,rn);
  toPositive(ww,v,rd);

  return;
}

void toWeylWord(weyl::WeylWord& ww, const latticetypes::LatticeMatrix& q, 
		const RootDatum& rd)

/*
  Synopsis: writes in ww the expression of q as a product of simple 
  reflections.

  Precondition: q is expressible as such a product.
*/

{
  Weight v(rd.rank());
  q.apply(v,rd.twoRho());
  toPositive(ww,v,rd);

  return;
}

void twoRho(Weight& tr, const RootList& rl, const RootDatum& rd)

/*
  Synopsis: puts in tworho the sum of the positive roots in rl.

  Precondition: rl holds the roots in a sub-rootsystem of the root system of 
  rd;
*/

{
  tr.assign(rd.rank(),0);

  for (size_t j = 0; j < rl.size(); ++j)
    if (rd.isPosRoot(rl[j]))
      tr += rd.root(rl[j]);

  return;
}

void twoRho(Weight& tr, const RootSet& rs, const RootDatum& rd)

/*
  Synopsis: puts in tworho the sum of the positive roots in rs.

  Precondition: rs holds the roots in a sub-rootsystem of the root system of 
  rd;
*/

{
  tr.assign(rd.rank(),0);

  RootSet rsp = rs;
  rsp &= rd.posRootSet();

  RootSet::iterator rsp_end = rsp.end();

  for (RootSet::iterator i = rsp.begin(); i != rsp.end(); ++i)
    tr += rd.root(*i);

  return;
}

}

/*****************************************************************************

                Chapter III -- Auxiliary functions.

  ... explain here when it is stable ...

******************************************************************************/

namespace {

void fillPositiveRoots(RootList& pr, const RootDatum& rd)

/*
  Fills in the set of positive roots w.r.t. the basis in d_rootBasis.
*/

{
  using namespace lattice;
  using namespace tags;

  // express roots in terms of the root basis
  // we have to be careful! in order to be able to do the base change,
  // we should extend the root basis to a full basis of the lattice
  // by adding the coradical basis

  WeightList lb(rd.beginSimpleRoot(),rd.endSimpleRoot());
  copy(rd.beginCoradical(),rd.endCoradical(),back_inserter(lb));

  WeightList rl;

  baseChange(rd.beginRoot(),rd.endRoot(),back_inserter(rl),lb.begin(),
	     lb.end());

  // sort --- this will put all negative roots first

  sort(rl.begin(),rl.end());

  // the last half of rl contains the positive roots

  WeightList::iterator prl_begin = rl.begin()+(rl.size()/2);
  WeightList::iterator prl_end = rl.end();

  // revert to original basis and original ordering

  inverseBaseChange(prl_begin,prl_end,prl_begin,lb.begin(),lb.end());
  sort(prl_begin,prl_end);

  // extract addresses of positive roots

  unsigned long i = 0;

  for (unsigned long j = 0; j < rd.numRoots(); ++j) {
    if (rd.root(j) == prl_begin[i]) { // found location of positive root
      pr.push_back(j);
      ++i;
    }
  }

  return;
}

void fillRoots(WeightList& rl, WeightList& crl, RootList& srl,
	       const RootDatum& rd)

/*
  This function fills in the list rl of all the roots in the system, and the
  list crl of all the co-roots, from the datum of the root-basis rb and the 
  co-root basis crb. The idea is to start out from rb, and then saturate 
  through successive applications of simple reflections.

  The main problem is not to loose track of the root/co-root relationship.
  We want it to be so that the coroot of rl[j] is crl[j]. This is why we
  work with root/co-root _pairs_
*/

{
  typedef WRootIterator I;
  typedef std::pair<Weight,Weight> RP;

  std::map<Weight,Weight> roots;
  std::list<RP> new_roots;

  // initialize roots and new_roots with simple pairs

  I rb_end = rd.endSimpleRoot();

  for (I ri = rd.beginSimpleRoot(), cri = rd.beginSimpleCoroot(); 
       ri != rb_end; ++ri, ++cri) {
    roots.insert(make_pair(*ri,*cri));
    new_roots.push_back(make_pair(*ri,*cri));
  }

  // construct root list

  while (!new_roots.empty()) {
    RP rp = new_roots.front();
    for (size_t j = 0; j < rd.semisimpleRank(); ++j) {
      RP rp_new = rp;
      rd.simpleReflection(rp_new.first,j);
      rd.simpleCoreflection(rp_new.second,j);
      if (roots.insert(rp_new).second) // we found a new root
	new_roots.push_back(rp_new);
    }
    new_roots.pop_front();
  }

  // make a note of the current simple roots

  WeightList rs(rl);
  rl.clear();
  crl.clear();

  // write to rl and crl

  std::map<Weight,Weight>::iterator roots_end = roots.end();

  for (std::map<Weight,Weight>::iterator rpi = roots.begin(); 
       rpi != roots_end; ++rpi) {
    rl.push_back(rpi->first);
    crl.push_back(rpi->second);
  }

  // reset the positions of the simple roots
  // one might want to sort them; we don't do that for now

  for (unsigned long j = 0; j < rs.size(); ++j)
    srl[j] = lower_bound(rl.begin(),rl.end(),rs[j]) - rl.begin();

  return;
}

}

}
