// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_MANIPULATORS_LLVM_SYMBOL_RELOCATION_HEADER_GUARD
#define QUXLANG_MANIPULATORS_LLVM_SYMBOL_RELOCATION_HEADER_GUARD
#include "quxlang/data/code_relocation.hpp"

#include <llvm/Object/ObjectFile.h>

namespace quxlang
{
    std::optional< symbol_relocation > to_symbol_relocation(llvm::object::RelocationRef const & ref);
}

#endif //LLVM_SYMBOL_RELOCATION_HPP
