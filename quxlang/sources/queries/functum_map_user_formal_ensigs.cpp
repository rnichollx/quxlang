// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/functum_map_user_formal_ensigs_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <vector>

#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include <quxlang/macros.hpp>

using namespace quxlang;


rpnx::querygraph::coroutine< quxlang::functum_map_user_formal_ensigs_spec > quxlang::functum_map_user_formal_ensigs_impl(type_symbol input)
{
    auto const& decls = co_await rpnx::querygraph::request< functum_list_user_ensig_declarations_query >(input);

    std::optional<std::string> input_name;
    if constexpr (QUXLANG_IN_DEBUG)
    {
        input_name = quxlang::to_string(input);
    }
    std::map< temploid_ensig, std::size_t > output;

    bool is_member_functum = typeis< submember >(input);
    std::optional< type_symbol > class_type;
    type_symbol thistype_type = thistype{};
    bool is_ctor = false;
    bool is_dtor = false;
    if (is_member_functum)
    {
        submember const& m = as< submember >(input);
        class_type = m.of;
        if (m.name == "CONSTRUCTOR")
        {
            is_ctor = true;
        }
        else if (m.name == "DESTRUCTOR")
        {
            is_dtor = true;
        }
    }

    for (std::size_t i = 0; i < decls.size(); i++)
    {
        auto const& decl = decls.at(i);
        temploid_ensig formal_ensig;
        formal_ensig.priority = decl.priority;
        formal_ensig.enable_if = decl.enable_if;
        for (auto const& param : decl.interface.named)
        {
            contextual_type_reference declared_type_with_context;
            declared_type_with_context.type = param.second.type;

            // We can't look at the typedefs of the function while we are resolving the function's formal ensig, as this would cause a circular dependency.
            declared_type_with_context.context = type_parent(input).value_or(void_type{});

            auto const& formal_type_opt = co_await rpnx::querygraph::request< lookup_query >(declared_type_with_context);
            if (!formal_type_opt.has_value())
            {
                throw quxlang::semantic_compilation_error("Type not found");
            }
            if (param.second.is_pack)
            {
                throw quxlang::semantic_compilation_error("Named variadic packs are not supported");
            }
            formal_ensig.interface.named[param.first] = argif{.type = formal_type_opt.value(), .is_defaulted = param.second.is_defaulted, .is_pack = param.second.is_pack};
        }
        for (auto const& param : decl.interface.positional)
        {
            contextual_type_reference declared_type_with_context;
            declared_type_with_context.type = param.type;
            declared_type_with_context.context = type_parent(input).value_or(void_type{});
            auto const& formal_type_opt = co_await rpnx::querygraph::request< lookup_query >(declared_type_with_context);
            if (!formal_type_opt.has_value())
            {
                throw quxlang::semantic_compilation_error("Type not found");
            }
            formal_ensig.interface.positional.push_back(argif{.type = formal_type_opt.value(), .is_defaulted = param.is_defaulted, .is_pack = param.is_pack});
        }

        if (is_member_functum && !formal_ensig.interface.named.contains("THIS"))
        {
            argif this_argif;

            if (is_ctor)
            {
                this_argif.type = nvalue_slot{.target = thistype_type};
            }
            else if (is_dtor)
            {
                this_argif.type = dvalue_slot{.target = thistype_type};
            }
            else
            {
                this_argif.type = ptrref_type{.target = thistype_type, .ptr_class = pointer_class::ref, .qual = qualifier::auto_};
            }

            formal_ensig.interface.named["THIS"] = this_argif;
        }

        if (is_ctor && formal_ensig.interface.named.contains("OTHER"))
        {
            auto const& other_type = formal_ensig.interface.named.at("OTHER").type;
            if (!is_ref(other_type) && other_type == class_type.value())
            {
                throw quxlang::semantic_compilation_error("Constructors cannot declare @OTHER with the same value type as the constructed class; use a reference form such as CONST& or TEMP& instead.");
            }
        }

        formal_ensig = strip_source_locations(std::move(formal_ensig));

        if (output.contains(formal_ensig))
        {
            throw quxlang::semantic_compilation_error("Duplicate overload");
        }

        output.insert({formal_ensig, i});
    }

    co_return output;
}
