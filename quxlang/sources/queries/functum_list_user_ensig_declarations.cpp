// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/functum_list_user_ensig_declarations_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_list_user_ensig_declarations_spec > quxlang::functum_list_user_ensig_declarations_impl(type_symbol input)
{
    auto const& decls = co_await rpnx::querygraph::request< functum_list_user_overload_declarations_query >(input);

    std::vector< temploid_ensig > output;

    for (std::size_t i = 0; i < decls.size(); i++)
    {
        auto const& head = decls.at(i).header;

        temploid_ensig ensig;
        ensig.priority = head.priority;
        ensig.enable_if = head.enable_if;

        for (std::size_t y = 0; y < head.call_parameters.size(); y++)
        {
            auto const& param = head.call_parameters.at(y);

            argif arg;
            if (param.default_expr.has_value())
            {
                arg.is_defaulted = true;
            }

            arg.type = param.type;
            arg.is_pack = param.is_pack;

            if (param.api_name.has_value())
            {
                if (ensig.interface.named.contains(param.api_name.value()))
                {
                    throw quxlang::semantic_compilation_error("Duplicate parameter name");
                    //
                }

                ensig.interface.named[param.api_name.value()] = arg;
            }
            else
            {
                ensig.interface.positional.push_back(arg);
            }
        }

        output.push_back(ensig);
    }

    co_return output;
}
