// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/functanoid_is_noexcept_spec.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/keywords.hpp>

rpnx::querygraph::coroutine< quxlang::functanoid_is_noexcept_spec > quxlang::functanoid_is_noexcept_impl(instanciation_reference input)
{
    temploid_reference const& function = input.temploid;
    bool is_destructor = false;
    if (typeis< submember >(function.templexoid))
    {
        std::string const& name = as< submember >(function.templexoid).name;
        if (name == "DESTRUCTOR" || name == "FULLOBJECT_DESTRUCTOR" || name == "SUBOBJECT_DESTRUCTOR")
        {
            is_destructor = true;
        }
    }
    std::optional< ast2_function_declaration > const& declaration = co_await rpnx::querygraph::request< function_declaration_query >(function);
    if (declaration.has_value())
    {
        if (declaration->header.noexcept_if.has_value())
        {
            co_return co_await rpnx::querygraph::request< constexpr_bool_query >(constexpr_input{
                .expr = *declaration->header.noexcept_if,
                .context = input,
            });
        }
        co_return declaration->header.is_noexcept || is_destructor;
    }
    if (is_destructor)
    {
        type_symbol const& owner = as< submember >(function.templexoid).of;
        if (!(co_await rpnx::querygraph::request< user_default_dtor_exists_query >(owner)))
        {
            co_return true;
        }
        std::optional< instanciation_reference > const& body = co_await rpnx::querygraph::request< functum_initialize_query >(initialization_reference{
            .initializee = submember{.of = owner, .name = "DESTRUCTOR"},
            .parameters = instatype_from_invotype(invotype{.named = {{"THIS", make_mref(owner)}}}),
            .adaptations = allowed_adaptations::none,
        });
        co_return !body.has_value() || (co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(*body));
    }
    if (!typeis< submember >(function.templexoid))
    {
        co_return false;
    }
    submember const& member = as< submember >(function.templexoid);
    if (!keywords::is_constructor_name(member.name) && member.name != "OPERATOR<->")
    {
        co_return false;
    }
    builtin_function_kind kind = co_await rpnx::querygraph::request< function_builtin_query >(function);
    if (kind == builtin_function_kind::builtin_intrinsic)
    {
        co_return true;
    }
    if (kind == builtin_function_kind::not_builtin)
    {
        co_return false;
    }
    try
    {
        vmir2::functanoid_routine3 const& routine = co_await rpnx::querygraph::request< builtin_vm_procedure3_query >(input);
        for (std::pair< type_symbol const, type_symbol > const& destructor : routine.non_trivial_dtors)
        {
            if (!(co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(as< instanciation_reference >(destructor.second))))
            {
                co_return false;
            }
        }
        for (vmir2::executable_block const& block : routine.blocks)
        {
            if (block.terminator.has_value() && block.terminator->type_is< vmir2::throw_exception >())
            {
                co_return false;
            }
            for (vmir2::vm_instruction const& instruction : block.instructions)
            {
                if (instruction.type_is< vmir2::invoke >())
                {
                    if (!(co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(as< instanciation_reference >(instruction.as< vmir2::invoke >().what))))
                    {
                        co_return false;
                    }
                }
                else if (instruction.type_is< vmir2::invoke_indirect >() || instruction.type_is< vmir2::invoke_virtual >() ||
                    instruction.type_is< vmir2::interface_invoke >() || instruction.type_is< vmir2::jvm_allocate_object_storage >() ||
                    instruction.type_is< vmir2::jvm_allocate_multiple_object_storage >())
                {
                    co_return false;
                }
            }
        }
    }
    catch (compilation_error const& error)
    {
        if (!error.structured_error.type_is< semantic_error >())
        {
            throw;
        }
        // A generated operation is unavailable when a required suboperation is ill-formed.
        co_return false;
    }
    co_return true;
}
