// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_BENCHMARK_HEADER_GUARD
#define QUXLANG_DATA_BENCHMARK_HEADER_GUARD
#include <quxlang/data/basic_types.hpp>
namespace quxlang
{
    /** Describes one expanded benchmark case in a target-independent suite catalog. */
    struct benchmark_entry
    {
        std::string name;
        type_symbol procedure_symbol;
        std::vector< std::string > measurements;
        bool oneshot = false;
        RPNX_MEMBER_METADATA(benchmark_entry, name, procedure_symbol, measurements, oneshot);
    };
    /** Identifies compiler-owned benchmark metadata objects. */
    inline auto is_benchmark_object(type_symbol const& symbol) -> bool
    {
        if (!symbol.type_is< builtin_symbol >())
        {
            return false;
        }
        std::string const& name = symbol.get_as< builtin_symbol >().name;
        return name == "BENCHMARK_COUNT" || name == "BENCHMARK_NAMES" || name == "BENCHMARK_PROC" || name == "BENCHMARK_MEASUREMENT_NAMES" || name == "BENCHMARK_MEASUREMENT_OFFSETS" || name == "BENCHMARK_ONESHOT";
    }
    /** Returns the canonical procedure type shared by all benchmark suite cases. */
    inline auto benchmark_procedure_type() -> type_symbol
    {
        type_symbol state = ptrref_type{
            .target = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"}, .name = "BENCHMARK_STATE"},
            .ptr_class = pointer_class::ref,
            .qual = qualifier::mut,
        };
        return procedure_type{.signature = sigtype{.params = invotype{.named = {{"STATE", state}}}, .return_type = void_type{}}};
    }
} // namespace quxlang
#endif // QUXLANG_DATA_BENCHMARK_HEADER_GUARD
