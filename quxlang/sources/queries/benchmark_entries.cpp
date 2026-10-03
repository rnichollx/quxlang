// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/benchmark_entries_spec.hpp>
rpnx::querygraph::coroutine< quxlang::benchmark_entries_spec > quxlang::benchmark_entries_impl(std::vector< std::string > input)
{
    std::set< type_symbol > symbols;
    for (std::string const& module : input)
    {
        std::set< type_symbol > const& found = co_await rpnx::querygraph::request< list_benchmarks_query >(absolute_module_reference{.module_name = module});
        symbols.insert(found.begin(), found.end());
    }
    std::vector< benchmark_entry > result;
    for (type_symbol const& symbol : symbols)
    {
        ast2_symboid const& ast = co_await rpnx::querygraph::request< symboid_query >(symbol);
        ast2_benchmark const& declaration = ast.get_as< ast2_benchmark >();
        if (declaration.cases.empty() && !declaration.parameters.empty())
        {
            throw semantic_compilation_error("Parameterized suite benchmark requires CASE entries: " + to_string(symbol));
        }
        std::size_t count = declaration.cases.empty() ? 1 : declaration.cases.size();
        for (std::size_t index = 0; index < count; ++index)
        {
            type_symbol function = subsymbol{.of = symbol, .name = "__BENCHMARK_CASE_" + std::to_string(index)};
            std::optional< instanciation_reference > procedure = co_await rpnx::querygraph::request< instanciation_query >(initialization_reference{.initializee = function, .parameters = instatype_from_invotype(benchmark_procedure_type().get_as< procedure_type >().signature.params)});
            if (!procedure)
            {
                throw semantic_compilation_error("Could not instantiate benchmark case: " + to_string(symbol));
            }
            result.push_back(benchmark_entry{
                .name = to_string(symbol) + (declaration.cases.empty() ? "" : "/" + declaration.cases[index].name),
                .procedure_symbol = *procedure,
                .measurements = declaration.measurements,
                .oneshot = declaration.oneshot,
            });
        }
    }
    co_return result;
}
