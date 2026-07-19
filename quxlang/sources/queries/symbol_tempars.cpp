// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/symbol_tempars_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/macros.hpp"

#include <functional>

#include "query_helpers.hpp"

namespace quxlang::detail
{
    struct symbol_tempars_helpers
    {
        template < typename T >
        static auto merge_set(std::set< T >& target, std::set< T > const& source) -> void
        {
            for (T const& item : source)
            {
                target.insert(item);
            }
        }
    };
} // namespace quxlang::detail

namespace quxlang
{
    rpnx::querygraph::coroutine< symbol_tempars_spec > symbol_tempars_impl(type_symbol input)
    {
        using tempar_subquery = rpnx::querygraph::coroutine< symbol_tempars_spec >::cosubroutine< tempar_name_set >;
        std::function< tempar_subquery(type_symbol const&) > get_symbol_tempars;
        auto get_ensig_tempars = [&](temploid_ensig const& ensig)
            -> tempar_subquery
        {
            tempar_name_set result;
            for (auto const& param : ensig.interface.positional)
            {
                detail::symbol_tempars_helpers::merge_set(result, co_await get_symbol_tempars(param.type));
            }
            for (auto const& [_, param] : ensig.interface.named)
            {
                detail::symbol_tempars_helpers::merge_set(result, co_await get_symbol_tempars(param.type));
            }
            co_return result;
        };

        get_symbol_tempars = [&](type_symbol const& value)
            -> tempar_subquery
        {
            if (typeis< auto_temploidic >(value))
            {
                co_return tempar_name_set{as< auto_temploidic >(value).name};
            }
            if (typeis< decay_temploidic >(value))
            {
                co_return tempar_name_set{as< decay_temploidic >(value).name};
            }
            if (typeis< type_temploidic >(value))
            {
                co_return tempar_name_set{as< type_temploidic >(value).name};
            }
            if (typeis< numeric_literal_any_temploidic >(value))
            {
                co_return tempar_name_set{as< numeric_literal_any_temploidic >(value).name};
            }
            if (typeis< string_literal_any_temploidic >(value))
            {
                co_return tempar_name_set{as< string_literal_any_temploidic >(value).name};
            }
            if (typeis< temploid_reference >(value))
            {
                tempar_name_set result = co_await get_symbol_tempars(as< temploid_reference >(value).templexoid);
                auto ensig = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(as< temploid_reference >(value));
                if (ensig.has_value())
                {
                    detail::symbol_tempars_helpers::merge_set(result, co_await get_ensig_tempars(*ensig));
                }
                co_return result;
            }
            if (typeis< submember >(value))
            {
                co_return co_await get_symbol_tempars(as< submember >(value).of);
            }
            if (typeis< subsymbol >(value))
            {
                co_return co_await get_symbol_tempars(as< subsymbol >(value).of);
            }
            if (typeis< instanciation_reference >(value))
            {
                tempar_name_set result = co_await get_symbol_tempars(type_symbol(as< instanciation_reference >(value).temploid));
                for (auto const& param : as< instanciation_reference >(value).params.positional)
                {
                    detail::symbol_tempars_helpers::merge_set(result, co_await get_symbol_tempars(parameter_instantiation_type(param)));
                }
                for (auto const& [_, param] : as< instanciation_reference >(value).params.named)
                {
                    detail::symbol_tempars_helpers::merge_set(result, co_await get_symbol_tempars(parameter_instantiation_type(param)));
                }
                co_return result;
            }
            if (typeis< pack_arg_type_ref >(value))
            {
                co_return tempar_name_set{};
            }

            co_return tempar_name_set{};
        };

        co_return co_await get_symbol_tempars(input);
    }
} // namespace quxlang
