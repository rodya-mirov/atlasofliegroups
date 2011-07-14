
#ifndef FILEKL_H
#define FILEKL_H


#include <iostream>
#include <fstream>

#include "blocks.h"
#include "kl.h"
#include "basic_io.h"

namespace atlas {
  namespace filekl {
    
    const blocks::BlockElt UndefBlock= ~blocks::BlockElt(0);
    const blocks::BlockElt noGoodAscent= UndefBlock-1;
    
    const unsigned int magic_code=0x06ABdCF0; 
    
    void write_block_file(const blocks::Block& block, std::ostream& out);
    
    void write_matrix_file(const kl::KLContext& klc, std::ostream& out);
    
    void write_KL_store(const kl::KLStore& store, std::ostream& out);

  }
}
#endif
