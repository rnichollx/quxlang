// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/temploid_formal_ensig_spec.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace quxlang
{
    rpnx::querygraph::coroutine< temploid_formal_ensig_spec > temploid_formal_ensig_impl(temploid_reference input)
    {
        auto resolve_unique_or_explicit_id = [&](std::size_t count) -> std::optional< std::uint64_t >
        {
            if (input.overload_id.has_value())
            {
                if (*input.overload_id >= count)
                {
                    return std::nullopt;
                }
                return *input.overload_id;
            }

            if (count == 1)
            {
                return 0;
            }
            return std::nullopt;
        };

        auto template_parameter_name = [](declared_parameter const& param) -> std::optional< std::string >
        {
            if (param.name.has_value())
            {
                return param.name;
            }
            if (param.api_name.has_value())
            {
                return param.api_name;
            }

            auto const& param_type = param.type;
            if (typeis< auto_temploidic >(param_type))
            {
                auto const& name = as< auto_temploidic >(param_type).name;
                if (!name.empty())
                {
                    return name;
                }
            }
            else if (typeis< decay_temploidic >(param_type))
            {
                auto const& name = as< decay_temploidic >(param_type).name;
                if (!name.empty())
                {
                    return name;
                }
            }
            else if (typeis< type_temploidic >(param_type))
            {
                auto const& name = as< type_temploidic >(param_type).name;
                if (!name.empty())
                {
                    return name;
                }
            }

            return std::nullopt;
        };

        auto declared_parameters_to_ensig = [&](declared_parameters const& params, std::optional< std::int64_t > priority, type_symbol template_context)
            -> rpnx::querygraph::coroutine< temploid_formal_ensig_spec >::cosubroutine< std::optional< temploid_ensig > >
        {
            temploid_ensig ensig;
            ensig.priority = priority;

            for (auto const& param : params.positional)
            {
                auto canonical_param = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = template_context,
                    .type = param.type,
                });
                if (!canonical_param.has_value())
                {
                    co_return std::nullopt;
                }

                ensig.interface.positional.push_back(argif{
                    .type = *canonical_param,
                    .requires_static_value = param.kind == template_parameter_kind::value,
                });
            }

            for (auto const& [name, param] : params.named)
            {
                auto canonical_param = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = template_context,
                    .type = param.type,
                });
                if (!canonical_param.has_value())
                {
                    co_return std::nullopt;
                }

                auto canonical_name = template_parameter_name(param).value_or(name);
                ensig.interface.named[canonical_name] = argif{
                    .type = *canonical_param,
                    .requires_static_value = param.kind == template_parameter_kind::value,
                };
            }

            co_return ensig;
        };

        auto templexoid_kind = co_await rpnx::querygraph::request< symbol_type_query >(input.templexoid);
        if (templexoid_kind == symbol_kind::functum)
        {
            auto const& user_overloads = co_await rpnx::querygraph::request< functum_map_user_formal_ensigs_query >(input.templexoid);
            auto const& builtin_overloads = co_await rpnx::querygraph::request< functum_builtin_overloads_query >(input.templexoid);
            auto resolved_id = resolve_unique_or_explicit_id(user_overloads.size() + builtin_overloads.size());
            if (!resolved_id.has_value())
            {
                co_return std::nullopt;
            }

            if (*resolved_id < user_overloads.size())
            {
                for (auto const& [ensig, index] : user_overloads)
                {
                    if (index == *resolved_id)
                    {
                        co_return ensig;
                    }
                }
                throw compiler_bug("User functum overload id did not map to a formal ensig");
            }

            std::uint64_t builtin_index = *resolved_id - static_cast< std::uint64_t >(user_overloads.size());
            std::uint64_t current_index = 0;
            for (auto const& ensig : builtin_overloads)
            {
                if (current_index == builtin_index)
                {
                    co_return ensig;
                }
                current_index++;
            }

            throw compiler_bug("Builtin functum overload id did not map to a formal ensig");
        }

        if (templexoid_kind == symbol_kind::templex)
        {
            auto builtin_templates = co_await rpnx::querygraph::request< templex_builtins_query >(input.templexoid);
            if (!builtin_templates.empty())
            {
                auto resolved_id = resolve_unique_or_explicit_id(builtin_templates.size());
                if (!resolved_id.has_value())
                {
                    co_return std::nullopt;
                }

                auto const& builtin_template = builtin_templates.at(static_cast< std::vector< builtin_template_info >::size_type >(*resolved_id));
                auto template_context = type_parent(input.templexoid).value_or(context_reference{});
                co_return co_await declared_parameters_to_ensig(builtin_template.template_args, builtin_template.priority, template_context);
            }

            auto const& sym = co_await rpnx::querygraph::request< symboid_query >(input.templexoid);
            if (!typeis< ast2_templex >(sym))
            {
                throw compiler_bug("Templex overload reference parent did not resolve to ast2_templex");
            }

            auto const& templex = as< ast2_templex >(sym);
            auto resolved_id = resolve_unique_or_explicit_id(templex.templates.size());
            if (!resolved_id.has_value())
            {
                co_return std::nullopt;
            }

            auto template_context = type_parent(input.templexoid).value_or(context_reference{});
            auto const& tmpl = templex.templates.at(static_cast< std::vector< ast2_template_declaration >::size_type >(*resolved_id));
            co_return co_await declared_parameters_to_ensig(tmpl.m_template_args, tmpl.priority, template_context);
        }

        if (templexoid_kind == symbol_kind::noexist)
        {
            co_return std::nullopt;
        }

        throw compiler_bug("temploid_formal_ensig called on a non-templexoid parent");
    }
} // namespace quxlang
