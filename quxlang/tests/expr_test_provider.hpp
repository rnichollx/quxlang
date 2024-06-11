//
// Created by Ryan Nicholl on 6/10/24.
//

#ifndef RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER
#define RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER

#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/res/vm_procedure2.hpp>

namespace quxlang
{
    struct expr_test_provider
    {
        // Slot 0 is always reserved for the void slot
        // Including as data to avoid special-casing this.
        std::vector< vmir2::vm_slot > slots{vmir2::vm_slot{.type = void_type{}, .name = "VOID"}};

        std::vector< vmir2::vm_instruction > instructions;

        std::map<type_symbol, vmir2::storage_index> loadable_symbols;

        struct interface : public co_vmir_expression_emitter::co_interface
        {
            expr_test_provider* gen;

          public:
            interface(expr_test_provider* gen)
                : gen(gen)
            {
            }

            virtual QUX_SUBCO_MEMBER_FUNC(create_temporary_storage, vmir2::storage_index, (type_symbol type)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(lookup_symbol, std::optional< vmir2::storage_index >, (type_symbol sym)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(emit, void, (vmir2::vm_instruction)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(index_type, type_symbol, (vmir2::storage_index)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(create_string_literal, vmir2::storage_index, (std::string)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(create_numeric_literal, vmir2::storage_index, (std::string)) override;
        };
    };

}; // namespace quxlang

#endif // RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER
