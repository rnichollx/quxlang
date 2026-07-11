// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/symbol_type_spec.hpp>
#include <quxlang/data/lambda_types.hpp>
#include <quxlang/macros.hpp>

#include "quxlang/manipulators/typeutils.hpp"


rpnx::querygraph::coroutine< quxlang::symbol_type_spec > quxlang::symbol_type_impl(type_symbol input)
{
   // auto type_str = to_string(input);

    if (typeis< nvalue_slot >(input) || typeis< dvalue_slot >(input) || typeis< array_initializer_type >(input))
    {
        co_return symbol_kind::pseudotype;
    }

    if (parse_lambda_closure_symbol(input).has_value())
    {
        co_return symbol_kind::class_;
    }

    if (!(co_await rpnx::querygraph::request< templex_builtins_query >(input)).empty())
    {
        co_return symbol_kind::templex;
    }

    if (typeis< builtin_symbol >(input))
    {
        builtin_symbol const& builtin = as< builtin_symbol >(input);
        if (is_builtin_atomic_access_mode_name(builtin.name))
        {
            co_return symbol_kind::class_;
        }
        if (is_builtin_global_functum_name(builtin.name))
        {
            co_return symbol_kind::functum;
        }
        if (builtin.name == "UNIT_TEST_COUNT" || builtin.name == "UNIT_TEST_NAMES" || builtin.name == "UNIT_TEST_PROC")
        {
            co_return symbol_kind::global_variable;
        }
        co_return symbol_kind::noexist;
    }

    if (typeis<temploid_reference>(input))
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
         throw compiler_bug("temploid_reference parent resolved to unexpected symbol kind " + std::to_string(static_cast<int>(templexoid_type)) + ": " + to_string(temploid.templexoid));
       }
    }

    if (typeis< instanciation_reference >(input))
    {
       auto const& selected_ast = co_await rpnx::querygraph::request< symboid_query >(input);

       if (typeis< ast2_struct_declaration >(selected_ast) || typeis< ast2_enum_declaration >(selected_ast) || typeis< ast2_flagset_declaration >(selected_ast))
       {
          co_return symbol_kind::class_;
       }
       else if (typeis< ast2_interface_declaration >(selected_ast))
       {
          co_return symbol_kind::interface_;
       }
       else if (typeis< ast2_implementation_declaration >(selected_ast))
       {
          co_return symbol_kind::implementation_;
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
       else if (typeis< ast2_test >(selected_ast))
       {
          co_return symbol_kind::test;
       }
       else
       {
          throw compiler_bug("instanciation_reference resolved to unsupported symboid kind for " + to_string(input));
       }
    }

    if (typeis< subtag_type >(input))
    {
        std::optional< parameter_instantiation > const binding = co_await rpnx::querygraph::request< subtag_binding_query >(as< subtag_type >(input));
        if (!binding.has_value())
        {
            co_return symbol_kind::noexist;
        }
        if (binding->type_is< parameter_value_instantiation >())
        {
            co_return symbol_kind::global_variable;
        }
        co_return co_await rpnx::querygraph::request< symbol_type_query >(binding->get_as< parameter_type_instantiation >().type);
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
    else if ( typeis< numeric_literal_type >(input) ||  typeis< string_literal_type >(input) || typeis< bool_type >(input) || typeis< int_type >(input) || typeis< float_type >(input) || typeis< procedure_type >(input) || typeis< ptrref_type >(input) || typeis<nvalue_slot>(input) || is_ref(input) || typeis<dvalue_slot>(input) || typeis< byte_type >(input) || typeis< initguard_type >(input) || typeis< initguard_lock_type >(input) || typeis< constexpr_proxy >(input) || typeis< thistype >(input)
     || typeis<readonly_constant>(input)
     || typeis< address_type >(input)
     || typeis< attached_type_reference >(input)
     || typeis< storage >(input)
     || typeis< aligned_storage >(input)
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

        class_kind const parent_class_kind = parent_kind == symbol_kind::class_
                                                 ? co_await rpnx::querygraph::request< class_type_query >(parent)
                                                 : class_kind::noexist;

        if (parent_class_kind == class_kind::enum_)
        {
            std::string const& value_name = typeis< subsymbol >(input) ? as< subsymbol >(input).name : as< submember >(input).name;
            enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(parent);
            for (enum_value_info const& value : info.values)
            {
                if (value.name == value_name)
                {
                    co_return symbol_kind::enum_value;
                }
            }
        }

        if (parent_class_kind == class_kind::flagset)
        {
            std::string const& value_name = typeis< subsymbol >(input) ? as< subsymbol >(input).name : as< submember >(input).name;
            flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(parent);
            for (flagset_value_info const& value : info.values)
            {
                if (value.name == value_name)
                {
                    co_return symbol_kind::flagset_value;
                }
            }
        }

        if (parent_kind == symbol_kind::class_)
        {
            auto decls = co_await rpnx::querygraph::request< functum_overloads_query >(input);

            if (decls.size() > 0)
            {
                co_return symbol_kind::functum;
            }
        }
        if (parent_kind == symbol_kind::interface_)
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
        else if (typeis< ast2_struct_declaration >(s) || typeis< ast2_enum_declaration >(s) || typeis< ast2_flagset_declaration >(s))
        {
            co_return symbol_kind::class_;
        }
        else if (typeis< ast2_interface_declaration >(s))
        {
            co_return symbol_kind::interface_;
        }
        else if (typeis< ast2_implementation_declaration >(s))
        {
            co_return symbol_kind::implementation_;
        }
        else if (typeis< ast2_namespace_declaration >(s))
        {
            co_return symbol_kind::namespace_;
        }
        else if (typeis< ast2_test >(s))
        {
            co_return symbol_kind::test;
        }
        else if (typeis< ast2_asm_procedure_declaration >(s))
        {
            co_return symbol_kind::functum;
        }
        else if (typeis< ast2_extern_procedure >(s))
        {
            co_return symbol_kind::functum;
        }
        else if (typeis< ast2_option >(s))
        {
            co_return symbol_kind::option;
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
            throw compiler_bug("subsymbol/submember resolved to unsupported symboid kind for " + to_string(input));
        }
    }
    else if (typeis <array_type>(input))
    {
        co_return symbol_kind::class_;
    }
    throw compiler_bug("symbol_type does not support symbol: " + to_string(input));
}
