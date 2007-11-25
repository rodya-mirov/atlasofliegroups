/*
  This is blockmode.cpp

  Copyright (C) 2007 Marc van Leeuwen
  part of the Atlas of Reductive Lie Groups

  For copyright and license information see the LICENSE file
*/

#include <iostream>
#include <fstream>

#include "blockmode.h"

#include "complexredgp.h"
#include "complexredgp_io.h"
#include "error.h"
#include "helpmode.h"
#include "interactive.h"
#include "io.h"
#include "ioutils.h"
#include "basic_io.h"
#include "filekl.h"
#include "realredgp.h"
#include "realredgp_io.h"
#include "cartanset.h"
#include "kgb.h"
#include "kgb_io.h"
#include "blocks.h"
#include "block_io.h"
#include "kl.h"
#include "kl_io.h"
#include "wgraph.h"
#include "wgraph_io.h"
#include "special.h"
#include "test.h"

/****************************************************************************

  This file contains the commands defined in the "block" mode of the program.
  This means that a real form and a dual real form have been chosen.

*****************************************************************************/

namespace atlas {

namespace {

  using namespace blockmode;

  void block_mode_entry() throw(commands::EntryError);
  void block_mode_exit();

  // functions for the predefined commands

  void type_f();
  void realform_f();
  void small_kgb_f();
  void small_dual_kgb_f();
template<bool small>
  void block_f();
template<bool small>
  void dual_block_f();
  void dual_map_f();
  void blockd_f();
  void blockorder_f();
  void blocku_f();
  void blockwrite_f();
  void blockstabilizer_f();
  void klbasis_f();
  void kllist_f();
  void primkl_f();
  void klwrite_f();
  void wgraph_f();
  void wcells_f();


  // local variables

  complexredgp::ComplexReductiveGroup* dual_G_C_pointer=NULL;
  realredgp::RealReductiveGroup* dual_G_R_pointer=NULL;
  blocks::Block* block_pointer=NULL;
  klsupport::KLSupport* kls_pointer=NULL;
  kl::KLContext* klc_pointer=NULL;
  wgraph::WGraph* WGr_pointer=NULL;
}

/*****************************************************************************

        Chapter I -- Functions declared in blockmode.h

******************************************************************************/

namespace blockmode {

// Returns a |CommandMode| object that is constructed on first call.
commands::CommandMode& blockMode()
{
  static commands::CommandMode block_mode
    ("block: ",block_mode_entry,block_mode_exit);
  if (block_mode.empty()) // true upon first call
  {
    // add the commands from the real mode
    commands::addCommands(block_mode,realmode::realMode());

    // add commands for this mode
    // the "type" command should be redefined here because it needs to exit
    // the block and real modes
    block_mode.add("type",type_f); // override
    block_mode.add("realform",realform_f); // this one too
    block_mode.add("smallkgb",small_kgb_f);
    block_mode.add("smalldualkgb",small_dual_kgb_f);
    block_mode.add("block",block_f<false>);
    block_mode.add("smallblock",block_f<true>);
    block_mode.add("dualblock",dual_block_f<false>);
    block_mode.add("smalldualblock",dual_block_f<true>);
    block_mode.add("dualmap",dual_map_f);
    block_mode.add("blockd",blockd_f);
    block_mode.add("blocku",blocku_f);
    block_mode.add("blockorder",blockorder_f);
    block_mode.add("blockwrite",blockwrite_f);
    block_mode.add("blockstabilizer",blockstabilizer_f);
    block_mode.add("klbasis",klbasis_f);
    block_mode.add("kllist",kllist_f);
    block_mode.add("primkl",primkl_f);
    block_mode.add("klwrite",klwrite_f);
    block_mode.add("wcells",wcells_f);
    block_mode.add("wgraph",wgraph_f);

    // add special commands
    special::addSpecialCommands(block_mode,BlockmodeTag());

    // add test commands
    test::addTestCommands(block_mode,BlockmodeTag());
  }
  return block_mode;
}

complexredgp::ComplexReductiveGroup& currentDualComplexGroup()
{
  return *dual_G_C_pointer;
}

realredgp::RealReductiveGroup& currentDualRealGroup()
{
  return *dual_G_R_pointer;
}

realform::RealForm currentDualRealForm()
{
  return dual_G_R_pointer->realForm();
}

blocks::Block& currentBlock()
{
  if (block_pointer==NULL)
  {
    block_pointer=new
      blocks::Block(mainmode::currentComplexGroup(),
		    realmode::currentRealGroup().realForm(),
		    dual_G_R_pointer->realForm());
  }
  return *block_pointer;
}

kl::KLContext& currentKL()
{
  if (klc_pointer==NULL)
  {
    kls_pointer=new klsupport::KLSupport(currentBlock());
    kls_pointer->fill();
    klc_pointer=new kl::KLContext(*kls_pointer);
    klc_pointer->fill();
  }
  return *klc_pointer;
}

wgraph::WGraph& currentWGraph()
{
  if (WGr_pointer==NULL)
  {
    const kl::KLContext& c=currentKL();
    WGr_pointer=new wgraph::WGraph(c.rank());
    kl::wGraph(*WGr_pointer,c);
  }
  return *WGr_pointer;
}

} // namespace blockmode

/****************************************************************************

        Chapter II -- The block mode |CommandMode|

  One instance of |CommandMode| for the block mode is created at the
  first call of |blockMode()|; further calls just return a reference to it.

*****************************************************************************/

namespace {

/*
  Synopsis: attempts to set a real form interactively. In case of failure,
  throws an InputError and returns.
*/
void block_mode_entry() throw(commands::EntryError)
{
  try
  {
    realredgp::RealReductiveGroup& G_R = realmode::currentRealGroup();
    G_R.fillCartan();

    complexredgp::ComplexReductiveGroup& G_C = G_R.complexGroup();
    const realredgp_io::Interface& G_RI = realmode::currentRealInterface();
    const complexredgp_io::Interface& G_I = G_RI.complexInterface();

    // get dual real form
    realform::RealForm drf;

    interactive::getInteractive
      (drf,G_I,G_C.dualRealFormLabels(G_R.mostSplit()),tags::DualTag());

    dual_G_C_pointer=new
      complexredgp::ComplexReductiveGroup(G_C,tags::DualTag());
    dual_G_R_pointer=new realredgp::RealReductiveGroup(*dual_G_C_pointer,drf);
    dual_G_R_pointer->fillCartan(); // this will be needed in all cases
  }
  catch(error::InputError& e)
  {
    block_mode_exit(); // clean up
    e("no dual real form set");
    throw commands::EntryError();
  }
}


/*
  Synopsis: destroys any local data, resoring NULL pointers
*/
void block_mode_exit()
{
  delete dual_G_C_pointer; dual_G_C_pointer=NULL;
  delete dual_G_R_pointer; dual_G_R_pointer=NULL;
  delete block_pointer; block_pointer=NULL;
  delete kls_pointer; kls_pointer=NULL;
  delete klc_pointer; klc_pointer=NULL;
  delete WGr_pointer; WGr_pointer=NULL;
}

} // namespace

/*****************************************************************************

        Chapter III --- Functions for the predefined commands

  This section contains the definitions of the functions associated to the
  various commands defined in this mode.

******************************************************************************/

namespace {

/*
  Synopsis: resets the type of the complex group.

  In case of success, the real forms are invalidated, and therefore we
  should exit real mode; in case of failure, we don't need to.
*/
void type_f()
{
  try {
    complexredgp::ComplexReductiveGroup* G;
    complexredgp_io::Interface* I;

    interactive::getInteractive(G,I);
    mainmode::replaceComplexGroup(G,I);
    commands::exitMode(); // upon success pop block mode, destroying dual group
    commands::exitMode(); // and pop real mode, destroying real group
  }
  catch (error::InputError& e) {
    e("complex group and real form not changed");
  }
}


/*
  Synopsis: resets the type, effectively re-entering the real mode. If the
  construction of the new type fails, the current block remains in force.
*/
void realform_f()
{
  try {
    realredgp::RealReductiveGroup& G_R=realmode::currentRealGroup();
    interactive::getInteractive(G_R,mainmode::currentComplexInterface());

    realredgp_io::Interface RI(G_R,mainmode::currentComplexInterface());
    realmode::currentRealInterface().swap(RI);

    commands::exitMode(); // upon success pop block mode, destroying dual group
  }
  catch (error::InputError& e) {
    e("real form not changed");
  }
  catch (error::InnerClassError& e) {
    e("that real form is not in the current inner class\n"
      "real form not changed");
  }
}

// Print the kgb table, only the necessary part for one block
void small_kgb_f()
{
  realredgp::RealReductiveGroup& G_R = realmode::currentRealGroup();
  realredgp::RealReductiveGroup& dGR = currentDualRealGroup();

  bitmap::BitMap common=blocks::common_Cartans(G_R,dGR);

  std::cout << "relevant Cartan classes: ";
  basic_io::seqPrint(std::cout,common.begin(),common.end(),",","{","}\n");

  std::cout
    << "partial kgb size: "
    << mainmode::currentComplexGroup().cartanClasses().KGB_size
        (realmode::currentRealForm(),common)
    << std::endl;

  ioutils::OutputFile file;
  kgb::KGB kgb(G_R,common);
  kgb_io::printKGB(file,kgb);
}

void small_dual_kgb_f()
{
  realredgp::RealReductiveGroup& G_R = realmode::currentRealGroup();
  realredgp::RealReductiveGroup& dGR = currentDualRealGroup();
  complexredgp::ComplexReductiveGroup& dGC = currentDualComplexGroup();

  bitmap::BitMap common=blocks::common_Cartans(dGR,G_R);

  std::cout << "relevant Cartan classes for dual group: ";
  basic_io::seqPrint(std::cout,common.begin(),common.end(),",","{","}\n");

  std::cout << "partial kgb size: " <<
    dGC.cartanClasses().KGB_size(currentDualRealForm(),common) << std::endl;
  ioutils::OutputFile file;

  kgb::KGB kgb(dGR,common);
  kgb_io::printKGB(file,kgb);
}

// Print the current block
template<bool small>
void block_f()
{
  ioutils::OutputFile file;
  if (small) // must unfortunatly regenerate the block here
  {
    block_io::printBlock
      (file,blocks::Block(mainmode::currentComplexGroup(),
			  realmode::currentRealForm(),
			  currentDualRealForm(),
			  true));
  }
  else
    block_io::printBlock(file,currentBlock());
}

// Print the dual block of the current block
template<bool small>
void dual_block_f()
{
  complexredgp::ComplexReductiveGroup& dG = currentDualComplexGroup();

  blocks::Block block(dG,currentDualRealForm(),realmode::currentRealForm(),
		      small);

  ioutils::OutputFile file;
  block_io::printBlock(file,block);
}

// Print the correspondence of the current block with its dual block
void dual_map_f()
{
  const blocks::Block& block = currentBlock();
  blocks::Block dual_block(currentDualComplexGroup(),
			   currentDualRealForm(),
			   realmode::currentRealForm());

  std::vector<blocks::BlockElt> v=blocks::dual_map(block,dual_block);

  std::ostringstream s("");
  basic_io::seqPrint(s,v.begin(),v.end(),", ","[","]\n");
  ioutils::OutputFile file;
  foldLine(file,s.str()," ");
}

// Print the current block with involutions in involution-reduced form
void blockd_f()
{
  ioutils::OutputFile file;
  block_io::printBlockD(file,currentBlock());
}

// Print the unitary elements of the block.
void blocku_f()
{
  ioutils::OutputFile file;
  block_io::printBlockU(file,currentBlock());
}


// Print the Hasse diagram for the Bruhat order on the current block
void blockorder_f()
{
  blocks::Block& block = currentBlock();
  std::cout << "block size: " << block.size() << std::endl;
  ioutils::OutputFile file;
  kgb_io::printBruhatOrder(file,block.bruhatOrder());
}

// Writes a binary file containing descent sets and ascent sets for block
void blockwrite_f()
{
  std::ofstream block_out; // binary output files
  if (interactive::open_binary_file
      (block_out,"File name for block output: "))
  {
    filekl::write_block_file(currentBlock(),block_out);
    std::cout<< "Binary file written.\n" << std::endl;
  }
  else
    std::cout << "No file written.\n";
}

/*
  Synopsis: prints out information about the stabilizer of a representation
  under the cross action
*/
void blockstabilizer_f()
{
  realredgp::RealReductiveGroup& G_R = realmode::currentRealGroup();
  realredgp::RealReductiveGroup& dGR = currentDualRealGroup();

  bitmap::BitMap common=blocks::common_Cartans(G_R,dGR);

  // get Cartan class; abort if unvalid
  size_t cn;
  interactive::getCartanClass(cn,common,commands::currentLine());

  ioutils::OutputFile file;
  realredgp_io::printBlockStabilizer
    (file,realmode::currentRealGroup(),cn,currentDualRealForm());
}

/* For each element $y$ in the block, outputs the list of non-zero K-L
   polynomials $P_{x,y}$.

   This is what is required to write down the K-L basis element $c_y$.
*/
void klbasis_f()
{
  kl::KLContext& klc = currentKL();

  ioutils::OutputFile file;
  file << "Full list of non-zero Kazhdan-Lusztig-Vogan polynomials:"
       << std::endl << std::endl;
  kl_io::printAllKL(file,klc);
}


// Print the list of all distinct Kazhdan-Lusztig-Vogan polynomials
void kllist_f()
{
  const kl::KLContext& klc = currentKL();

  ioutils::OutputFile file;
  kl_io::printKLList(file,klc);
}

/*
  Print out the list of all K-L polynomials for primitive pairs.

  Explanation: x is primitive w.r.t. y, if any descent for y is also a
  descent for x, or a type II imaginary ascent. Ths means that none of
  the easy recursion formulas applies to P_{x,y}.
*/

void primkl_f()
{
  kl::KLContext& klc = currentKL();

  ioutils::OutputFile file;
  file << "Kazhdan-Lusztig-Vogan polynomials for primitive pairs:"
       << std::endl << std::endl;
  kl_io::printPrimitiveKL(file,klc);
}

// Write the results of the KL computations to a pair of binary files
void klwrite_f()
{
  std::ofstream matrix_out, coefficient_out; // binary output files
  interactive::open_binary_file(matrix_out,"File name for matrix output: ");
  interactive::open_binary_file
    (coefficient_out,"File name for polynomial output: ");

  const kl::KLContext& klc = currentKL();

  if (matrix_out.is_open())
  {
    std::cout << "Writing matrix entries... " << std::flush;
    filekl::write_matrix_file(klc,matrix_out);
    std::cout << "Done." << std::endl;
  }
  if (coefficient_out.is_open())
  {
    std::cout << "Writing polynomial coefficients... " << std::flush;
    filekl::write_KL_store(klc.polStore(),coefficient_out);
    std::cout << "Done." << std::endl;
  }
}

// Print the W-graph corresponding to a block.
void wgraph_f()
{
  wgraph::WGraph& wg = currentWGraph();
  ioutils::OutputFile file; wgraph_io::printWGraph(file,wg);
}

// Print the cells of the W-graph of the block.
void wcells_f()
{
  wgraph::WGraph& wg = currentWGraph();
  wgraph::DecomposedWGraph dg(wg);

  ioutils::OutputFile file; wgraph_io::printWDecomposition(file,dg);
}




} // namespace




//      Chapter IV ---    H E L P    F U N C T I O N S


// Install help functions for block functions into help mode

namespace {

  const char* block_tag = "prints all the representations in a block";
  const char* dual_block_tag = "prints a block for the dual group";
  const char* dual_map_tag = "prints a map from block to its dual block";
  const char* blockd_tag =
   "prints all representations in the block, alternative format";
  const char* blocku_tag =
   "prints the unitary representations in the block at rho";
  const char* blockorder_tag =
   "shows Hasse diagram of the Bruhat order on the blocks";
  const char* blockwrite_tag = "writes the block information to disk";
  const char* blockstabilizer_tag = "print the real Weyl group for the block";
  const char* klbasis_tag = "prints the KL basis for the Hecke module";
  const char* kllist_tag = "prints the list of distinct KL polynomials";
  const char* klprim_tag = "prints the KL polynomials for primitive pairs";
  const char* klwrite_tag = "writes the KL polynomials to disk";
  const char* wgraph_tag = "prints the W-graph for the block";
  const char* wcells_tag = "prints the Kazhdan-Lusztig cells for the block";

void block_h()
{
  io::printFile(std::cerr,"block.help",io::MESSAGE_DIR);
}

void blockd_h()
{
  io::printFile(std::cerr,"blockd.help",io::MESSAGE_DIR);
}

void blocku_h()
{
  io::printFile(std::cerr,"blocku.help",io::MESSAGE_DIR);
}

void blockorder_h()
{
  io::printFile(std::cerr,"blockorder.help",io::MESSAGE_DIR);
}

void blockwrite_h()
{
  io::printFile(std::cerr,"blockwrite.help",io::MESSAGE_DIR);
}
void klbasis_h()
{
  io::printFile(std::cerr,"klbasis.help",io::MESSAGE_DIR);
}

void kllist_h()
{
  io::printFile(std::cerr,"kllist.help",io::MESSAGE_DIR);
}

void primkl_h()
{
  io::printFile(std::cerr,"primkl.help",io::MESSAGE_DIR);
}

void klwrite_h()
{
  io::printFile(std::cerr,"klwrite.help",io::MESSAGE_DIR);
}

void wcells_h()
{
  io::printFile(std::cerr,"wcells.help",io::MESSAGE_DIR);
}

void wgraph_h()
{
  io::printFile(std::cerr,"wgraph.help",io::MESSAGE_DIR);
}

} // namespace

namespace blockmode {

void addBlockHelp(commands::CommandMode& mode, commands::TagDict& tagDict)

{
  using commands::insertTag;

  //  mode.add("components",helpmode::nohelp_h);
  mode.add("block",block_h);
  mode.add("blockd",blockd_h);
  mode.add("blocku",blocku_h);
  mode.add("blockorder",blockorder_h);
  mode.add("blockwrite",blockwrite_h);
  mode.add("blockstabilizer",helpmode::nohelp_h);
  mode.add("klwrite",klwrite_h);
  mode.add("kllist",kllist_h);
  mode.add("primkl",primkl_h);
  mode.add("klbasis",klbasis_h);
  mode.add("wcells",wcells_h);
  mode.add("wgraph",wgraph_h);

  // insertTag(tagDict,"components",components_tag);
  insertTag(tagDict,"block",block_tag);
  insertTag(tagDict,"blockd",blockd_tag);
  insertTag(tagDict,"blocku",blocku_tag);
  insertTag(tagDict,"blockorder",blockorder_tag);
  insertTag(tagDict,"blockwrite",blockwrite_tag);
  insertTag(tagDict,"blockstabilizer",blockstabilizer_tag);
  insertTag(tagDict,"klbasis",klbasis_tag);
  insertTag(tagDict,"kllist",kllist_tag);
  insertTag(tagDict,"primkl",klprim_tag);
  insertTag(tagDict,"klwrite",klwrite_tag);
  insertTag(tagDict,"wcells",wcells_tag);
  insertTag(tagDict,"wgraph",wgraph_tag);
}

} // namespace blockmode

} // namespace atlas