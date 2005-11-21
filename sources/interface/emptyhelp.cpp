/*
  This is emptyhelp.cpp
  
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups version 0.2.4 

  See file main.cpp for full copyright notice
*/

#include "emptyhelp.h"

#include <iostream>

#include "commands.h"
#include "io.h"

/****************************************************************************

  This file contains the help mode for the "empty" command mode, which
  is the startup mode for the program.

*****************************************************************************/

namespace atlas {

namespace {

  // help commands
  void help_h();
  void qq_h();

  // command tags for the help facility
  const char* help_tag = "enters help mode";
  const char* qq_tag = "exits the program";

}

/*****************************************************************************

        Chapter I --- Functions for the help commands

  This section contains the definitions of the help functions associated to 
  the various commands defined in this mode.

******************************************************************************/

namespace {

void help_h()

{  
  io::printFile(std::cerr,"help.help",io::MESSAGE_DIR);
  return;
}

void qq_h()

{  
  io::printFile(std::cerr,"qq.help",io::MESSAGE_DIR);
  return;
}

}

/*****************************************************************************

        Chapter II --- Functions declared in emptyhelp.h

******************************************************************************/

namespace emptyhelp {

void addEmptyHelp(commands::CommandMode& mode, commands::TagDict& tagDict)

{  
  using namespace commands;

  mode.add("help",help_h);
  mode.add("qq",qq_h);

  TagDict::value_type help_entry = std::make_pair("help",help_tag);
  tagDict.insert(help_entry);

  TagDict::value_type qq_entry = std::make_pair("qq",qq_tag);
  tagDict.insert(qq_entry);

  return;
}

}

}
