// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/symbol_type_spec.hpp>
#include <quxlang/macros.hpp>


rpnx::querygraph::coroutine< quxlang::symbol_type_spec > quxlang::symbol_type_impl(type_symbol input)
{
    auto type_str = to_string(input);

    if (typeis< nvalue_slot >(input) || typeis< dvalue_slot >(input) || typeis< array_initializer_type >(input))
    {
        co_return symbol_kind::pseudotype;
    }

    auto functions = co_await rpnx::querygraph::request< functum_overloads_query >(input);
    if (functions.size() > 0)
    {
      co_return symbol_kind::functum;
    }

    if (typeis< absolute_module_reference >(input))
    {
        // TODO: Check if the module exists or not.
        co_return symbol_kind::module;
    }
    else if ( typeis< numeric_literal_reference >(input) || typeis< bool_type >(input) || typeis< int_type >(input) || typeis< procedure_type >(input) || typeis< ptrref_type >(input) || typeis<nvalue_slot>(input) || is_ref(input) || typeis<dvalue_slot>(input) || typeis< byte_type >(input) || typeis< initguard_type >(input) || typeis< initguard_lock_type >(input)
     || typeis<readonly_constant>(input)
    )
    {
        co_return symbol_kind::class_;
    }
    else if (typeis <void_type>(input))
    {
        co_return symbol_kind::noexist;
    }
    else if (typeis< submember >(input) || typeis< subsymbol >(input))
    {
        auto parent = type_parent(input).value();

        auto parent_kind = co_await rpnx::querygraph::request< symbol_type_query >(parent);

        if (parent_kind == symbol_kind::noexist)
        {
            co_return symbol_kind::noexist;
        }

        if (parent_kind == symbol_kind::class_)
        {
            auto decls = co_await rpnx::querygraph::request< functum_overloads_query >(input);

            if (decls.size() > 0)
            {
                co_return symbol_kind::functum;
            }
        }

        auto s = co_await rpnx::querygraph::request< symboid_query >(input);

        if (typeis< functum >(s))
        {
            co_return symbol_kind::functum;
        }
        else if (typeis< ast2_variable_declaration >(s))
        {
            if (typeis< subsymbol >(input))
            {
                co_return symbol_kind::global_variable;
            }
            else if (typeis< submember >(input))
            {
                co_return symbol_kind::member_variable;
            }
            else
            {
                throw compiler_bug("variable symbol must be a subsymbol or submember");
            }
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
        else if (typeis< ast2_templex >(s))
        {
            co_return symbol_kind::templex;
        }
        else
        {
            throw rpnx::unimplemented();
        }
    }
    else if (typeis <array_type>(input))
    {
        co_return symbol_kind::class_;
    }
    else if (typeis< instanciation_reference >(input))
    {
       auto const& selected_ast = co_await rpnx::querygraph::request< symboid_query >(input);

       if (typeis< ast2_class_declaration >(selected_ast))
       {
          co_return symbol_kind::class_;
       }
       else if (typeis< functum >(selected_ast))
       {
          co_return symbol_kind::functum;
       }
       else if (typeis< ast2_function_declaration >(selected_ast))
       {
          co_return symbol_kind::funtanoid;
       }
       else if (typeis< ast2_variable_declaration >(selected_ast))
       {
          if (typeis< subsymbol >(as< instanciation_reference >(input).temploid.templexoid))
          {
             co_return symbol_kind::global_variable;
          }
          else if (typeis< submember >(as< instanciation_reference >(input).temploid.templexoid))
          {
             co_return symbol_kind::member_variable;
          }
          else
          {
             throw compiler_bug("templated variable symbol must be a subsymbol or submember");
          }
       }
       else if (typeis< std::monostate >(selected_ast))
       {
          co_return symbol_kind::noexist;
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

       auto templexoid_type = co_await rpnx::querygraph::request< symbol_type_query >(templexoid);

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
