/*
  This is subdatum.cpp.

  Copyright (C) 2010 Marc van Leeuwen
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/

#include "subdatum.h"

#include "latticetypes.h"
#include "rootdata.h"
#include "tits.h"
#include "realredgp.h"
#include "kgb.h"

#include <cassert>

namespace atlas {

namespace subdatum {

SubSystem::SubSystem(const rootdata::RootDatum& parent,
		     const rootdata::RootList& sub_sys)
  : rootdata::RootSystem(parent.cartanMatrix(sub_sys).transposed()) // build
  , rd(parent) // share
  , sub_W(rootdata::RootSystem::cartanMatrix()) // use Cartan matrix above
  , pos_map(numPosRoots(),~0)
  , inv_map(rd.numRoots()+1,~0) // one spare entry for "unfound root in parent"
  , sub_root(sub_sys.size())
{
  for (weyl::Generator i=0; i<sub_sys.size(); ++i)
  {
    rootdata::RootNbr alpha = pos_map[i]=sub_sys[i];
    inv_map[alpha] = simpleRootNbr(i);
    inv_map[rd.rootMinus(alpha)] = numPosRoots()-1-i;

    size_t count=0; weyl::Generator s;
    while (alpha!=rd.simpleRootNbr(s=rd.find_descent(alpha)))
    {
      rd.simple_reflect_root(alpha,s);
      ++count;
    }

    sub_root[i].to_simple.resize(count);
    sub_root[i].reflection.resize(2*count+1);
    sub_root[i].simple=sub_root[i].reflection[count]=s; // set middle letter

    size_t j=count; // redo search loop, now storing values
    for (alpha=sub_sys[i]; j-->0; rd.simple_reflect_root(alpha,s))
    {
      s=rd.find_descent(alpha);
      sub_root[i].to_simple[j]=s;
      sub_root[i].reflection[count+1+j]=sub_root[i].reflection[count-1-j]=s;
    }
    assert(alpha==rd.simpleRootNbr(sub_root[i].simple));
  }

  for (unsigned int i=rank(); i<numPosRoots(); ++i)
  {
    rootdata::RootNbr alpha = posRootNbr(i); // root number in subsystem
    weyl::Generator s = find_descent(alpha);
    simple_reflect_root(alpha,s);
    rootdata::RootNbr beta = pos_map[posRootIndex(alpha)]; // in parent
    pos_map[i] = rd.permuted_root(sub_root[s].reflection,beta);
    inv_map[pos_map[i]] = posRootNbr(i);
    inv_map[rd.rootMinus(pos_map[i])] = numPosRoots()-1-i;
  }
}

SubSystem SubSystem::integral // pseudo contructor for integral system
  (const rootdata::RootDatum& parent, const latticetypes::RatWeight& gamma)
{
  latticetypes::LatticeCoeff n=gamma.denominator();
  const latticetypes::Weight& v=gamma.numerator();
  rootdata::RootSet int_roots(parent.numRoots());
  for (size_t i=0; i<parent.numPosRoots(); ++i)
    if (v.dot(parent.posCoroot(i))%n == 0)
      int_roots.insert(parent.posRootNbr(i));

  // it suffices that simple basis computed below live until end of constructor
  return SubSystem(parent,parent.simpleBasis(int_roots));
}

rootdata::RootNbr SubSystem::parent_nr(rootdata::RootNbr alpha) const
{
  return isPosRoot(alpha) ? pos_map[posRootIndex(alpha)]
    : parent_datum().rootMinus(pos_map[numPosRoots()-1-alpha]) ;
}

prerootdata::PreRootDatum SubSystem::pre_root_datum() const
{
  latticetypes::WeightList roots(rank()),coroots(rank()); // rank of subsystem
  for (weyl::Generator i=0; i<rank(); ++i)
  {
    roots[i]  =rd.root  (parent_nr_simple(i));
    coroots[i]=rd.coroot(parent_nr_simple(i));
  }
  return prerootdata::PreRootDatum(roots,coroots,rd.rank());
}

// compute twist and subsystem twisted involution $ww$ for $-theta^t$
// used in |tits::GlobalTitsGroup| constructor and in |iblock|, |test| cmds
weyl::Twist SubSystem::twist(const latticetypes::LatticeMatrix& theta,
			     weyl::WeylWord& ww) const
{
  rootdata::RootList Delta(rank()); // list of subsystem simple images by theta
  for (weyl::Generator i=0; i<rank(); ++i)
  {
    rootdata::RootNbr image =
      inv_map[rd.rootNbr(theta*rd.root(parent_nr_simple(i)))];
    assert(image < numRoots());  // |image| is number of image in subsystem
    Delta[i] = rootMinus(image); // |-theta| image of |root(i)|
  }

  weyl::WeylWord wrt = // this is ordered according to parent perspective
    rootdata::wrt_distinguished(*this,Delta); // make |Delta| distinguished

  // |Delta| now describes the |sub|-side fundamental involution, a twist

  weyl::Twist result; // the subsystem twist that |Delta| has be  reduced to
  for (weyl::Generator i=0; i<rank(); ++i)
    result[i] = simpleRootIndex(Delta[i]);

  // Let |theta_0| be the involution such that |Delta| describes $-theta_0^t$
  // then (for integrality systems) |theta_0| is parent quasi-split involution
  // and $theta=pw.theta_0$ where |pw| is |wrt| in terms of parent generators;
  // equivalently, $-theta^t=Delta.wrt^{-1}$ in terms of the subsystem

  // However, we want |ww| such that $-theta^t=ww.Delta$ (on subsystem side),
  // therefore |ww=twist(wrt^{-1})|. Nonetheless if |theta| is an involution,
  // it is the same to say $-theta^t=ww.Delta$ or $-theta^t=Delta.ww^{-1}$,
  // so following inversion and twist do nothing (|ww| is twisted involution).
  ww.resize(wrt.size()); // we must reverse and twist, to express on our side
  for (size_t i=0; i<wrt.size(); ++i)
    ww[wrt.size()-1-i] = result[wrt[i]];

  return result; // the result returned is the subsystem twist, not |ww|
}

// Here we seek twist and |ww| on parent side (dual with respect to |sub|)
// used in |tits::TitsGroup| constructor for subdatum, called from |SubDatum|
weyl::Twist SubSystem::parent_twist(const latticetypes::LatticeMatrix& theta,
				    weyl::WeylWord& ww) const
{
  // beginning is identical to |SubSystem::twist| above
  rootdata::RootList Delta(rank());
  for (weyl::Generator i=0; i<rank(); ++i)
  {
    rootdata::RootNbr image =
      inv_map[rd.rootNbr(theta*rd.root(parent_nr_simple(i)))];
    assert(image < numRoots());
    Delta[i] = image; // PLUS |theta| image of |root(i)|
  }

  // set |ww| to parent-side word, omitting inversion and twist done above
  ww = rootdata::wrt_distinguished(*this,Delta);
  // the above call has makes |Delta| sub-distinguished from parent side

  weyl::Twist result;
  for (weyl::Generator i=0; i<rank(); ++i)
    result[i] = simpleRootIndex(Delta[i]);

  return result;
}

latticetypes::LatticeMatrix SubSystem::action_matrix(const weyl::WeylWord& ww)
 const
{
  latticetypes::LatticeMatrix result(rd.rank());
  for (size_t i=0; i<ww.size(); ++i)
    result *= rd.root_reflection(parent_nr_simple(ww[i]));
  return result;
}

// grading of subsystem imaginary roots (parent perspective) induced by parent
gradings::Grading SubSystem::induced(gradings::Grading base_grading) const
{
  base_grading.complement(rd.semisimpleRank()); // flag compact simple roots
  gradings::Grading result;
  for (weyl::Generator s=0; s<rank(); ++s)
  { // count compact parent-simple roots oddly contributing to (subsystem) |s|
    gradings::Grading mask (rd.root_expr(parent_nr_simple(s))); // mod 2
    result.set(s,mask.dot(base_grading)); // mark if odd count (=>|s| compact)
  }

  result.complement(rank()); // back to convention marking non-compact roots
  return result;
}

  /*

        class |SubDatum|


  */

SubDatum::SubDatum(realredgp::RealReductiveGroup& GR,
		   const latticetypes::RatWeight& gamma,
		   kgb::KGBElt x)
  : SubSystem(SubSystem::integral(GR.rootDatum(),gamma))
  , base_ww()
  , delta(GR.complexGroup().involutionMatrix(GR.kgb().involution(x)))
  , Tg(static_cast<const SubSystem&>(*this),delta,base_ww)
  , ini_tw()
{ const rootdata::RootDatum& pd = parent_datum();

  // We'd like to |assert| here that |gamma| is valid, but it's hard to do.

  // we must still modify |delta| by |ww| going up to sub-distinguished invol
  for (size_t i=0; i<base_ww.size(); ++i) // apply in in reverse, to the right
    delta *= pd.root_reflection(parent_nr_simple(base_ww[i]));

  ini_tw = Tg.weylGroup().element(base_ww); // so that |ini_ww*delta=theta|

  // now store in |base_ww| the parent twisted involution for |delta|
  rootdata::RootList simple_image(pd.semisimpleRank());
  for (weyl::Generator s=0; s<simple_image.size(); ++s)
    simple_image[s] = pd.rootNbr(delta*pd.simpleRoot(s));

  // set the value that will be accessible later as |base_twisted_in_parent()|
  base_ww = rootdata::wrt_distinguished(pd,simple_image);
#ifndef NDEBUG
  const complexredgp::ComplexReductiveGroup& GC=GR.complexGroup();
  // We test that the subdatum shares its most split involution with |GC|
  latticetypes::LatticeMatrix M=delta;
  const weyl::WeylWord sub_w0 = Weyl_group().word(Weyl_group().longest());
  for (size_t i=0; i<sub_w0.size(); ++i)
    M *= pd.root_reflection(parent_nr_simple(sub_w0[i]));

  assert(M==GC.dualDistinguished().negative_transposed());
#endif
}

latticetypes::LatticeMatrix SubDatum::involution(weyl::TwistedInvolution tw)
  const
{
  latticetypes::LatticeMatrix theta = delta;
  weyl::WeylWord ww = Weyl_group().word(tw);
  for (size_t i=ww.size(); i-->0; )
    theta.leftMult(parent_datum().root_reflection(parent_nr_simple(ww[i])));
  return theta;
}

tits::GlobalTitsElement SubDatum::sub_base_point
  (const tits::GlobalTitsGroup& Tg, // dual global Tits group for parent
   const weyl::TwistedInvolution& tw) const
{
  const latticetypes::RatWeight delta_0(-Tg.torus_part_offset());
  const latticetypes::RatWeight delta_sub = // as embedded in |Tg|
    latticetypes::RatWeight(parent_sub_2rho(),4)+=delta_0;
  std::vector<tits::GlobalTitsElement> sub_sigma;
  sub_sigma.reserve(semisimple_rank());

  for (weyl::Generator s=0; s<semisimple_rank(); ++s)
  {
    tits::GlobalTitsElement lift(delta_0);
    sub_sigma.push_back(lift);
  }

  tits::GlobalTitsElement sbp(delta_sub);

  return sbp;
}

} // |namespace subdatum|

} // |namespace atlas|