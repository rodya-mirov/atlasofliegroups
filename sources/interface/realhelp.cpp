/*
  This is realhelp.cpp

  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  For copyright and license information see the LICENSE file
*/

#include "realhelp.h"

#include <iostream>

#include "io.h"
#include "helpmode.h"
#include "realmode.h"
#include "test.h"

/****************************************************************************

  This file contains the help mode for the "real" command mode.

*****************************************************************************/

namespace atlas {

namespace {

  // help commands
  void components_h();
  void cartan_h();
  void corder_h();
  void realweyl_h();
  void kgb_h();
  void KGB_h();
  void kgborder_h();

  // command tags for the help facility
  const char* components_tag = "describes component group of the real group";
  const char* cartan_tag = "prints the conjugacy classes of Cartan subgroups";
  const char* corder_tag = "shows Hasse diagram of ordering of Cartan classes";
  const char* realweyl_tag = "outputs the structure of the real Weyl group";
  const char* kgb_tag = "prints the orbits of K on G/B";
  const char* KGB_tag =
    "computes KGB data (more information than the kgb command)";
  const char* kgborder_tag = "prints the Bruhat ordering on K\\G/B";

}

/****************************************************************************

        Chapter I --- Functions declared in realhelp.h

*****************************************************************************/

namespace realhelp {

void addRealHelp(commands::CommandMode& mode, commands::TagDict& tagDict)

{
  using commands::insertTag;

  mode.add("components",components_h);
  mode.add("cartan",cartan_h);
  mode.add("corder",corder_h);
  mode.add("realweyl",realweyl_h);
  mode.add("kgb",kgb_h);
  mode.add("KGB",KGB_h);
  mode.add("kgborder",kgborder_h);

  insertTag(tagDict,"components",components_tag);
  insertTag(tagDict,"cartan",cartan_tag);
  insertTag(tagDict,"corder",corder_tag);
  insertTag(tagDict,"realweyl",realweyl_tag);
  insertTag(tagDict,"kgb",kgb_tag);
  insertTag(tagDict,"KGB",KGB_tag);
  insertTag(tagDict,"kgborder",kgborder_tag);
}

}

/*****************************************************************************

        Chapter II --- Functions for the help commands

  This section contains the definitions of the help functions associated to
  the various commands defined in this mode.

******************************************************************************/

namespace {

void components_h()
{
  io::printFile(std::cerr,"components.help",io::MESSAGE_DIR);
}

void cartan_h()
{
  io::printFile(std::cerr,"cartan.help",io::MESSAGE_DIR);
}

void corder_h()
{
  io::printFile(std::cerr,"corder.help",io::MESSAGE_DIR);
}

void realweyl_h()
{
  io::printFile(std::cerr,"realweyl.help",io::MESSAGE_DIR);
}

void kgb_h()
{
  io::printFile(std::cerr,"kgb.help",io::MESSAGE_DIR);
}

void KGB_h()
{
  io::printFile(std::cerr,"KGB_.help",io::MESSAGE_DIR);
}

void kgborder_h()
{
  io::printFile(std::cerr,"kgborder.help",io::MESSAGE_DIR);
}

} // namespace

} // namespace atlas
