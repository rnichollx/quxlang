// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef LLVM_SYMBOL_RELOCATION_HPP
#define LLVM_SYMBOL_RELOCATION_HPP
#include "quxlang/data/code_relocation.hpp"

#include <llvm/Object/ObjectFile.h>

namespace quxlang
{
    std::optional< symbol_relocation > to_symbol_relocation(llvm::object::RelocationRef const & ref);
}

#endif //LLVM_SYMBOL_RELOCATION_HPP
