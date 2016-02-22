/*!
\file
\brief Implementation of the class RealReductiveGroup.
*/
/*
  This is realredgp.cpp
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Lie Groups and Representations

  For license information see the LICENSE file
*/

#include "realredgp.h"

#include "matreduc.h" // |adapted_basis|

#include "cartanclass.h"  // |Fiber|, and |toMostSplit| function (assertion)
#include "complexredgp.h" // various methods
#include "rootdata.h"     // |refl_prod| function (assertion)
#include "tori.h"         // |tori::RealTorus| used
#include "kgb.h"          // |KGB| constructed

#include <cassert>

namespace atlas {

/*****************************************************************************

        Chapter I -- The RealReductiveGroup class

******************************************************************************/

namespace realredgp {


/*!
  Synopsis : constructs a real reductive group from the datum of a complex
  reductive group and a real form.
*/
RealReductiveGroup::RealReductiveGroup
  (ComplexReductiveGroup& G_C, RealFormNbr rf)
  : d_complexGroup(G_C)
  , d_realForm(rf)
  , d_connectivity() // wait for most split torus to be constructed below

  , square_class_cocharacter(0) // set below
  , torus_part_x0(G_C.x0_torus_part(rf))

  , d_Tg(new // allocate private copy
	 TitsCoset(G_C,square_class_grading(G_C,square_class())))
  , kgb_ptr(NULL)
  , dual_kgb_ptr(NULL)
  , d_status()
{
  tori::RealTorus msT = G_C.cartan(G_C.mostSplit(rf)).fiber().torus();
  d_connectivity = topology::Connectivity(msT,G_C.rootDatum());

  { using namespace complexredgp;
    Grading gr =
      G_C.simple_roots_x0_compact(G_C.square_class_repr(G_C.xi_square(rf)));
    RatCoweight coch = complexredgp::coch_representative(rootDatum(),gr);
    // although already dependent only on square class, make it standard choice
    square_class_cocharacter = square_class_choice(G_C.distinguished(),coch);
  }

  d_status.set(IsConnected,d_connectivity.component_rank() == 0);
  d_status.set(IsCompact,msT.isCompact());

  d_status.set(IsQuasisplit,rf == G_C.quasisplit());
  d_status.set(IsSplit,msT.isSplit());
  d_status.set(IsSemisimple,G_C.rank() == G_C.semisimpleRank());

#ifndef NDEBUG
  // construct the torus for the most split Cartan
  const Fiber& fundf = G_C.fundamental();
  RootNbrSet so= cartanclass::toMostSplit(fundf,rf,G_C.rootSystem());

  // recompute matrix of most split Cartan
  const RootDatum& rd = G_C.rootDatum();
  tori::RealTorus T1
    (rootdata::refl_prod(so,rd) * G_C.distinguished()); // factors commute

  topology::Connectivity c(T1,rd);
  assert(d_connectivity.component_rank() == c.component_rank());
#endif
}

RealReductiveGroup::~RealReductiveGroup()
{ delete d_Tg; delete kgb_ptr; delete dual_kgb_ptr; }

/******** accessors *********************************************************/



/******** manipulators ******************************************************/


void RealReductiveGroup::swap(RealReductiveGroup& other)
{
  assert(&d_complexGroup==&other.d_complexGroup); // cannot swap references
  std::swap(d_realForm,other.d_realForm);
  d_connectivity.swap(other.d_connectivity);
  std::swap(d_Tg,other.d_Tg);
  std::swap(kgb_ptr,other.kgb_ptr);
  std::swap(dual_kgb_ptr,other.dual_kgb_ptr);
  std::swap(d_status,other.d_status);
}


const RootDatum& RealReductiveGroup::rootDatum() const
  { return d_complexGroup.rootDatum(); }

const TitsGroup& RealReductiveGroup::titsGroup() const
  { return d_Tg->titsGroup(); }

const WeylGroup& RealReductiveGroup::weylGroup() const
  { return d_complexGroup.weylGroup(); }

const TwistedWeylGroup& RealReductiveGroup::twistedWeylGroup() const
  { return d_complexGroup.twistedWeylGroup(); }

BitMap RealReductiveGroup::Cartan_set() const
  { return complexGroup().Cartan_set(d_realForm); }

// Returns Cartan \#cn (assumed to belong to cartanSet()) of the group.
const CartanClass& RealReductiveGroup::cartan(size_t cn) const
  { return d_complexGroup.cartan(cn); }

RatCoweight RealReductiveGroup::g() const
  { return g_rho_check()+rho_check(rootDatum()); }

size_t RealReductiveGroup::numCartan() const { return Cartan_set().size(); }

size_t RealReductiveGroup::rank() const { return rootDatum().rank(); };

size_t RealReductiveGroup::semisimpleRank() const
  { return rootDatum().semisimpleRank(); }

size_t RealReductiveGroup::numInvolutions()
  { return complexGroup().numInvolutions(Cartan_set()); }

size_t RealReductiveGroup::KGB_size() const
 { return d_complexGroup.KGB_size(d_realForm); }

size_t RealReductiveGroup::mostSplit() const
 { return d_complexGroup.mostSplit(d_realForm); }

/*
  Algorithm: the variable |rset| is first made to flag, among the imaginary
  roots of the fundamental Cartan, those that are noncompact for the chosen
  representative (in the adjoint fiber) of the real form of |G|. The result is
  formed by extracting only the information concerning the presence of the
  \emph{simple} roots in |rset|.
*/
Grading RealReductiveGroup::grading_offset()
{
  RootNbrSet rset= noncompactRoots(); // grading for real form rep
  return cartanclass::restrictGrading(rset,rootDatum().simpleRootList());
}

cartanclass::square_class RealReductiveGroup::square_class() const
  { return d_complexGroup.fundamental().central_square_class(d_realForm); }



const size_t RealReductiveGroup::component_rank() const
  { return d_connectivity.component_rank(); }
const SmallBitVectorList& RealReductiveGroup::dualComponentReps() const
  { return d_connectivity.dualComponentReps(); }

const WeightInvolution& RealReductiveGroup::distinguished() const
  { return d_complexGroup.distinguished(); }

RootNbrSet RealReductiveGroup::noncompactRoots() const
  { return d_complexGroup.noncompactRoots(d_realForm); }


// return stored KGB structure, after generating it if necessary
const KGB& RealReductiveGroup::kgb()
{
  if (kgb_ptr==NULL)
    kgb_ptr = new KGB(*this,Cartan_set(),false); // generate as non-dual
  return *kgb_ptr;
}

// return stored KGB structure, after generating it if necessary
const KGB& RealReductiveGroup::kgb_as_dual()
{
  if (dual_kgb_ptr==NULL)
    dual_kgb_ptr = new KGB(*this,Cartan_set(),true); // generate as dual
  return *dual_kgb_ptr;
}

// return stored Bruhat order of KGB, after generating it if necessary
const BruhatOrder& RealReductiveGroup::Bruhat_KGB()
{
  kgb(); // ensure |kgb_ptr!=NULL|, but we cannot use (|const|) result here
  return kgb_ptr->bruhatOrder(); // get Bruhat order (generate if necessary)
}


//			   function definitions


/*
  Given a real form cocharacter, find the one representing its square class.
  This basically chooses a representative for the action of the fundamental
  fiber group on strong involutions in the square class, and serves to define
  a base point, to which the bits for the initial $x_0$ will be an offset.

  This means reduce a class modulo $2X_*+\ker(1-\xi^t)$ to a class modulo
  $X_*+\ker(1-\xi^t)$. While naively it seems that projecting to $\xi^t$
  fixed coweights and then choosing a standard representative modulo $X_*$
  (by reducing each coordinate modulo 1) would work, this fails to do a
  complete reduction, as one can see in the example where $\xi$ acts on
  $\Q^2$ by swapping the coordinates: both $[0,0]$ and $[1,1]/2$ are
  $\xi^t$-fixed with reduced coordinates, but they are still equivalent.

  What is needed instead is to reduce modulo 1 for a complete set of
  $\xi$-stable coordinates integral on $X_*$, which can be obtained in the
  form of a basis of the sublattice $(X^*)^\xi$. Since we need $\xi+1$ here
  anyway for the projection at the end, we obtain that basis here from
  |adapted_basis| for the (smaller by a finite index) image $(\xi+1)(X^*)$.
*/
RatCoweight square_class_choice
  (const WeightInvolution& xi, const RatCoweight& coch)
{
  int_Matrix fix = xi+1; // projection to $\xi$-fixed weights
  CoeffList diagonal; // its values will be ignored, just the size counts
  int_Matrix B = matreduc::adapted_basis(fix,diagonal);
  // initial columns of |B| are basis of the saturarted sublattice $(X^*)^\xi$
  int_Matrix B_inv(B.inverse());
  const auto fix_rank=diagonal.size(), n=B.numRows();

  // taking preferred repr mod $X_*$ is basically just reduction modulo 1
  // however, do this only in the first |fix_rank| $\xi$-invariant coordinates
  RatCoweight tr =
    (coch*B.block(0,0,n,fix_rank)%=1)*B_inv.block(0,0,fix_rank,n);
  // now make this $\xi^t$-stable by projecting modulo $\ker(\xi^t-1)$
  return (tr*fix/=2).normalize();
}





} // |namespace realredgp|

} // |namespace atlas|
