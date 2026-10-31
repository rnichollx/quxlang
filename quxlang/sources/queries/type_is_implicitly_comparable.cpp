// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/type_is_implicitly_comparable_spec.hpp>
#include <quxlang/keywords.hpp>

#include <set>
#include <string>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::type_is_implicitly_comparable_spec > quxlang::type_is_implicitly_comparable_impl(type_symbol input)
{
    std::vector< type_symbol > pending{std::move(input)};
    std::set< type_symbol > visited;
    while (!pending.empty())
    {
        type_symbol current = std::move(pending.back());
        pending.pop_back();
        if (!visited.insert(current).second || (co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(current)))
        {
            continue;
        }
        if (typeis< attached_type_reference >(current))
        {
            pending.push_back(as< attached_type_reference >(current).carrying_type);
            continue;
        }
        if (typeis< array_type >(current))
        {
            pending.push_back(as< array_type >(current).element_type);
            continue;
        }
        if (co_await rpnx::querygraph::request< symbol_type_query >(current) != symbol_kind::class_)
        {
            co_return false;
        }

        class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(current);
        if (kind == class_kind::variant || kind == class_kind::union_)
        {
            std::vector< type_symbol > const& alternatives = co_await rpnx::querygraph::request< fusion_alternatives_list_query >(current);
            for (type_symbol const& alternative : alternatives)
            {
                if (!typeis< void_type >(alternative))
                {
                    pending.push_back(alternative);
                }
            }
            continue;
        }
        if (kind != class_kind::struct_)
        {
            co_return false;
        }

        std::set< std::string > const& tags = co_await rpnx::querygraph::request< struct_tags_query >(current);
        if (tags.contains(keywords::polymorphic) || tags.contains(keywords::virtual_polymorphic))
        {
            co_return false;
        }
        std::vector< struct_base_declaration > const& bases = co_await rpnx::querygraph::request< struct_direct_bases_query >(current);
        for (struct_base_declaration const& base : bases)
        {
            pending.push_back(base.base_type);
        }
        std::vector< struct_field > const& fields = co_await rpnx::querygraph::request< struct_field_list_query >(current);
        for (struct_field const& field : fields)
        {
            pending.push_back(field.type);
        }
    }
    co_return true;
}
