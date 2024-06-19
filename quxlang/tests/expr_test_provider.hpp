//
// Created by Ryan Nicholl on 6/10/24.
//

#ifndef RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER
#define RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER

#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/res/vm_procedure2.hpp>
#include <rpnx/simple_coroutine.hpp>

namespace quxlang
{
    struct expr_test_provider
    {
        // Slot 0 is always reserved for the void slot
        // Including as data to avoid special-casing this.
        std::vector< vmir2::vm_slot > slots{vmir2::vm_slot{.type = void_type{}, .name = "VOID"}};

        std::vector< vmir2::vm_instruction > instructions;

        std::map< type_symbol, vmir2::storage_index > loadable_symbols;

        struct co_provider
        {
            expr_test_provider& parent;
            co_provider(expr_test_provider& parent)
                : parent(parent)
            {
            }

            template < typename T >
            using co_type = rpnx::simple_coroutine< T >;

            rpnx::awaitable_result< std::optional< vmir2::storage_index > > lookup_symbol(type_symbol sym)
            {
                auto it = parent.loadable_symbols.find(sym);
                if (it != parent.loadable_symbols.end())
                {
                    return rpnx::result< std::optional< vmir2::storage_index > >(it->second);
                }
                else
                {
                    throw std::logic_error("Not found");
                }
            }

            rpnx::awaitable_result< quxlang::type_symbol > index_type(vmir2::storage_index idx)
            {
                if (idx < parent.slots.size())
                {
                    return parent.slots[idx].type;
                }
                else
                {
                    return std::make_exception_ptr(std::logic_error("Not found"));
                }
            }

            rpnx::awaitable_result< bool > functum_exists_and_is_callable_with(instanciation_reference)
            {
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< std::optional< instanciation_reference > > instanciation(type_symbol type)
            {
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< quxlang::type_symbol > functanoid_return_type(instanciation_reference)
            {
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< vmir2::storage_index > create_temporary_storage(type_symbol type)
            {
                vmir2::storage_index idx = parent.slots.size();
                parent.slots.push_back(vmir2::vm_slot{.type = type, .name = "TEMP" + std::to_string(idx)});
                return idx;
            }
        };

        auto provider()
        {
            return co_provider{*this};
        }
    };

}; // namespace quxlang

#endif // RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER
