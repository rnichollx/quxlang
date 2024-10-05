// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_CONVERT_LLVM_OBJECT_HEADER_GUARD
#define QUXLANG_MANIPULATORS_CONVERT_LLVM_OBJECT_HEADER_GUARD


#include <llvm/Object/ObjectFile.h>
#include "quxlang/data/output_object_symbol.hpp"

namespace quxlang
{
   void convert_llvm_object(llvm::object::ObjectFile const & obj, std::function<void(object_symbol)> callback);
}
#endif //RPNX_QUXLANG_CONVERT_LLVM_OBJECT_HEADER
