// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_TESTS_EXPR_TEST_PROVIDER_HEADER_GUARD
#define QUXLANG_TESTS_EXPR_TEST_PROVIDER_HEADER_GUARD

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
            this->testmap_instanciation_presets[as< instantiation_type >(type)] = quxlang::instantiation_type{.callee = quxlang::selection_reference{.templexoid = quxlang::parsers::parse_type_symbol("I32::.OPERATOR+"), .overload = function_overload{.builtin = true, .call_parameters = quxlang::calltype{.named = {{"THIS", quxlang::parsers::parse_type_symbol("I32")}}, .positional = {quxlang::parsers::parse_type_symbol("I32")}}}}, .parameters = quxlang::calltype{.named = {{"THIS", quxlang::parsers::parse_type_symbol("I32")}}, .positional = {quxlang::parsers::parse_type_symbol("I32")}, }};
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

        std::map< type_symbol, bool > testmap_functum_exists_presets{{quxlang::parsers::parse_type_symbol(std::string("I32::.OPERATOR+ #(@THIS I32, @OTHER I32)")), true}};

        std::map< instantiation_type, std::optional< instantiation_type > > testmap_instanciation_presets{

            {as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ #(@THIS MUT& I32, @OTHER MUT& I32)")), std::optional< instantiation_type >(as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ #{BUILTIN; @THIS I32, @OTHER I32 }")))}, {as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #(@THIS NEW& I32, @OTHER MUT& I32)")), as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #[BUILTIN; @THIS NEW& I32, @OTHER CONST& I32 ] #( @THIS NEW& I32, @OTHER CONST& I32 )"))}, {as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR- #(@THIS I32, @OTHER NUMERIC_LITERAL)")), as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.OPERATOR- #[BUILTIN; @THIS I32, @OTHER I32 ] #( @THIS I32, @OTHER I32 )"))}, {as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #( @THIS NEW& I32, @OTHER NUMERIC_LITERAL)")), as< instantiation_type >(quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #[ BUILTIN; @THIS NEW& I32, @OTHER NUMERIC_LITERAL ] #( @THIS NEW& I32, @OTHER NUMERIC_LITERAL )"))}

        };


        std::map< type_symbol, symbol_kind > testmap_symbol_type_presets{
                {quxlang::parsers::parse_type_symbol("I32"), symbol_kind::builtin_class},
        };
        std::map< type_symbol, type_symbol > testmap_functanoid_return_type_presets{

        // clang-format off
        {quxlang::parsers::parse_type_symbol("I32::.OPERATOR+ #[BUILTIN; @THIS I32, @OTHER I32 ] #( @THIS I32, @OTHER I32 )"), quxlang::parsers::parse_type_symbol("I32")},

        {quxlang::parsers::parse_type_symbol("I32::.OPERATOR- #[BUILTIN; @THIS I32, @OTHER I32 ] #( @THIS I32, @OTHER I32 )"), quxlang::parsers::parse_type_symbol("I32")},


        {quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR #[BUILTIN; @THIS NEW& I32, @OTHER CONST& I32 ] #( @THIS NEW& I32, @OTHER CONST& I32 )"), quxlang::parsers::parse_type_symbol("VOID")},

        {quxlang::parsers::parse_type_symbol("I32::.CONSTRUCTOR#[BUILTIN; @THIS NEW& I32, @OTHER NUMERIC_LITERAL] #(@THIS NEW& I32, @OTHER NUMERIC_LITERAL)"), quxlang::parsers::parse_type_symbol("VOID")}
            // clang-format on
        };


        static inline auto quxtype(std::string val) -> type_symbol { return quxlang::parsers::parse_type_symbol(val);}

        std::map< implicitly_convertible_to_query, bool > testmap_implicitly_convertible_to_presets {
            { implicitly_convertible_to_query{.from = quxtype("MUT& I32"), .to=quxtype("CONST& I32")}, true }
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

                assert(!typeis< nvalue_slot >(ty));

                if (slot_alive(index).await_resume())
                {
                    new_type = mvalue_reference{.target = ty};
                }
                else
                {
                    assert(!typeis< nvalue_slot >(ty));
                    new_type = nvalue_slot{.target = ty};
                }
                vmir2::storage_index temp = create_temporary_storage_internal(new_type);
                vmir2::make_reference ref;
                ref.value_index = index;
                ref.reference_index = temp;

                // TODO: make this not a hack
                parent.instructions.push_back(ref);

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

                parent.instructions.push_back(ref);

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
                    if (parent.slot_alive.at(idx))
                    {
                        return parent.slots.at(idx).type;
                    }
                    else
                    {
                        return create_nslot(parent.slots.at(idx).type);
                    }

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

            rpnx::awaitable_result< bool > functum_exists_and_is_callable_with(instantiation_type what)
            {
                std::cout << "functum_exists_and_is_callable_with(" << quxlang::to_string(what) << ")" << std::endl;

                auto it = parent.testmap_functum_exists_presets.find(what);
                if (it != parent.testmap_functum_exists_presets.end())
                {
                    return it->second;
                }
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< vmir2::storage_index > create_binding( vmir2::storage_index index, type_symbol bound_symbol)
            {
                return std::make_exception_ptr(std::logic_error("Not implemented"));
            }

            rpnx::awaitable_result< std::optional< instantiation_type > > instanciation(instantiation_type type)
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

            rpnx::awaitable_result< vmir2::storage_index > index_binding(vmir2::storage_index index)
            {
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

            rpnx::awaitable_result< quxlang::symbol_kind > symbol_type(type_symbol type)
            {
                std::cout << "symbol_type(" << quxlang::to_string(type) << ")" << std::endl;
                auto it = parent.testmap_symbol_type_presets.find(type);
                if (it != parent.testmap_symbol_type_presets.end())
                {
                    std::cout << "symbol_type(" << quxlang::to_string(type) << ") -> " << int(it->second) << std::endl;
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

            rpnx::awaitable_result<void> emit_cast_reference(vmir2::cast_reference cst)
            {
               parent.slot_alive.at(cst.target_ref_index) = true;
               return rpnx::awaitable_result<void>();
            }

            rpnx::awaitable_result< bool > implicitly_convertible_to(type_symbol from, type_symbol to)
            {
                 std::cout << "implicitly_convertible_to(" << quxlang::to_string(from) << ", " << quxlang::to_string(to) << ")" << std::endl;
                auto it = parent.testmap_implicitly_convertible_to_presets.find(implicitly_convertible_to_query{.from = from, .to = to});
                if (it != parent.testmap_implicitly_convertible_to_presets.end())
                {
                    return it->second;
                }
                return std::make_exception_ptr(std::logic_error("Not implemented"));

                 throw rpnx::unimplemented();
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

            rpnx::awaitable_result<void> emit(vmir2::access_field af)
            {
                parent.instructions.push_back(af);
                parent.slot_alive.at(af.store_index) = true;
                return {};
            }

            rpnx::awaitable_result<void> emit(vmir2::invoke ivk)
            {

                std::cout << "emit_invoke(" << quxlang::to_string(ivk.what) << ")"
                          << " " << quxlang::to_string(ivk.args) << std::endl;

                parent.instructions.push_back(ivk);

                instantiation_type inst = as< instantiation_type >(ivk.what);

                for (auto& arg : ivk.args.positional)
                {
                    type_symbol arg_type = index_type(arg).await_resume();
                    if (typeis< nvalue_slot >(arg_type))
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
                for (auto& [name, arg] : ivk.args.named)
                {
                    type_symbol arg_type = index_type(arg).await_resume();
                    if (name == "RETURN")
                    {
                        parent.slot_alive.at(arg) = true;
                        continue;
                    }
                    type_symbol parameter_type = inst.parameters.named.at(name);

                    if (typeis< nvalue_slot >(parameter_type))
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

            rpnx::awaitable_result< quxlang::class_layout > class_layout(type_symbol type)
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
