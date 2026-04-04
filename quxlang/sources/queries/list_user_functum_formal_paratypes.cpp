// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/list_user_functum_formal_paratypes_spec.hpp>

#include "quxlang/operators.hpp"


rpnx::querygraph::coroutine< quxlang::list_user_functum_formal_paratypes_spec > quxlang::list_user_functum_formal_paratypes_impl(type_symbol input)
{
    std::string name = to_string(input);

    auto decls = co_await rpnx::querygraph::query_request< functum_list_user_overload_declarations_query >(input);

    std::vector< paratype > result;

    bool is_member_func = typeis< submember >(input);
    bool is_constructor = is_member_func && input.get_as< submember >().name == "CONSTRUCTOR";
    bool is_destructor = is_member_func && input.get_as< submember >().name == "DESTRUCTOR";

    auto thistype_type = type_parent(input).value_or(void_type{});

    for (auto const& decl : decls)
    {
        paratype p;

        for (auto const& param : decl.header.call_parameters)
        {
            contextual_type_reference ctx_type{.context = input, .type = param.type};

            auto type = co_await rpnx::querygraph::query_request< lookup_query >(ctx_type);
            if (!type)
            {
                throw std::logic_error("Type not found");
            }

            parameter_type formal_parameter;

            formal_parameter.type = type.value();

            // TODO: do all symbol lookups in this context
            formal_parameter.default_value = param.default_expr;

            if (param.api_name)
            {
                p.named[param.api_name.value()] = formal_parameter;
            }
            else
            {
                p.positional.push_back(formal_parameter);
            }
        }

        if (is_member_func && !p.named.contains("THIS"))
        {
            if (!is_constructor && !is_destructor)
            {
                parameter_type default_this{};
                // By default, AUTO& THISTYPE
                default_this.type = ptrref_type{.target = thistype_type, .ptr_class = pointer_class::ref, .qual = qualifier::auto_};
                p.named["THIS"] = default_this;
            }
            else if (is_constructor)
            {
                parameter_type default_this{};
                default_this.type = nvalue_slot{.target = thistype_type};

                p.named["THIS"] = default_this;
            }
            else
            {
                parameter_type default_this{};
                default_this.type = dvalue_slot{.target = thistype_type};
                p.named["THIS"] = default_this;
            }
        }

        result.push_back(p);
    }

    co_return result;
}
