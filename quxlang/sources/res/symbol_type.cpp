// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
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
    else if ( typeis< numeric_literal_reference >(input_val) || typeis< bool_type >(input_val) || typeis< int_type >(input_val) || typeis< pointer_type >(input_val) || typeis<nvalue_slot>(input_val) || is_ref(input_val))
    {
        co_return symbol_kind::class_;
    }
    else if (typeis <void_type>(input_val))
    {
        co_return symbol_kind::noexist;
    }
    else if (typeis< submember >(input_val) || typeis< subsymbol >(input_val))
    {
        auto parent = qualified_parent(input_val).value();

        auto parent_kind = co_await QUX_CO_DEP(symbol_type, (parent));

        if (parent_kind == symbol_kind::noexist)
        {
            co_return symbol_kind::noexist;
        }

        if (parent_kind == symbol_kind::class_)
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
            co_return symbol_kind::class_;
        }
        else if (typeis< ast2_namespace_declaration >(s))
        {
            co_return symbol_kind::namespace_;
        }
        else if (typeis< std::monostate >(s))
        {
            co_return symbol_kind::noexist;
        }
        else
        {
            throw rpnx::unimplemented();
        }
    }
    else if (typeis< instanciation_reference >(input_val))
    {
       temploid_reference const & temploid = as<instanciation_reference>(input).temploid;

       auto temploid_type = co_await QUX_CO_DEP(symbol_type, (temploid));

       if (temploid_type == symbol_kind::function)
       {
          co_return symbol_kind::funtanoid;
       }
       else
       {
          throw rpnx::unimplemented();
       }
    }
    else if (typeis<temploid_reference>(input))
    {
       temploid_reference const & temploid = as<temploid_reference>(input);

       auto const & templexoid = temploid.templexoid;

       auto templexoid_type = co_await QUX_CO_DEP(symbol_type, (templexoid));

       if (templexoid_type == symbol_kind::templex)
       {
         co_return symbol_kind::template_;
       }
       else if (templexoid_type == symbol_kind::functum)
       {
         co_return symbol_kind::function;
       }
       else if (templexoid_type == symbol_kind::noexist)
       {
         co_return symbol_kind::noexist;
       }
       else
       {
         throw rpnx::unimplemented();
       }

    }

    throw rpnx::unimplemented();
}