// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/coroutine_destructor_spec.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <set>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::coroutine_destructor_spec > quxlang::coroutine_destructor_impl(coroutine_destructor_request input)
{
    coroutine_destructor_selection selected;
    std::vector< type_symbol > pending{input.object_type};
    std::set< type_symbol > visited;

    // Direct overload selection is evaluated once per reachable owning type.
    // The visited set also terminates cycles through boxed alternatives.
    while (!pending.empty())
    {
        type_symbol current = std::move(pending.back());
        pending.pop_back();
        if (!visited.insert(current).second || current.type_is< void_type >()) continue;
        if (current.type_is< attached_type_reference >())
        {
            pending.push_back(current.get_as< attached_type_reference >().carrying_type);
            continue;
        }
        if (current.type_is< array_type >())
        {
            pending.push_back(current.get_as< array_type >().element_type);
            continue;
        }
        if (current.type_is< composite_type >())
        {
            for (std::pair< std::string const, type_symbol > const& field : current.get_as< composite_type >().fields)
                pending.push_back(field.second);
            continue;
        }

        class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(current);
        if (kind == class_kind::struct_)
        {
            type_symbol const destructor_symbol = submember{.of = current, .name = "DESTRUCTOR"};
            bool declares_coroutine = false;
            std::vector< ast2_function_declaration > const& declarations = co_await rpnx::querygraph::request< functum_list_user_overload_declarations_query >(destructor_symbol);
            for (ast2_function_declaration const& declaration : declarations)
                declares_coroutine = declares_coroutine || declaration.header.execution_kind == function_execution_kind::coroutine;

            if (declares_coroutine)
            {
                initialization_reference initialization;
                initialization.initializee = destructor_symbol;
                initialization.lookup_kind = function_lookup_kind::invocation;
                initialization.implicit_named_arguments.insert("CONTROLLER");
                initialization.parameters = instatype_from_invotype(invotype{.named{
                    {"THIS", make_mref(current)},
                    {"CONTROLLER", make_mref(remove_ref(input.controller_type))},
                }});
                initialization.adaptations = allowed_adaptations::none;
                std::optional< instanciation_reference > const& operation = co_await rpnx::querygraph::request< functum_initialize_query >(initialization);
                if (operation.has_value())
                {
                    std::optional< ast2_function_declaration > const& declaration =
                        co_await rpnx::querygraph::request< function_declaration_query >(operation->temploid);
                    if (!declaration.has_value()) throw compiler_bug("Selected destructor body has no declaration");
                    if (current == input.object_type)
                        co_return coroutine_destructor_selection{.operation = operation,
                            .is_coroutine = declaration->header.execution_kind == function_execution_kind::coroutine,
                            .requires_subobject_cleanup = true};
                    selected.requires_subobject_cleanup = true;
                    co_return selected;
                }
            }

            bool const has_ordinary_body = co_await rpnx::querygraph::request< user_default_dtor_exists_query >(current);
            if (declares_coroutine && !has_ordinary_body)
                throw semantic_compilation_error("No coroutine destructor of " + to_string(current) + " accepts controller " + to_string(input.controller_type));
            if (current == input.object_type && has_ordinary_body)
            {
                initialization_reference initialization;
                initialization.initializee = destructor_symbol;
                initialization.lookup_kind = function_lookup_kind::invocation;
                initialization.parameters = instatype_from_invotype(invotype{.named{{"THIS", make_mref(current)}}});
                initialization.adaptations = allowed_adaptations::none;
                selected.operation = co_await rpnx::querygraph::request< functum_initialize_query >(initialization);
                selected.requires_subobject_cleanup = true;
            }

            std::vector< struct_field > const& fields = co_await rpnx::querygraph::request< struct_field_list_query >(current);
            for (struct_field const& field : fields) pending.push_back(field.type);
            std::vector< struct_base_declaration > const& bases = co_await rpnx::querygraph::request< struct_direct_bases_query >(current);
            for (struct_base_declaration const& base : bases) pending.push_back(base.base_type);
        }
        else if (kind == class_kind::union_)
        {
            union_info const& info = co_await rpnx::querygraph::request< union_info_query >(current);
            for (union_option_info const& option : info.options) pending.push_back(option.type);
        }
        else if (kind == class_kind::variant)
        {
            variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(current);
            for (type_symbol const& alternative : info.alternatives) pending.push_back(alternative);
        }
    }

    if (selected.operation.has_value()) co_return selected;
    std::optional< instanciation_reference > const& operation = co_await rpnx::querygraph::request< class_default_dtor_query >(input.object_type);
    co_return coroutine_destructor_selection{.operation = operation};
}
