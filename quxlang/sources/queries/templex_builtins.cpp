// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/templex_builtins_spec.hpp>

#include <set>

using namespace quxlang;

rpnx::querygraph::coroutine< quxlang::templex_builtins_spec > quxlang::templex_builtins_impl(type_symbol input)
{
    std::vector< builtin_template_info > results;

    if (typeis< submember >(input))
    {
        submember const& member = as< submember >(input);
        std::optional< type_symbol > const atomic_value_type = atomic_type_argument(member.of);
        bool const initguard_member = typeis< initguard_type >(member.of);
        if (!atomic_value_type.has_value() && !initguard_member)
        {
            co_return results;
        }

        static std::set< std::string > const single_mode_members = {
            "LOAD",
            "STORE",
            "FETCH_ADD",
            "FETCH_SUB",
            "FETCH_AND",
            "FETCH_OR",
            "FETCH_XOR",
            "ADD",
            "SUB",
            "AND",
            "OR",
            "XOR",
        };

        builtin_template_info info;
        if (single_mode_members.contains(member.name))
        {
            info.template_args.named["T"] = declared_parameter{
                .name = "T",
                .kind = template_parameter_kind::type,
                .type = type_temploidic{},
            };
            results.push_back(std::move(info));
        }
        else if (member.name == "COMPARE_EXCHANGE")
        {
            info.template_args.named["SUCCESS"] = declared_parameter{
                .name = "SUCCESS",
                .kind = template_parameter_kind::type,
                .type = type_temploidic{},
            };
            info.template_args.named["FAILURE"] = declared_parameter{
                .name = "FAILURE",
                .kind = template_parameter_kind::type,
                .type = type_temploidic{},
            };
            results.push_back(std::move(info));
        }

        co_return results;
    }

    if (!typeis< builtin_symbol >(input))
    {
        co_return results;
    }

    auto const& builtin = as< builtin_symbol >(input);
    if (!is_builtin_allocator_name(builtin.name) && !is_builtin_atomic_templex_name(builtin.name))
    {
        co_return results;
    }

    builtin_template_info typed_info;
    typed_info.template_args.named["T"] = declared_parameter{
        .name = "T",
        .kind = template_parameter_kind::type,
        .type = type_temploidic{},
    };
    results.push_back(std::move(typed_info));

    if (is_builtin_allocator_name(builtin.name))
    {
        builtin_template_info sized_info;
        sized_info.template_args.named["SIZE"] = declared_parameter{
            .name = "SIZE",
            .kind = template_parameter_kind::value,
            .type = size_type{},
        };
        sized_info.template_args.named["ALIGN"] = declared_parameter{
            .name = "ALIGN",
            .kind = template_parameter_kind::value,
            .type = size_type{},
        };
        results.push_back(std::move(sized_info));
    }

    co_return results;
}
