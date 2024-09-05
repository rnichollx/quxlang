//
// Created by Ryan Nicholl on 3/2/24.
//
#include <quxlang/compiler.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/res/symbol_type.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(symbol_type)
{
    auto type_str = to_string(input_val);

    auto functions = co_await QUX_CO_DEP(list_functum_overloads, (input_val));
    if (functions.size() > 0)
    {
      co_return symbol_kind::functum;
    }

        if (typeis< module_reference >(input_val))
    {
        // TODO: Check if the module exists or not.
        co_return symbol_kind::module;
    }
    else if ( typeis< numeric_literal_reference >(input_val) || typeis< primitive_type_bool_reference >(input_val) || typeis< primitive_type_integer_reference >(input_val) || typeis< instance_pointer_type >(input_val) || typeis< array_pointer_type >(input_val))
    {
        co_return symbol_kind::builtin_class;
    }
    else if (typeis <void_type>(input_val))
    {
        co_return symbol_kind::noexist;
    }
    else if (typeis< subdotentity_reference >(input_val) || typeis< subentity_reference >(input_val))
    {
        auto parent = qualified_parent(input_val).value();

        auto parent_kind = co_await QUX_CO_DEP(symbol_type, (parent));

        if (parent_kind == symbol_kind::noexist)
        {
            co_return symbol_kind::noexist;
        }

        if (parent_kind == symbol_kind::builtin_class)
        {
            auto decls = co_await QUX_CO_DEP(list_builtin_functum_overloads, (input_val));

            if (decls.size() > 0)
            {
                co_return symbol_kind::functum;
            }
        }

        auto s = co_await QUX_CO_DEP(symboid, (input_val));

        if (typeis< functum >(s))
        {
            co_return symbol_kind::functum;
        }
        else if (typeis< ast2_class_declaration >(s))
        {
            co_return symbol_kind::user_class;
        }
        else if (typeis< ast2_namespace_declaration >(s))
        {
            co_return symbol_kind::namespace_;
        }

        else
        {
            throw rpnx::unimplemented();
        }
    }

    throw rpnx::unimplemented();
}