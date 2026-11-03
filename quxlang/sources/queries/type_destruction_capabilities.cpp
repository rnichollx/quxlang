// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/type_destruction_capabilities_spec.hpp>

#include <set>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::type_destruction_capabilities_spec > quxlang::type_destruction_capabilities_impl(type_symbol input)
{
    type_destruction_capabilities result;
    std::vector< type_symbol > pending;
    pending.push_back(std::move(input));
    std::set< type_symbol > visited;

    while (!pending.empty())
    {
        type_symbol current = std::move(pending.back());
        pending.pop_back();
        if (!visited.insert(current).second || current.type_is< void_type >())
        {
            continue;
        }

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
            {
                pending.push_back(field.second);
            }
            continue;
        }

        class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(current);
        if (kind == class_kind::struct_)
        {
            std::vector< ast2_function_declaration > const& destructors = co_await rpnx::querygraph::request< functum_list_user_overload_declarations_query >(submember{.of = current, .name = "DESTRUCTOR"});
            for (ast2_function_declaration const& destructor : destructors)
            {
                if (destructor.header.execution_kind == function_execution_kind::coroutine)
                {
                    result.prefers_coroutine = true;
                    if (!(co_await rpnx::querygraph::request< user_default_dtor_exists_query >(current)))
                    {
                        result.is_async_only = true;
                        co_return result;
                    }
                    break;
                }
            }

            std::vector< struct_field > const& fields = co_await rpnx::querygraph::request< struct_field_list_query >(current);
            for (struct_field const& field : fields)
            {
                pending.push_back(field.type);
            }
            std::vector< struct_base_declaration > const& bases = co_await rpnx::querygraph::request< struct_direct_bases_query >(current);
            for (struct_base_declaration const& base : bases)
            {
                pending.push_back(base.base_type);
            }
        }
        else if (kind == class_kind::union_)
        {
            union_info const& info = co_await rpnx::querygraph::request< union_info_query >(current);
            for (union_option_info const& option : info.options)
            {
                pending.push_back(option.type);
            }
        }
        else if (kind == class_kind::variant)
        {
            variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(current);
            for (type_symbol const& alternative : info.alternatives)
            {
                pending.push_back(alternative);
            }
        }
    }

    co_return result;
}
