//
// Created by Ryan Nicholl on 2/13/24.
//

#ifndef RPNX_QUXLANG_CONVERT_LLVM_OBJECT_HEADER
#define RPNX_QUXLANG_CONVERT_LLVM_OBJECT_HEADER


#include <llvm/Object/ObjectFile.h>
#include "quxlang/data/output_object_symbol.hpp"

namespace quxlang
{
   void convert_llvm_object(llvm::object::ObjectFile const & obj, std::function<void(object_symbol)> callback);
}
#endif //RPNX_QUXLANG_CONVERT_LLVM_OBJECT_HEADER
