/*!
\file
\brief Implementation of the class KLContext.

  This module contains code for the computation of the Kazhdan-Lusztig
  polynomials for a given block of representations. We have taken the radical
  approach of not using the Bruhat ordering at all, just ordering by length
  instead, and coping with the ensuing appearance of zero polynomials. It
  is expected that the simplification thus achieved will more than outweigh
  the additional polynomials computed.

  The general scheme is fairly similar to the one in Coxeter: there is a
  "KLSupport" structure, that holds the list of primitive pairs that makes it
  possible to read the d_kl list, plus some additional lists that allow for
  a fast primitivization algorithm, for instance; there are two main lists,
  d_kl (filled in for all primitive pairs), and d_mu (filled in only for
  non-zero mu coefficients.)
*/
/*
  This is kl.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#include "kl.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <string>
#include <algorithm> // for |std::lower_bound|

#include <cassert>
#include <set>  // for |down_set|
#include <stdexcept>

#include "hashtable.h"
#include "kl_error.h"
#include "wgraph.h"	// for the |wGraph| function


// extra defs for windows compilation -spc
#ifdef WIN32
#include <iterator>
#endif

/*
  [Fokko's original description, which referred to a slighly older
  version of the computation. Fokko implemented the change from using
  extremal pairs to the slightly large set of primitive pairs, but did
  not change all of the code comments. I am trying to do that now. DV
  8/26/06.]

  This module contains code for the computation of the Kazhdan-Lusztig
  polynomials for a given block of representations. We have taken the radical
  approach of not using the Bruhat ordering at all, just ordering by length
  instead, and coping with the ensuing appearance of zero polynomials. It
  is expected that the simplification thus achieved will more than outweigh
  the additional polynomials computed.

  The general scheme is fairly similar to the one in Coxeter: there is a
  "KLSupport" structure, that holds the list of extremal pairs that makes it
  possible to read the d_kl list, plus some additional lists that allow for
  a fast extremalization algorithm, for instance; there are two main lists,
  d_kl (filled in for all extremal pairs), and d_mu (filled in only for
  non-zero mu coefficients.)
*/

namespace atlas {

namespace kl {

  /*!
\brief Polynomial 0, which is stored as a vector of size 0.
  */
  const KLPol Zero;

  /*! \brief Polynomial 1.q^0. */
  const KLPol One(0,KLCoeff(1)); // Polynomial(d,1) gives 1.q^d.

// we wrap |KLPol| into a class |KLPolEntry| that can be used in a |HashTable|

/* This associates the type |KLStore| as underlying storage type to |KLPol|,
   and adds the methods |hashCode| (hash function) and |!=| (unequality), for
   use by the |HashTable| template.
 */
class KLPolEntry : public KLPol
{
public:
  // constructors
  KLPolEntry() : KLPol() {} // default constructor builds zero polynomial
  KLPolEntry(const KLPol& p) : KLPol(p) {} // lift polynomial to this class

  // members required for an Entry parameter to the HashTable template
  typedef KLStore Pooltype;		   // associated storage type
  size_t hashCode(size_t modulus) const; // hash function

  // compare polynomial with one from storage
  bool operator!=(Pooltype::const_reference e) const;

}; // |class KLPolEntry|


namespace helper {

  typedef hashtable::HashTable<KLPolEntry,KLIndex> KLHashStore;

  class Helper
  : public KLContext
  {
    // a table for KL polynomial lookup, which is absent in KLContext
    KLHashStore d_hashtable;
    bool d_verbose;

  // Member serving for statistics

    size_t prim_size;            // number of pairs stored in d_kl

  public:

    // constructors and destructors
    Helper(const KLContext&); // assumes an existing bare-bones |KLContext|

    ~Helper()
    {
      if(d_verbose)
      {
	std::cout << "\nNumber of matrix entries stored:     "
		  << prim_size << ".\n";
      }
    }

    //accessors

    weyl::Generator firstDirectRecursion(BlockElt y) const;

    weyl::Generator first_nice_and_real(BlockElt x,BlockElt y) const;

    std::pair<weyl::Generator,weyl::Generator>
      first_endgame_pair(BlockElt x, BlockElt y) const;

    BlockEltPair inverseCayley(size_t s, BlockElt y) const
    { return block().inverseCayley(s,y); }

    std::set<BlockElt> down_set(BlockElt y) const;

    // manipulators
    void rebuild_index() { d_hashtable.reconstruct(); }

    void fill(BlockElt y, bool verbose);

    void fillKLRow(BlockElt y);

    void recursionRow(std::vector<KLPol> & klv,
		      const PrimitiveRow& e, BlockElt y, size_t s);

    void muCorrection(std::vector<KLPol>& klv,
		      const PrimitiveRow& e,
		      BlockElt y, size_t s);

    void complete_primitives(const std::vector<KLPol>& klv,
				const PrimitiveRow& e, BlockElt y);

    void newRecursionRow(KLRow & klv,const PrimitiveRow& pr, BlockElt y);

    KLPol muNewFormula(BlockElt x, BlockElt y, size_t s, const MuRow& muy);


  }; // class Helper

} // namespace helper
} // namespace kl

/*****************************************************************************

        Chapter I -- Methods of the KLContext and KLPolEntry classes.

 *****************************************************************************/

/* methods of KLContext */


namespace kl {

KLContext::KLContext(const Block_base& b)
  : klsupport::KLSupport(b) // construct unfilled support object from block
  , fill_limit(0)
  , d_kl()
  , d_mu()
  , d_store(2)
{
  d_store[d_zero]=Zero; // ensure these polynomials are present
  d_store[d_one]=One;   // at expected indices, even if maybe absent in |d_kl|
}

/******** copy, assignment and swap ******************************************/


/*!
  \brief Copy constructor. Used only to copy-construct base for Helper class
*/
KLContext::KLContext(const KLContext& other)
  : klsupport::KLSupport(other)
  , fill_limit(other.fill_limit)
  , d_kl(other.d_kl)
  , d_mu(other.d_mu)
  , d_store(other.d_store)
{}



void KLContext::swap(KLContext& other) // used to extract base out of Helper
{
  klsupport::KLSupport::swap(other); // swap base (support) objects
  std::swap(fill_limit,other.fill_limit);

  d_kl.swap(other.d_kl);
  d_mu.swap(other.d_mu);

  d_store.swap(other.d_store);  // this puts the Helper store into base object
}

// If table extension fails, we need to recover the old part of the tables
void KLContext::partial_swap(KLContext& other)
{
  // this is only called both to copy old tables into the helper object |other|,
  // and in case of an overflow to recover those old tables form |other|
  klsupport::KLSupport::swap(other); // swap base (support) objects

  d_kl.swap(other.d_kl);
  d_mu.swap(other.d_mu);
  d_store.swap(other.d_store);

  if (other.fill_limit<fill_limit) // we are initially filling helper object
  {
    other.fill_limit=fill_limit; // for fill limit, copy to other but keep ours
    d_store.reserve(other.d_store.size()); // a stupid way to record old size
  }
  else // then we are restoring after a throw; |other| will soon disappear
  {
    d_kl.resize(fill_limit);
    d_mu.resize(fill_limit);
    d_store.resize(other.d_store.capacity()); // truncate to old size
  }

}

/******** accessors **********************************************************/


/*!
  \brief Returns the Kazhdan-Lusztig-Vogan polynomial P_{x,y}

  Precondition: x and y are smaller than size();

  Explanation: since d_store holds all polynomials for primitive
  pairs (x,y), this is basically a lookup function. While x is not
  primitive w.r.t. y, it moves x up, using the "easy" induction
  relations. At that point, we look x up in the primitive list for
  y. If it is found, we get a pointer to the result.  If it is not
  found, the polynomial is zero.
*/
KLPolRef KLContext::klPol(BlockElt x, BlockElt y) const
{
  const KLRow& klr = d_kl[y];
  unsigned int inx=prim_index(x,descentSet(y));

  if (inx>=klr.size()) // l(x)>=l(y), includes case x==~0: no primitivization
    return d_store[inx==self_index(y) ? d_one : d_zero];
  return d_store[d_kl[y][inx]];
}

KLIndex KLContext::KL_pol_index(BlockElt x, BlockElt y) const
{
  const KLRow& klr = d_kl[y];
  unsigned int inx=prim_index(x,descentSet(y));
  // if |inx>=klr.size()| then |l(primitive(x))>=l(y)|, maybe |primitive(x)==~0|
  return inx<klr.size() ? klr[inx] : inx==self_index(y) ? d_one : d_zero;
}

/*!
  \brief Returns mu(x,y).

  This function os not used internally. So we prefer do just use the table of
  KL polynomials.
 */
MuCoeff KLContext::mu(BlockElt x, BlockElt y) const
{
  unsigned int lx=length(x),ly=length(y);
  if (ly<=lx or (ly-lx)%2==0)
    return MuCoeff(0);
  KLPolRef p = klPol(x,y);
  return p.degree()<(ly-lx)/2 ? MuCoeff(0) : MuCoeff(p[p.degree()]);
}

/* The following two methods were moved here from the Helper class, since
   they turn out to be useful even when no longer constructing the KLContext
*/

/*
  Returns the list of all x extremal w.r.t. y.

  Explanation: this means that length(x) < length(y), and every descent
  for y is either a descent for x.  Or:  asc(x)\cap desc(y)=\emptyset
  Here descent means "in the tau invariant" (possibilities C-, ic, r1, r2).
*/
PrimitiveRow KLContext::extremalRow(BlockElt y)
  const
{
  BitMap b(size());
  b.fill(0,lengthLess(length(y)));  // start with all elements < y in length
  filter_extremal(b,descentSet(y)); // filter out those that are not extremal

  return PrimitiveRow(b.begin(),b.end()); // convert to list
}


/*
  Returns the list of all x primitive w.r.t. y.

  Explanation: this means that length(x) < length(y), and every descent
  for y is either a descent, or an imaginary type II ascent for x.
*/
PrimitiveRow KLContext::primitiveRow(BlockElt y) const
{
  BitMap b(size());
  b.fill(0,lengthLess(length(y)));   // start with all elements < y in length
  filter_primitive(b,descentSet(y)); // filter out those that are not primitive

  return PrimitiveRow(b.begin(),b.end());
}


/******** manipulators *******************************************************/

/*!
  \brief Fills (or extends) the KL- and mu-lists.

  Explanation: this is the main function in this module; all the work is
  deferred to the Helper class.
*/
void KLContext::fill(BlockElt y, bool verbose)
{

  if (y<fill_limit)
    return; // tables present already sufficiently large for |y|

#ifndef VERBOSE
  verbose=false; // if compiled for silence, force this variable
#endif

  if (verbose)
    std::cerr << "computing Kazhdan-Lusztig polynomials ..." << std::endl;

  helper::Helper help(*this); // make helper, with empty base KLContext
  if (fill_limit>0)
  {
    partial_swap(help);   // move previously computed tables into helper
    help.rebuild_index(); // since we just swapped the referred-to |KLStore|
  }

  try
  {
    help.fill(y,verbose); // takes care of filling embedded |KLSupport| too
    swap(help); // swap base object, including the |KLSupport| and |KLStore|

    fill_limit = y+1;

    if (verbose)
      std::cerr << "done" << std::endl;
  }
  catch (std::bad_alloc)
  { // roll back, and transform failed allocation into MemoryOverflow
    std::cerr << "\n memory full, KL computation abondoned." << std::endl;
    partial_swap(help); // restore (only) the previous contents from helper
    throw error::MemoryOverflow();
  }

}

BitMap KLContext::primMap (BlockElt y) const
{
  BitMap b(size()); // block-size bitmap

  // start with all elements < y in length
  b.fill(0,lengthLess(length(y)));
  b.insert(y);   // and y itself

  filter_primitive(b,descentSet(y)); // filter out those that are not primitive

  // now b holds a bitmap indicating primitive elements for y

  // our result will be a bitmap of that size
  BitMap result (b.size()); // initiallly all bits are cleared

  // traverse |b|, and for its elements that occur in, set bits in |result|

  size_t position=0; // position among set bits in b (avoids using b.position)
  for (BitMap::iterator it=b.begin(); it(); ++position,++it)
    if (not klPol(*it,y).isZero()) // look if |*it| indexes nonzero element
      result.insert(position);     // record its position and advance in row

  return result;
}


/* methods of KLPolEntry */


/*!
  \brief calculate a hash value in [0,modulus[, where modulus is a power of 2

  The function is in fact evaluation of the polynomial (with coefficients
  interpreted in Z) at the point 2^21+2^13+2^8+2^5+1=2105633, which can be
  calculated quickly (without multiplications) and which gives a good spread
  (which is not the case if 2105633 is replaced by a small number, because
  the evaluation values will not grow fast enough for low degree
  polynomials!).

*/
inline size_t KLPolEntry::hashCode(size_t modulus) const
{ const KLPol& P=*this;
  if (P.isZero()) return 0;
  polynomials::Degree i=P.degree();
  size_t h=P[i]; // start with leading coefficient
  while (i-->0) h= ((h<<21)+(h<<13)+(h<<8)+(h<<5)+h+P[i]) & (modulus-1);
  return h;
}

bool KLPolEntry::operator!=(KLPolEntry::Pooltype::const_reference e) const
{
  if (degree()!=e.degree()) return true;
  if (isZero()) return false; // since degrees match
  for (polynomials::Degree i=0; i<=degree(); ++i)
    if ((*this)[i]!=e[i]) return true;
  return false; // no difference found
}

} // namespace kl


/*****************************************************************************

        Chapter II -- Methods of the Helper class.

 *****************************************************************************/

namespace kl {
  namespace helper {

    // main constructor

Helper::Helper(const KLContext& kl)
  : KLContext(kl.block())  // re-construct empty KLContext base object
  , d_hashtable(d_store) // hash table identifies helper base object's d_store
  , d_verbose(false)
  , prim_size(0)
{
}

/******** accessors **********************************************************/


/*!
  \brief Returns the first descent generator that is not real type II

  Explanation: those are the ones that give a direct recursion formula for the
  K-L basis element. Explicitly, we search for a generator |s| such that
  |descentValue(s,y)| is either |DescentStatus::ComplexDescent| or
  |DescentStatus::RealTypeI|. If no such generator exists, we return |rank()|.
*/
weyl::Generator Helper::firstDirectRecursion(BlockElt y) const
{
  const DescentStatus& d = descent(y);
  size_t s;
  for (s=0; s<rank(); ++s) {
    DescentStatus::Value v = d[s];
    if (DescentStatus::isDirectRecursion(v))
      break;
  }
  return s;

} // |Helper::firstDirectRecursion|

/*
  Returns the first real nonparity ascent for y that is a complex ascent, or
  imaginary type 2, or compact imaginary for x.

  Explanation: those are the ones that give a nice new recursion formula for
  the K-L polynomial

  If no such generator exists, we return |rank()|.
*/
weyl::Generator Helper::first_nice_and_real(BlockElt x,BlockElt y) const
{
  const DescentStatus& dx = descent(x);
  const DescentStatus& dy = descent(y);

  weyl::Generator s;
  for (s=0; s<rank(); ++s)
    if (dy[s]==DescentStatus::RealNonparity)
      { DescentStatus::Value vx = dx[s];
	if (vx==DescentStatus::ComplexAscent or
	    vx==DescentStatus::ImaginaryTypeII or
	    vx==DescentStatus::ImaginaryCompact)
	  break; // and return the current |s|
      }
  return s;

} // |Helper::first_nice_and_real|

/*
  Preconditions:
  * all descents for y are of type r2 (firstDirectRecursion has failed)
  * x is extremal for y (none of the descents for y are ascents for x)
  * none of the rn ascents for y is C+, ic or i2 for x (so
    |first_nice_and_real| failed to find anything)

  Returns the first pair (s,t) such that
  1) (s,t) is (rn,r2) for y;
  2) (s,t) is (i1,ic) for x;
  3) (s,t) is (i1,i1/2) for s.x.
  Since the statuses of t for x and s.x differ, s must be ajdacent to t in the
  Dynkin diagram (but this is not tested or used explicitly here)

  Such a pair can be used to compute P_{x,y}+P_{sx,y} using s,
  then P_{sx,y} using t, and so P_{x,y}.

  The test that t is ic for x is omitted, since x and s.x being related by an
  imaginary cross action are in the same fiber, so t is imaginary for x if it
  is so for s.x, and noncompact for x is ruled out by extremality precondition

  If no such pair exists or (rank,*) is returned. Under the given
  preconditions, such failure allows concluding that P_{x,y}=0.
*/
std::pair<weyl::Generator,weyl::Generator>
   Helper::first_endgame_pair(BlockElt x, BlockElt y) const
{
  const DescentStatus& dx = descent(x);
  const DescentStatus& dy = descent(y);

  weyl::Generator s,t, r=rank();

  for (s=0; s<r; ++s)
    if (dy[s]==DescentStatus::RealNonparity and
	dx[s]==DescentStatus::ImaginaryTypeI)
    { BlockElt sx = cross(s,x);
      const DescentStatus& dsx = descent(sx);
      for (t = 0; t<r; ++t)
	if (dy[t]==DescentStatus::RealTypeII)
	  if (dsx[t]==DescentStatus::ImaginaryTypeI or
	      dsx[t]==DescentStatus::ImaginaryTypeII)
	    return std::make_pair(s,t);
    }
  return std::make_pair(r,0); // failure

} // |Helper::first_endgame_pair|

// compute the down-set of $y$, the non-extremal $x$ with $\mu(x,y)\neq0$
std::set<BlockElt> Helper::down_set(BlockElt y) const
{
  std::set<BlockElt> result;

  for (RankFlags::iterator it=descentSet(y).begin(); it(); ++it)
    switch (descentValue(*it,y))
    {
    case DescentStatus::ComplexDescent: result.insert(cross(*it,y));
      break;
    case DescentStatus::RealTypeI:
      {
	BlockEltPair sy = inverseCayley(*it,y);
	result.insert(sy.first); result.insert(sy.second);
      }
      break;
    case DescentStatus::RealTypeII:
      result.insert(inverseCayley(*it,y).first);
      break;
    default: // |case DescentStatus::ImaginaryCompact| nothing
      break;
    }
  return result;

} // |Helper::down_set|





/******** manipulators *******************************************************/


/*!
  \brief Dispatches the work of filling the KL- and mu-lists.

  The current implementation blindly assumes that the tables are empty
  initially. While this is true in the way it is now used, it should really be
  rewritten to take into account the possibility of initial |fill_limit>0|
*/
void Helper::fill(BlockElt last_y, bool verbose)
{
  // make sure the support (base of base) is filled
  klsupport::KLSupport::fill();

  // resize the outer lists to the block size
  d_kl.resize(size());
  d_mu.resize(size());

  // fill the lists
  size_t minLength = length(0);
  size_t maxLength = length(last_y<size() ? last_y : size()-1);

  //set timers for KL computation
  d_verbose=verbose; // inform our destructor of |verbose| setting
  std::time_t time0;
  std::time(&time0);
  std::time_t time;

  std::ifstream statm("/proc/self/statm"); // open file for memory status

  // do the other cases
  for (size_t l=minLength; l<=maxLength; ++l)
  {
    BlockElt y_limit = l<maxLength ? lengthLess(l+1) : last_y+1;
    for (BlockElt y=lengthLess(l); y<y_limit; ++y)
    {
      if (verbose)
	std::cerr << y << "\r";

      try
      {
	fillKLRow(y);
      }
      catch (kl_error::KLError& e)
      {
	std::ostringstream os;
	os << "negative coefficient in P_{" << e.x << ',' << e.y
	   << "} at line " << e.line << '.';
	throw std::runtime_error(os.str()); // so that realex may catch it
      }
    }

    // now length |l| is completed
    if (verbose)
    {
      size_t p_capacity // currently used memory for polynomials storage
	=d_hashtable.capacity()*sizeof(KLIndex)
	+ d_store.capacity()*sizeof(KLPol);
      for (size_t i=0; i<d_store.size(); ++i)
	p_capacity+= (d_store[i].degree()+1)*sizeof(KLCoeff);

      std::time(&time);
      double deltaTime = difftime(time, time0);
      std::cerr << "t="    << std::setw(5) << deltaTime
		<< "s. l=" << std::setw(3) << l // completed length
		<< ", y="  << std::setw(6)
		<< lengthLess(l+1)-1 // last y value done
		<< ", polys:"  << std::setw(11) << d_store.size()
		<< ", pmem:" << std::setw(11) << p_capacity
		<< ", mat:"  << std::setw(11) << prim_size
		<<  std::endl;

      unsigned int size, resident,share,text,lib,data;
      statm.seekg(0,std::ios_base::beg);
      statm >> size >> resident >> share >> text >> lib >> data;
      std::cerr << "Current data size " << data*4 << "kB (" << data*4096
		<< "), resident " << resident*4 << "kB, total "
		<< size*4 << "kB.\n";
    }

  } // for (l=min_length+1; l<=max_Length; ++l)

  if (verbose)
  {
    std::time(&time);
    double deltaTime = difftime(time, time0);
    std::cerr << std::endl;
    std::cerr << "Total elapsed time = " << deltaTime << "s." << std::endl;
    std::cerr << d_store.size() << " polynomials, "
	      << prim_size << " matrix entries."<< std::endl;

    std::cerr << std::endl;
  }

}


/*!
  \brief Fills in the row for y in the KL-table.

  Precondition: all lower rows have been filled; y is of length > 0;
  R-packets are consecutively numbered;

  Explanation: this function actually fills out the "row" of y (the set of
  all P_{x,y} for x<y, which is actually more like a column)
*/
void Helper::fillKLRow(BlockElt y)
{
  if (d_kl[y].size()>0)
    return; // row has already been filled
  weyl::Generator s = firstDirectRecursion(y);
  if (s<rank())  // a direct recursion was found, use it for |y|, for all |x|
  {
    std::vector<KLPol> klv;
    PrimitiveRow e = extremalRow(y); // we compute for |x| extremal only

    recursionRow(klv,e,y,s); // compute all polynomials for these |x|
    complete_primitives(klv,e,y); // add values at primitives; store in |d_kl|
  }
  else // we must use an approach that distinguishes on |x| values
  {
    KLRow& klv = d_kl[y]; // here we write directly into |d_kl| and
    PrimitiveRow pr=primitiveRow(y); // and use all |x| primitive for |y|
    // (any ascents for x that are descents for y must be imaginary type II)

    newRecursionRow(klv,pr,y); // put result of recursion formula in klv
  }
}

/*!
  \brief Puts into klv the right-hand side of the recursion formula for y
  corresponding to the descent s.

  Precondition: s is either a complex, or a real type I descent for y.

  Explanation: the shape of the formula is:

    P_{x,y} = (c_s.c_{y1})-part - correction term

  where y1 = cross(s,y) when s is complex for y, one of the two elements in
  inverseCayley(s,y) when s is real. The (c_s.c_{y1})-part depends on the
  status of x w.r.t. s (we look only at extremal x, so we know it is a
  descent). The correction term, coming from $\sum_z mu(z,y1)c_z$, is handled
  by |muCorrection|; the form of the summation depends only on |y1| (which it
  recomputes), but involves polynomials $P_{x,z}$ that depend on $x$ as well.
*/
void Helper::recursionRow(std::vector<KLPol>& klv,
			  const PrimitiveRow& e,
			  BlockElt y,
			  size_t s)
{
  klv.resize(e.size()+1);

  BlockElt sy =
    descentValue(s,y) == DescentStatus::ComplexDescent ? cross(s,y)
    : inverseCayley(s,y).first;  // s is real type I for y here, ignore .second

  size_t i; // keep outside for error reporting
  try {

    // last K-L polynomial is 1
    klv.back() = One;

    // the following loop could be run in either direction: no dependency.
    // however it is natural to take |x| descending from |y| (exclusive)
    for (i=e.size(); i-->0; )
    {
      BlockElt x = e[i];
      switch (descentValue(s,x))
      {
      case DescentStatus::ImaginaryCompact:
	{ // (q+1)P_{x,sy}
	  klv[i] = klPol(x,sy);
	  klv[i].safeAdd(klv[i],1);
	}
	break;
      case DescentStatus::ComplexDescent:
	{ // P_{sx,sy}+q.P_{x,sy}
	  BlockElt sx = cross(s,x);
	  klv[i] = klPol(sx,sy);
	  klv[i].safeAdd(klPol(x,sy),1);
	}
	break;
      case DescentStatus::RealTypeI:
	{ // P_{sx.first,sy}+P_{sx.second,sy}+(q-1)P_{x,sy}
	  BlockEltPair sx = inverseCayley(s,x);
	  klv[i] = klPol(sx.first,sy);
	  klv[i].safeAdd(klPol(sx.second,sy));
	  klv[i].safeAdd(klPol(x,sy),1);
	  klv[i].safeSubtract(klPol(x,sy));
	}
	break;
      case DescentStatus::RealTypeII:
	{ // P_{x_1,y_1}+qP_{x,sy}-P_{s.x,sy}
	  BlockElt sx = inverseCayley(s,x).first;
	  klv[i] = klPol(sx,sy);
	  klv[i].safeAdd(klPol(x,sy),1);
	  klv[i].safeSubtract(klPol(cross(s,x),sy));
	}
	break;
      default: assert(false); // this cannot happen
      }
    }
  }
  catch (error::NumericUnderflow& err){
    throw kl_error::KLError(e[i],y,__LINE__,
			    static_cast<const KLContext&>(*this));
  }

  muCorrection(klv,e,y,s); // subtract mu-correction from all of |klv|

} // |Helper::recursionRow|

/*!
  \brief Subtracts from all polynomials in |klv| the correcting terms in the
  K-L recursion.

  Precondtion: |klv| already contains, for all $x$ that are primitive w.r.t.
  |y| in increasing order, the terms in $P_{x,y}$ corresponding to
  $c_s.c_{y1}$, whery |y1| is $s.y$ if |s| is a complex descent, and |y1| is
  an inverse Cayley transform of |y| if |s| is real type I.
  The mu-table and KL-table have been filled in for elements of length < l(y).

  Explanation: the recursion formula is of the form:
  $$
    lhs = c_s.c_{y'} - \sum_{z} mu(z,y')c_z
  $$
  where |z| runs over the elements $< y'$ such that |s| is a descent for |z|.
  Here $lhs$ stands for $c_y$ when |s| is a complex descent or real type I for
  |y|, and for $c_{y}+c_{s.y}$ when |s| is real type II; however it plays no
  part in this function that only subtracts $\mu$-terms.

  We construct a loop over |z| first, before traversing |klv| (the test for
  $z<sy$ is absent, but $\mu(z,sy)\neq0$ implies $z<sy$ (by convention
  mu(y,y)=0, in any case no coefficient for |sy| is stored in |d_mu[sy]|, and
  moreover $z=sy$ would be rejected by the descent condition). The chosen loop
  order allows fetching $\mu(z,y1)$ only once, and terminating the scan of
  |klv| once its values |x| become too large to produce a non-zero $P_{x,z}$.

  Elements of length at least $l(sy)=l(y)-1$ on the list |e| are always
  rejected, so the tail of |e| never reached.
 */
void Helper::muCorrection(std::vector<KLPol>& klv,
			  const PrimitiveRow& e,
			  BlockElt y, size_t s)
{
  BlockElt sy =
    descentValue(s,y) == DescentStatus::ComplexDescent ? cross(s,y)
    : inverseCayley(s,y).first;  // s is real type I for y here, ignore .second

  const MuRow& mrow = d_mu[sy];
  size_t l_y = length(y);

  size_t j; // define outside for error reporting
  try {
    for (size_t i = 0; i<mrow.size(); ++i)
    {
      BlockElt z = mrow[i].first;
      size_t l_z = length(z);

      DescentStatus::Value v = descentValue(s,z);
      if (not DescentStatus::isDescent(v))
	continue;

      MuCoeff mu = mrow[i].second; // mu!=MuCoeff(0)

      polynomials::Degree d = (l_y-l_z)/2; // power of q used in the loops below

      if (mu==MuCoeff(1)) // avoid useless multiplication by 1 if possible
	for (j = 0; j < e.size(); ++j)
	{
	  BlockElt x = e[j];
	  if (length(x) > l_z) break; // once reached, no more terms for |z|

	  KLPolRef pol = klPol(x,z);
	  klv[j].safeSubtract(pol,d); // subtract q^d.P_{x,z} from klv[j]
	} // for (j)
      else // mu!=MuCoeff(1)
	for (j = 0; j < e.size(); ++j)
	{
	  BlockElt x = e[j];
	  if (length(x) > l_z) break; // once reached, no more terms for |z|

	  KLPolRef pol = klPol(x,z);
	  klv[j].safeSubtract(pol,d,mu); // subtract q^d.mu.P_{x,z} from klv[j]
	} // for {j)

    } // for (i)
  }
  catch (error::NumericUnderflow& err){
    throw kl_error::KLError(e[j],y,__LINE__,
			    static_cast<const KLContext&>(*this));
  }

} // |Helper::muCorrection|

/* A method that takes a row |klv| of completed KL polynomials, computed by
   |recursionRow| at |y| and extremal elements |x| listed in |er|, and
   transfers them to the main storage structures. Its tasks are

   - gernerated the list of all primitve elements for |y|, which contains |er|
   - for each primitive element |x|, if it is extremal just look up $P_{x,y}$
     from |klv| in |d_hashtable; if |x| is primitive but not extramal, compute
     that polynomial (as sum of two $P_{x',y}$ in the same row) and similarly
     store the result
   - record those |x| which have nonzero $\mu(x,y)$, and write |d_mu[y]|

   For the latter point there are two categories of |x|: the extremal ones
   (which can conveniently be handled in the loop over |x|), and those found
   by a (complex or real) descent from |y| itself (they have $\mu(x,y)=1$).
   The latter are of length one less than |y| (but there can be extremal |x|
   of that length as well with nonzero mu), and are primitive only in the real
   type 2 case; we must treat them outside the loop over primitive elements.
 */
void Helper::complete_primitives(const std::vector<KLPol>& klv,
				 const PrimitiveRow& er, BlockElt y)
{
  PrimitiveRow pr = primitiveRow(y);

  KLRow& KL = d_kl[y];
  KL.resize(pr.size());

  BitMap mu_elements(pr.size()); // flags |i| with |mu(pr[i],y)>0|

  unsigned int ly = length(y);
  if (ly==0)
    return; // nothing to do at minimal length

  size_t j= er.size()-1; // points to last extremal element (not |y|)

  for (size_t i = pr.size(); i-->0;)
    if (j<er.size() and pr[i]==er[j]) // |j| may underflow, whence first test
    { // extremal element $e=er[j]$
      unsigned int lx=length(er[j]);
      const KLPol& Pxy=klv[j--]; // use KL polynomial and advance downwards
      KL[i]=d_hashtable.match(Pxy);
      if ((ly-lx)%2>0 and Pxy.degree()==(ly-lx)/2) // in fact |(ly-lx-1)/2|
	mu_elements.insert(i);
    }
    else // must insert a polynomial for primitive non-extramal |pr[i]|
    {
      unsigned int s = ascent_descent(pr[i],y);
      BlockEltPair xs = cayley(s,pr[i]);
      assert(xs.second != blocks::UndefBlock); // must be imaginary type II
      KLPol pol = klPol(xs.first,y); // look up P_{x',y} in current row, above
      pol.safeAdd(klPol(xs.second,y)); // current point, and P_{x'',y} as well
      KL[i]=d_hashtable.match(pol); // add poly at primitive non-extremal x
    }


  std::set<BlockElt> downs = down_set(y);

  d_mu[y].reserve(mu_elements.size()+downs.size());
  for (BitMap::iterator it=mu_elements.begin(); it(); ++it)
  {
    BlockElt x=pr[*it];
    KLPolRef Pxy=d_hashtable[KL[*it]];
    assert(not Pxy.isZero());
    d_mu[y].push_back(std::make_pair(x,Pxy[Pxy.degree()]));
  }
  for (std::set<BlockElt>::iterator it=downs.begin(); it!=downs.end(); ++it)
  {
    d_mu[y].push_back(std::make_pair(*it,MuCoeff(1)));
    for (size_t i=d_mu[y].size()-1; i>0 and d_mu[y][i-1].first>*it; --i)
      std::swap(d_mu[y][i-1],d_mu[y][i]); // insertion-sort
  }

  prim_size+=KL.size(); // for statistics

} // |Helper::complete_primitives|


/*
  Puts in klv[i] the polynomial P_{e[i],y} for every primtitve x=pr[i],
  computed a recursion formula for those |y| that admit no direct recursion.

  Precondition: every simple root is for y either a complex ascent or
  imaginary or real (no complex descents for y). (split 1)

  In fact real type 1 descents for |y| don't occur, but this is not used.

  From the precondition we get: for each extremal |x| for |y|, there either
  exists a true ascent |s| that is real for |y|, necessarily nonparity because
  |x| is extremal, or we are assured that $P_{x,y}=0$.

  Here there is a recursion formula of a somewhat opposite nature than in the
  case of direct recursion. The terms involving $P_{x',y}$ where $x'$ are in
  the up-set of |x| appear in what is most naturally the left hand side of the
  equation, while the sum involving |mu| valeus appears on the right. As a
  consequence, the |mu| terms will be computed first, and then modifications
  involving such $P_{x',y}$ and subtraction are applied. However if |s| is
  type 'i1' for |x| the left hand side has (apart from $P_{x,y}$) another term
  $P_{s.x,y}$ for the imaginary cross image $s.x$, and so $P_{x,y}$ cannot be
  directly obtained in this manner; we shall avoid this case if we can.

  An additional case where |s| is 'ic' for |x| and 'rn' for |y| can be handled
  similarly, and we do so if the occasion presents itself. Here there are no
  terms from the up-set of |x| so the right hand side is just the mu terms,
  but the left hand side is $(q+1)P_{x,y}$ truncated to terms of degree at
  most $(l(y)-l(x)-1)/2$, from which we can recover $P_{x,y}$ by |safeDivide|.

  All in all the following cases are handled easily: |x| is (primitive but)
  not extremal, |x| has some |s| which is 'rn' for |y| and one of 'C+', 'i2'
  or 'ic' for |x| (|first_nice_and_real|). If no such |s| exists we can almost
  conclude $P_{x,y}=0$, but need to handle an exceptional "endgame" situation
  in which the formula for an 'i1' ascent can be exploited in spite of the
  presence of $P_{s.x,y}$, because that term can be computed on the fly.

  The sum involving mu, produced by |muNewFormula|, has terms involving
  $P_{x,z}\mu(z,y}$, so when doing a downward loop over |x| it pays to keep
  track of the |x| with nonzero $\mu(x,y)$.

  This code gets executed for |y| that are of minimal length, in which case
  it only contributes $P_{y,y}=1$; the |while| loop will be executed 0 times.
*/
void Helper::newRecursionRow(KLRow& klv,
			     const PrimitiveRow& pr,
			     BlockElt y)
{
  klv.resize(pr.size()); // primitive element of length less than |length(y)|

  unsigned int l_y = length(y);

  MuRow mu_y; mu_y.reserve(lengthLess(length(y))); // a very gross estimate
  std::set<BlockElt> downs = down_set(y);
  for (std::set<BlockElt>::iterator it=downs.begin(); it!=downs.end(); ++it)
    mu_y.push_back(std::make_pair(*it,MuCoeff(1)));

  size_t j = klv.size(); ; // declare outside try block for error reporting
  try {
    while (j-->0)
    {
      BlockElt x = pr[j];

      unsigned int s= ascent_descent(x,y);
      if (s<rank()) // a primitive element that is not extremal; easy case
      { // equation (1.9b) in recursion.pdf
	assert(descentValue(s,x)==DescentStatus::ImaginaryTypeII);
	BlockEltPair p = cayley(s,x);
	KLPol pol = klPol(p.first,y); // present since |klv| is |d_kl[y]|
	pol.safeAdd(klPol(p.second,y));
	klv[j] = d_hashtable.match(pol);
	continue; // done with |x|, go on to the next
      }

      unsigned int l_x = length(x);

      // now x is extremal for y. By (split 1) and Lemma 3.1 of recursion.pdf
      // this implies that if x<y in the Bruhat order, there is at least one s
      // real for y that is a true ascent (not rn) of x and therefore rn for y
      // we first hope that at least one of them is not i1 for x

      // we first seek a real nonparity ascent for y that is C+,i2 or ic for x
      s = first_nice_and_real(x,y);
      if (s < rank()) // there is such an ascent s
      {
	// start setting |pol| to the expression (3.4) in recursion.pdf
	KLPol pol = muNewFormula(x,y,s,mu_y);

	switch (descentValue(s,x))
	{
	case DescentStatus::ComplexAscent:
	{ // use equations (3.3a)=(3.4)
	  BlockElt sx = cross(s,x);
	  pol.safeSubtract(klPol(sx,y),1);
	  // subtract qP_{sx,y} from mu terms
	} // ComplexAscent case
	break;

	case DescentStatus::ImaginaryTypeII:
	{ // use equations (3.3a)=(3.5)
	  BlockEltPair p = cayley(s,x);
	  KLPol sum = klPol(p.first,y);
	  sum.safeAdd(klPol(p.second,y));
	  pol.safeAdd(sum);
	  pol.safeSubtract(sum,1); //now we've added (1-q)(P_{x',y}+P_{x'',y})
	  pol.safeDivide(2);   //this may throw
	} // ImaginaryTypeII case
	break;

	case DescentStatus::ImaginaryCompact:
	  /* here s is a emph{descent} for x, which causes an extra unknown
	     leading (if nonzero) term to appear in addition to (3.4), giving
	     rise to equation (3.7). Yet we can determine the quotient by q+1.
	  */
	  pol.safeQuotient(length(y)-length(x));
	  break;

	default: assert(false); //we've handled all possible NiceAscents
	}
	klv[j] = d_hashtable.match(pol);
	if ((l_y-l_x)%2!=0 and pol.degree()==(l_y-l_x)/2)
	  mu_y.push_back(std::make_pair(x,pol[pol.degree()]));

      } // end of |first_nice_and_real| case

      else
      {
	/* just setting klv[j]=Zero; won't do here, even in C2. We need to use
	   idea on p. 8 of recursion.pdf. This means: find s and t, both real
	   for y and imaginary for x, moreover repectively nonparity and
	   parity (r2) for y, repectively i1 and compact for x, while t is
	   noncompact for s.x (the imaginary cross of x), which implies t is
	   adjacent to s. Then we can compute P_{s.x,y} using t (an easy
	   recursion, (1.9) but for t, expresses it as sum of one or two
	   already computed polynomials), and for the sum P_{sx,y}+P_{x,y} we
	   have a formula (3.6) of the kind used for NiceAscent, and it
	   suffices to subtract P_{s.x,y} from it.

	   If no such s,t exist then we may conclude x is not Bruhat below y,
	   so P_{x,y}=0.
	*/
	std::pair<size_t,size_t> st = first_endgame_pair(x,y);
	if ((s=st.first) < rank())
	{
	  KLPol pol = muNewFormula(x,y,s,mu_y);

	  //subtract (q-1)P_{xprime,y} from terms of expression (3.4)
	  BlockElt xprime = cayley(s,x).first;
	  const KLPol& P_xprime_y =  klPol(xprime,y);
	  pol.safeAdd(P_xprime_y);
	  pol.safeSubtract(P_xprime_y,1);

	  //now klv[j] holds P_{x,y}+P_{s.x,y}
	  //compute P_{s.x,y} using t

	  BlockEltPair sx_up_t = cayley(st.second,cross(s,x));

	  pol.safeSubtract(klPol(sx_up_t.first,y));
	  if (sx_up_t.second != blocks::UndefBlock)
	    pol.safeSubtract(klPol(sx_up_t.second,y));

	  klv[j] = d_hashtable.match(pol);
	  if ((l_y-l_x)%2!=0 and pol.degree()==(l_y-l_x)/2)
	    mu_y.push_back(std::make_pair(x,pol[pol.degree()]));
	}
	else // |first_endgame_pair| found nothing
	  klv[j]=d_zero;
      } // end of no NiceAscent case
    } // while (j-->0)
  }
  catch (error::NumericUnderflow& err) // repackage error, reporting x,y
  {
    throw kl_error::KLError(pr[j],y,__LINE__,
			    static_cast<const KLContext&>(*this));
  }

  d_mu[y]=MuRow(mu_y.rbegin(),mu_y.rend()); // reverse to a tight copy

  for (unsigned int k=mu_y.size()-downs.size(); k<mu_y.size(); ++k)
  { // successively insertion-sort the down-set x's into previous list
    for (size_t i=k; i>0 and d_mu[y][i-1].first>d_mu[y][i].first; --i)
      std::swap(d_mu[y][i-1],d_mu[y][i]);
  }

} // |Helper::newRecursionRow|


/*!
  \brief Stores into |klv[j]| the $\mu$-sum appearing a new K-L recursion.

  Precondition: |pr| is the primitive row for |y|, $s$ is real nonparity for
  $y$ and either C+ or imaginary for $x=pr[j]$ (those are the cases for which
  the formula is used; the status w.r.t. $x$ is not actually used by the
  code), and for all $k>j$ one already has stored $P_{pr[k],y}$ in |klv[k]|.

  The mu-table and KL-table have been filled in for elements of length < l(y),
  so that for $z<y$ we can call |klPol(x,z)|.

  Explanation: the various recursion formulas involve a sum:
  $$
    \sum_{x<z<y} mu(z,y) q^{(l(y)-l(z)+1)/2}P_{x,z}
  $$
  where in addition to the condition given, |s| must be a descent for |z|.
  Since mu(z,y) cannot be nonzero unless z is primitive (indeed unless it is
  either extremal or primitive and of length l(y)-1, since in all other cases
  the recursion formulas show that $P_{z,y}$ cannot attain the maximal
  authorised degree $(l(y)-l(z)-1)/2$), so we can loop over elements of |pr|.

  We construct a loop over |z|.  The test for
  $z<y$ is absent, but $\mu(z,y)\neq0$ implies $z\leq y$. The chosen
  loop order allows fetching
  $\mu(z,y)$ only once, and terminating the scan of |klv| once its values |x|
  become too large to produce a non-zero $P_{x,z}$.

  We can't use d_mu(y), which hasn't yet been written, so mu(z,y) is extracted
  manually from the appropriate klv[k].
*/
KLPol Helper::muNewFormula(BlockElt x, BlockElt y, size_t s, const MuRow& mu_y)
{
  KLPol pol=Zero;

  size_t l_y = length(y);

  try
  {
    for (MuRow::const_iterator it=mu_y.begin(); it!=mu_y.end(); ++it)
    {
      BlockElt z = it->first;
      if (not DescentStatus::isDescent(descentValue(s,z))) continue;

      // now we have a true contribution with nonzero $\mu$
      size_t l_z = length(z);
      if (l_z<=length(x))
	break; // since length decreases in |mu_y| we can stop here

      unsigned int d = (l_y - l_z +1)/2; // power of q used in the formula
      MuCoeff mu = it->second;
      KLPolRef Pxz = klPol(x,z);

      if (mu==MuCoeff(1)) // avoid useless multiplication by 1 if possible
	pol.safeAdd(Pxz,d); // add q^d.P_{x,z} to pol
      else // mu!=MuCoeff(1)
	pol.safeAdd(Pxz,d,mu); // add q^d.mu.P_{x,z} to pol

    } // for (k)
  }
  catch (error::NumericOverflow& e){
    throw kl_error::KLError(x,y,__LINE__,
			    static_cast<const KLContext&>(*this));
  }

  return pol;
} //muNewFormula


} // namespace helper
} // namespace kl

/*****************************************************************************

        Chapter V -- Functions declared in kl.h

 *****************************************************************************/

namespace kl {


/*!
  \brief Puts in wg the W-graph for this block.

  Explanation: the W-graph is a graph with one vertex for each element of the
  block; the corresponding descent set is the tau-invariant, i.e. the set of
  generators s that are either complex descents, real type I or II, or
  imaginary compact. Let x < y in the block such that mu(x,y) != 0, and
  descent(x) != descent(y). Then there is an edge from x to y unless
  descent(x) is contained in descent(y), and an edge from y to x unless
  descent(y) is contained in descent(x). Note that the latter containment
  always holds when the length difference is > 1, so that in that case there
  will only be an edge from x to y (the edge must be there because we already
  assumed that the descent sets were not equal.) In both cases, the
  coefficient corresponding to the edge is mu(x,y).

  NOTE: if I'm not mistaken, the edgelists come already out sorted.
*/
void wGraph(wgraph::WGraph& wg, const KLContext& klc)
{
  wg.reset();
  wg.resize(klc.size());

  // fill in descent sets
  for (BlockElt y = 0; y < klc.size(); ++y)
    wg.descent(y) = klc.descentSet(y);

  // fill in edges and coefficients
  for (BlockElt y = 0; y < klc.size(); ++y) {
    const RankFlags& d_y = wg.descent(y);
    const MuRow& mrow = klc.muRow(y);
    for (size_t j = 0; j < mrow.size(); ++j) {
      BlockElt x = mrow[j].first;
      const RankFlags& d_x = wg.descent(x);
      if (d_x == d_y)
	continue;
      MuCoeff mu = mrow[j].second;
      if (klc.length(y) - klc.length(x) > 1) { // add edge from x to y
	wg.edgeList(x).push_back(y);
	wg.coeffList(x).push_back(mu);
	continue;
      }
      // if we get here, the length difference is 1
      if (not d_y.contains(d_x)) { // then add edge from x to y
	wg.edgeList(x).push_back(y);
	wg.coeffList(x).push_back(mu);
      }
      if (not d_x.contains(d_y)) { // then add edge from y to x
	wg.edgeList(y).push_back(x);
	wg.coeffList(y).push_back(mu);
      }
    }
  }

} // |wGraph|

} // namespace kl
} // namespace atlas
