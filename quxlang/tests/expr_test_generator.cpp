//
// Created by Ryan Nicholl on 6/10/24.
//
#include "expr_test_provider.hpp"

/* For reference:
 *
 #define QUX_SUBCO_MEMBER_FUNC(nameV, retT, argsV) \
 rpnx::general_coroutine< compiler, retT > nameV argsV

/// Defines a coroutine member
#define QUX_SUBCO_MEMBER_FUNC_DEF(classN, nameV, retT, argsV) \
rpnx::general_coroutine< quxlang::compiler, retT> quxlang::classN::nameV argsV

virtual QUX_SUBCO_MEMBER_FUNC(create_temporary_storage, vmir2::storage_index, (type_symbol type)) override;
virtual QUX_SUBCO_MEMBER_FUNC(lookup_symbol, std::optional< vmir2::storage_index >, (type_symbol sym)) override;
virtual QUX_SUBCO_MEMBER_FUNC(emit, void, (vmir2::vm_instruction)) override;
virtual QUX_SUBCO_MEMBER_FUNC(index_type, type_symbol, (vmir2::storage_index)) override;
virtual QUX_SUBCO_MEMBER_FUNC(create_string_literal, vmir2::storage_index, (std::string)) override;
virtual QUX_SUBCO_MEMBER_FUNC(create_numeric_literal, vmir2::storage_index, (std::string)) override;
            */
QUX_SUBCO_MEMBER_FUNC_DEF(expr_test_provider::interface, lookup_symbol, std::optional< quxlang::vmir2::storage_index >, (type_symbol sym))
{
    if (auto it = gen->loadable_symbols.find(sym); it != gen->loadable_symbols.end())
    {
        co_return it->second;
    }

    throw std::logic_error("Symbol not found");
}

QUX_SUBCO_MEMBER_FUNC_DEF(expr_test_provider::interface, create_temporary_storage, quxlang::vmir2::storage_index, (type_symbol type))
{
    gen->slots.emplace_back();
    gen->slots.back().type = type;
    co_return gen->slots.size() - 1;
}

QUX_SUBCO_MEMBER_FUNC_DEF(expr_test_provider::interface, emit, void, (vmir2::vm_instruction instr))
{
    gen->instructions.push_back(instr);
    co_return;
}

QUX_SUBCO_MEMBER_FUNC_DEF(expr_test_provider::interface, index_type, quxlang::type_symbol, (vmir2::storage_index index))
{
    if (index == 0)
        throw std::logic_error("Invalid index");
    if (index >= gen->slots.size())
        throw std::logic_error("Invalid index");
    co_return gen->slots.at(index).type;
}

QUX_SUBCO_MEMBER_FUNC_DEF(expr_test_provider::interface, create_string_literal, quxlang::vmir2::storage_index, (std::string value))
{
    gen->slots.emplace_back();

    rpnx::unimplemented();

    // Need the following line to make this a valid coroutine
    co_return 0;
}

QUX_SUBCO_MEMBER_FUNC_DEF(expr_test_provider::interface, create_numeric_literal, quxlang::vmir2::storage_index, (std::string value))
{
    gen->slots.emplace_back();
    gen->slots.back().type = numeric_literal_reference{};
    gen->slots.back().literal_value = value;
    co_return gen->slots.size() - 1;
}
