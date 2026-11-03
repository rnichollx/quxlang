// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/type_has_coroutine_lifecycle_spec.hpp>

#include <array>
#include <set>
#include <string>
#include <utility>
#include <vector>

auto quxlang::type_has_coroutine_lifecycle_impl(type_symbol input) -> rpnx::querygraph::coroutine< type_has_coroutine_lifecycle_spec >
{
    std::vector< type_symbol > pending{std::move(input)};
    std::set< type_symbol > visited;
    std::array< std::string, 6 > lifecycle_names{
        "CONSTRUCTOR", "FULLOBJECT_CONSTRUCTOR", "SUBOBJECT_CONSTRUCTOR", "DESTRUCTOR", "OPERATOR:=", "OPERATOR<->"};

    while (!pending.empty())
    {
        type_symbol current = std::move(pending.back());
        pending.pop_back();
        if (!visited.insert(current).second) continue;
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

        class_kind kind = co_await rpnx::querygraph::request< class_type_query >(current);
        if (kind == class_kind::struct_)
        {
            for (std::string const& name : lifecycle_names)
            {
                std::vector< ast2_function_declaration > const& declarations =
                    co_await rpnx::querygraph::request< functum_list_user_overload_declarations_query >(submember{.of = current, .name = name});
                for (ast2_function_declaration const& declaration : declarations)
                {
                    if (declaration.header.execution_kind == function_execution_kind::coroutine) co_return true;
                }
            }
            std::vector< struct_field > const& fields = co_await rpnx::querygraph::request< struct_field_list_query >(current);
            for (struct_field const& field : fields) pending.push_back(field.type);
            std::vector< struct_base_declaration > const& bases = co_await rpnx::querygraph::request< struct_direct_bases_query >(current);
            for (struct_base_declaration const& base : bases) pending.push_back(base.base_type);
        }
        else if (kind == class_kind::union_)
        {
            union_info const& info = co_await rpnx::querygraph::request< union_info_query >(current);
            for (union_option_info const& alternative : info.options) pending.push_back(alternative.type);
        }
        else if (kind == class_kind::variant)
        {
            variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(current);
            for (type_symbol const& alternative : info.alternatives) pending.push_back(alternative);
        }
    }
    co_return false;
}
