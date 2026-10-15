// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/type_operation_is_noexcept_spec.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/keywords.hpp>

rpnx::querygraph::coroutine< quxlang::type_operation_is_noexcept_spec > quxlang::type_operation_is_noexcept_impl(expression_is_noexcept_operation input)
{
    symbol_kind kind = co_await rpnx::querygraph::request< symbol_type_query >(input.of_type);
    if (kind != symbol_kind::class_ && kind != symbol_kind::interface_)
    {
        co_return false;
    }
    bool swap = input.operation == noexcept_operation::swap;
    std::string member = "OPERATOR<->";
    if (!swap)
    {
        struct_tags_result_type const& tags = co_await rpnx::querygraph::request< struct_tags_query >(input.of_type);
        member = tags.contains(keywords::virtual_polymorphic) ? "FULLOBJECT_CONSTRUCTOR" : "CONSTRUCTOR";
    }
    invotype arguments;
    arguments.named["THIS"] = swap ? make_mref(input.of_type) : type_symbol(nvalue_slot{input.of_type});
    arguments.named["OTHER"] = swap ? make_mref(input.of_type) :
        input.operation == noexcept_operation::copy ? make_cref(input.of_type) : make_tref(input.of_type);
    std::optional< instanciation_reference > const& selected = co_await rpnx::querygraph::request< functum_initialize_query >(initialization_reference{
        .initializee = submember{.of = input.of_type, .name = member},
        .parameters = instatype_from_invotype(arguments),
        .adaptations = allowed_adaptations::destination_rebinding,
    });
    if (!selected.has_value())
    {
        co_return false;
    }
    co_return co_await rpnx::querygraph::request< functanoid_is_noexcept_query >(*selected);
}
