//
// Created by Ryan Nicholl on 6/10/24.
//

#ifndef RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER
#define RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER

#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/res/vm_procedure2.hpp>
#include <rpnx/simple_coroutine.hpp>

namespace quxlang
{
    struct expr_test_provider
    {

        inline expr_test_provider()
        {
            auto type = quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ @(@THIS I32, I32)");
            this->testmap_instanciation_presets[as< instanciation_reference >(type)] = quxlang::instanciation_reference{.callee = quxlang::selection_reference{.callee = quxlang::parsers::parse_type_symbol("I32::.OPERATOR+"), .overload = function_overload{.builtin = true, .call_parameters = quxlang::call_type{.positional_parameters = {quxlang::parsers::parse_type_symbol("I32")}, .named_parameters = {{"THIS", quxlang::parsers::parse_type_symbol("I32")}}}}}, .parameters = quxlang::call_type{.positional_parameters = {quxlang::parsers::parse_type_symbol("I32")}, .named_parameters = {{"THIS", quxlang::parsers::parse_type_symbol("I32")}}}};
        }
        // Slot 0 is always reserved for the void slot
        // Including as data to avoid special-casing this.
        std::vector< vmir2::vm_slot > slots{vmir2::vm_slot{.type = void_type{}, .name = "VOID"}};

        std::vector< vmir2::vm_instruction > instructions;

        std::map< type_symbol, vmir2::storage_index > loadable_symbols;

        std::map< type_symbol, bool > testmap_functum_exists_presets{{quxlang::parsers::parse_type_symbol(std::string("I32::.OPERATOR+ @(@THIS I32, I32)")), true}};

        std::map< instanciation_reference, std::optional< instanciation_reference > > testmap_instanciation_presets{};
        std::map< type_symbol, type_symbol > testmap_functanoid_return_type_presets{};

        struct co_provider
        {
            expr_test_provider& parent;
            co_provider(expr_test_provider& parent) : parent(parent)
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

            rpnx::awaitable_result< bool > functum_exists_and_is_callable_with(instanciation_reference what)
            {
                std::cout << "functum_exists_and_is_callable_with(" << quxlang::to_string(what) << ")" << std::endl;

                auto it = parent.testmap_functum_exists_presets.find(what);
                if (it != parent.testmap_functum_exists_presets.end())
                {
                    return it->second;
                }
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< std::optional< instanciation_reference > > instanciation(instanciation_reference type)
            {
                std::cout << "instanciation(" << quxlang::to_string(type) << ")" << std::endl;
                auto it = parent.testmap_instanciation_presets.find(type);
                if (it != parent.testmap_instanciation_presets.end())
                {
                    auto result = it->second;
                    if (result)
                    {
                        std::cout << "instanciation(" << quxlang::to_string(type) << ") -> " << quxlang::to_string(result.value()) << std::endl;
                    }
                    return result;
                }
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< quxlang::type_symbol > functanoid_return_type(type_symbol type)
            {
                std::cout << "functanoid_return_type(" << quxlang::to_string(type) << ")" << std::endl;
                auto it = parent.testmap_functanoid_return_type_presets.find(type);
                if (it != parent.testmap_functanoid_return_type_presets.end())
                {
                    return it->second;
                }
                return std::make_exception_ptr(std::logic_error("Not implemented"));
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< vmir2::storage_index > create_temporary_storage(type_symbol type)
            {
                vmir2::storage_index idx = parent.slots.size();
                parent.slots.push_back(vmir2::vm_slot{.type = type, .name = "TEMP" + std::to_string(idx)});
                return idx;
            }

            rpnx::awaitable_result< void > emit_instruction(vmir2::vm_instruction instr)
            {
                parent.instructions.push_back(instr);
                return rpnx::awaitable_result< void >{};
            }

            rpnx::awaitable_result< vmir2::storage_index > create_numeric_literal(std::string value)
            {
                // TODO: Implement this
                rpnx::unimplemented();
                return 0;
            }

            rpnx::awaitable_result< quxlang::class_layout > class_layout_from_canonical_chain(type_symbol type)
            {
                throw rpnx::unimplemented();
            }
        };

        auto provider()
        {
            return co_provider{*this};
        }
    };

}; // namespace quxlang

#endif // RPNX_QUXLANG_EXPR_TEST_PROVIDER_HEADER
