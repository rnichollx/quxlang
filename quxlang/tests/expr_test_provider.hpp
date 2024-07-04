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
            auto type = quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ #(@THIS I32, I32)");
            this->testmap_instanciation_presets[as< instanciation_reference >(type)] = quxlang::instanciation_reference{.callee = quxlang::selection_reference{.callee = quxlang::parsers::parse_type_symbol("I32::.OPERATOR+"), .overload = function_overload{.builtin = true, .call_parameters = quxlang::call_type{.positional_parameters = {quxlang::parsers::parse_type_symbol("I32")}, .named_parameters = {{"THIS", quxlang::parsers::parse_type_symbol("I32")}}}}}, .parameters = quxlang::call_type{.positional_parameters = {quxlang::parsers::parse_type_symbol("I32")}, .named_parameters = {{"THIS", quxlang::parsers::parse_type_symbol("I32")}}}};
        }
        // Slot 0 is always reserved for the void slot
        // Including as data to avoid special-casing this.
        std::vector< vmir2::vm_slot > slots{vmir2::vm_slot{.type = void_type{}, .name = "VOID"}};
        std::vector< bool > slot_alive{true}; // 0 slot is always alive/dead/doesn't matter

        std::vector< vmir2::vm_instruction > instructions;

        struct loadable_symbol
        {
            type_symbol type;
            vmir2::storage_index index;
            bool is_mut;
        };

        std::map< type_symbol, loadable_symbol > loadable_symbols;

        std::map< type_symbol, bool > testmap_functum_exists_presets{{quxlang::parsers::parse_type_symbol(std::string("I32::.OPERATOR+ #(@THIS I32, I32)")), true}};

        std::map< instanciation_reference, std::optional< instanciation_reference > > testmap_instanciation_presets{

            {as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ #(@THIS MUT& I32, MUT& I32)")), std::optional< instanciation_reference >(as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ #[BUILTIN; @THIS I32, I32 ] #( @THIS I32, I32 )")))}, {as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #(@THIS NEW& I32, MUT& I32)")), as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #[BUILTIN; @THIS NEW& I32, CONST& I32 ] #( @THIS NEW& I32, CONST& I32 )"))}, {as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR- #(@THIS I32, NUMERIC_LITERAL)")), as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR- #[BUILTIN; @THIS I32, I32 ] #( @THIS I32, I32 )"))}, {as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #(@THIS NEW& I32, NUMERIC_LITERAL)")), as< instanciation_reference >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #[BUILTIN; @THIS NEW& I32, NUMERIC_LITERAL ] #( @THIS NEW& I32, NUMERIC_LITERAL )"))}

        };
        std::map< type_symbol, type_symbol > testmap_functanoid_return_type_presets{

        // clang-format off
        {quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ #[BUILTIN; @THIS I32, I32 ] #( @THIS I32, I32 )"), quxlang::parsers::parse_type_symbol("I32")},

        {quxlang::parsers::parse_type_symbol("I32::.OPERATOR- #[BUILTIN; @THIS I32, I32 ] #( @THIS I32, I32 )"), quxlang::parsers::parse_type_symbol("I32")},


        {quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #[BUILTIN; @THIS NEW& I32, CONST& I32 ] #( @THIS NEW& I32, CONST& I32 )"), quxlang::parsers::parse_type_symbol("VOID")},

        {quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR#[BUILTIN; @THIS NEW& I32, NUMERIC_LITERAL] #(@THIS NEW& I32, NUMERIC_LITERAL)"), quxlang::parsers::parse_type_symbol("VOID")}
            // clang-format on
        };

        struct co_provider
        {
            expr_test_provider& parent;
            co_provider(expr_test_provider& parent) : parent(parent)
            {
            }

            template < typename T >
            using co_type = rpnx::simple_coroutine< T >;

            vmir2::storage_index create_reference_internal(vmir2::storage_index index)
            {
                // This function is used to handle the case where we have an index and need to force it into a
                // reference type.
                // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
                // is encountered during an expression.
                auto ty = index_type(index).await_resume();

                std::vector< std::string > types;
                for (auto& slot : parent.slots)
                {
                    types.push_back(quxlang::to_string(slot.type));
                }

                if (is_ref(ty))
                {
                    // If this is already a reference, we don't need to modify the type, we can re-use
                    return index;
                }

                type_symbol new_type;

                assert(!typeis< nvalue_reference >(ty));

                if (slot_alive(index).await_resume())
                {
                    new_type = mvalue_reference{.target = ty};
                }
                else
                {
                    assert(!typeis< nvalue_reference >(ty));
                    new_type = nvalue_reference{.target = ty};
                }
                vmir2::storage_index temp = create_temporary_storage_internal(new_type);
                vmir2::make_reference ref;
                ref.value_index = index;
                ref.reference_index = temp;

                // TODO: make this not a hack
                emit_instruction(ref).await_resume();

                std::string slot_type = quxlang::to_string(parent.slots.at(temp).type);
                // this is kind of a hack, but we can presume this after the make_reference call.
                parent.slot_alive[temp] = true;

                return temp;
            }

            vmir2::storage_index create_reference_internal(vmir2::storage_index index, type_symbol new_type)
            {
                // This function is used to handle the case where we have an index and need to force it into a
                // reference type.
                // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
                // is encountered during an expression.
                auto ty = index_type(index).await_resume();

                std::vector< std::string > types;
                for (auto& slot : parent.slots)
                {
                    types.push_back(quxlang::to_string(slot.type));
                }

                vmir2::storage_index temp = create_temporary_storage_internal(new_type);
                vmir2::make_reference ref;
                ref.value_index = index;
                ref.reference_index = temp;

                // TODO: make this not a hack
                emit_instruction(ref).await_resume();

                std::string slot_type = quxlang::to_string(parent.slots.at(temp).type);
                // this is kind of a hack, but we can presume this after the make_reference call.
                parent.slot_alive[temp] = true;

                return temp;
            }

            rpnx::awaitable_result< std::optional< vmir2::storage_index > > lookup_symbol(type_symbol sym)
            {
                auto it = parent.loadable_symbols.find(sym);
                if (it != parent.loadable_symbols.end())
                {
                    auto sym = it->second;

                    if (is_ref(sym.type))
                    {
                        return rpnx::result< std::optional< vmir2::storage_index > >(sym.index);
                    }
                    else
                    {
                        return std::optional< vmir2::storage_index >(create_reference_internal(sym.index));
                    }
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
                    return parent.slots.at(idx).type;
                }
                else
                {
                    return std::make_exception_ptr(std::logic_error("Not found"));
                }
            }

            rpnx::awaitable_result< bool > slot_alive(vmir2::storage_index idx)
            {
                if (idx < parent.slot_alive.size())
                {
                    return rpnx::awaitable_result< bool >(parent.slot_alive[idx]);
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
                    std::cout << "functanoid_return_type(" << quxlang::to_string(type) << ") -> " << quxlang::to_string(it->second) << std::endl;
                    return it->second;
                }
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< vmir2::storage_index > create_temporary_storage(type_symbol type)
            {
                return create_temporary_storage_internal(type);
            }

            vmir2::storage_index create_temporary_storage_internal(type_symbol type)
            {
                vmir2::storage_index idx = parent.slots.size();
                parent.slots.push_back(vmir2::vm_slot{.type = type, .name = "TEMP" + std::to_string(idx)});
                parent.slot_alive.push_back(false);
                return idx;
            }

            rpnx::awaitable_result< void > emit_instruction(vmir2::vm_instruction instr)
            {
                parent.instructions.push_back(instr);
                return rpnx::awaitable_result< void >{};
            }

            rpnx::awaitable_result< vmir2::storage_index > implicit_cast_reference(vmir2::storage_index index, type_symbol target)
            {
                if (!is_ref_implicitly_convertible_by_syntax(index_type(index).await_resume(), target))
                {
                    throw std::logic_error("Cannot implicitly cast");
                }

                auto ref = create_reference_internal(index, target);

                return ref;
            }

            rpnx::awaitable_result< void > emit_invoke(type_symbol what, vmir2::invocation_args args)
            {

                std::cout << "emit_invoke(" << quxlang::to_string(what) << ")"
                          << " " << quxlang::to_string(args) << std::endl;
                vmir2::invoke ivk;
                ivk.what = what;
                ivk.args = args;
                emit_instruction(ivk).await_resume();

                instanciation_reference inst = as< instanciation_reference >(what);

                for (auto& arg : args.positional)
                {
                    type_symbol arg_type = index_type(arg).await_resume();
                    if (typeis< nvalue_reference >(arg_type))
                    {
                        if (parent.slot_alive.at(arg))
                        {
                            throw std::logic_error("Cannot invoke a functanoid with a NEW& parameter on a live slot.");
                        }
                        parent.slot_alive.at(arg) = true;
                    }

                    else if (typeis< dvalue_slot >(arg_type))
                    {
                        if (!parent.slot_alive.at(arg))
                        {
                            throw std::logic_error("Cannot invoke a functanoid with a DESTROY& parameter on a dead slot.");
                        }
                        parent.slot_alive.at(arg) = false;
                    }
                    else
                    {
                        if (!parent.slot_alive.at(arg))
                        {
                            throw std::logic_error("Cannot invoke a functanoid with a parameter on a dead slot.");
                        }

                        if (!is_ref(arg_type))
                        {
                            // In quxlang calling convention, callee is responsible for destroying the argument.
                            parent.slot_alive.at(arg) = false;
                        }

                        // however, references are not destroyed when passed as arguments.
                    }
                }
                for (auto& [name, arg] : args.named)
                {
                    type_symbol arg_type = index_type(arg).await_resume();
                    if (name == "RETURN")
                    {
                        parent.slot_alive.at(arg) = true;
                        continue;
                    }
                    type_symbol parameter_type = inst.parameters.named_parameters.at(name);

                    if (typeis< nvalue_reference >(parameter_type))
                    {
                        if (parent.slot_alive.at(arg))
                        {
                            throw std::logic_error("Cannot invoke a functanoid with a NEW& parameter on a live slot.");
                        }
                        std::cout << "Setting slot " << arg << " alive" << std::endl;
                        parent.slot_alive.at(arg) = true;
                    }

                    else if (typeis< dvalue_slot >(parameter_type))
                    {
                        if (!parent.slot_alive.at(arg))
                        {
                            throw std::logic_error("Cannot invoke a functanoid with a DESTROY& parameter on a dead slot.");
                        }
                        parent.slot_alive.at(arg) = false;
                    }
                    else
                    {
                        if (!parent.slot_alive.at(arg))
                        {
                            throw std::logic_error("Cannot invoke a functanoid with a parameter on a dead slot.");
                        }

                        if (!is_ref(arg_type))
                        {
                            // In quxlang calling convention, callee is responsible for destroying the argument.
                            // however, references are not destroyed when passed as arguments.
                            std::cout << "Setting slot " << arg << " dead" << std::endl;
                            parent.slot_alive.at(arg) = false;
                        }
                    }
                }

                return {};
            }

            rpnx::awaitable_result< vmir2::storage_index > create_numeric_literal(std::string value)
            {
                vmir2::storage_index idx = parent.slots.size();
                parent.slots.push_back(vmir2::vm_slot{.type = numeric_literal_reference{}, .name = "LITERAL" + std::to_string(idx), .literal_value = value});
                parent.slot_alive.push_back(true);
                return idx;
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
