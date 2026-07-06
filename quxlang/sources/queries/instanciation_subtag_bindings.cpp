// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/instanciation_subtag_bindings_spec.hpp>

rpnx::querygraph::coroutine< quxlang::instanciation_subtag_bindings_spec > quxlang::instanciation_subtag_bindings_impl(instanciation_reference input)
{
    std::map< std::string, parameter_instantiation > output;
    symbol_kind const instanciated_kind = co_await rpnx::querygraph::request< symbol_type_query >(input.temploid);
    if (instanciated_kind == symbol_kind::template_)
    {
        output = input.params.named;
    }

    temploid_instanciation_parameter_set const tempars = co_await rpnx::querygraph::request< instanciation_tempar_map_query >(input);
    for (std::map< std::string, type_symbol >::value_type const& binding : tempars.parameter_map)
    {
        std::string const& name = binding.first;
        type_symbol const& type = binding.second;
        if (!output.contains(name))
        {
            output[name] = make_type_instantiation(type);
        }
    }

    co_return output;
}
