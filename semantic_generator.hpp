//
// Created by Ryan Nicholl on 2/28/23.
//

#ifndef RPNX_RYANSCRIPT1031_SEMANTIC_GENERATOR_HEADER
#define RPNX_RYANSCRIPT1031_SEMANTIC_GENERATOR_HEADER

#include "ast.hpp"
#include "input_source.hpp"
#include "lir_type_index.hpp"
#include "sst.hpp"
#include "symbol_address.hpp"
#include <cassert>
#include <set>

namespace rs1031
{

    class semantic_generator
    {
        std::shared_ptr< sst_module > current_module;
        std::map< std::string, std::shared_ptr< sst_module > > imported_modules;

        sst_module_declaration& declaration(symbol_address const& where)
        {
            return current_module->stuff[where];
        }

        // std::multimap< sst_class*, sst_class* > unresolved_typedeps;

      public:
        inline semantic_generator()
        {
            current_module = std::make_shared< sst_module >();
        }
        void add(symbol_address const& where, ast_function const& function, input_source loc = input_source())
        {
            sst_module_declaration& decl = declaration(where);

            if (std::holds_alternative< sst_null_object >(decl.get()))
            {
                decl.get() = sst_function{};
            }
            else if (!std::holds_alternative< sst_function >(decl.get()))
            {
                throw std::runtime_error("Redeclaration of " + where.prettystring() + " as function where previous declaration in the same module was a non-function");
            }
            sst_procedure pr;
        }

        void add(symbol_address const& where, ast_class cl, input_source loc = input_source())
        {
            sst_module_declaration& decl = declaration(where);

            if (std::holds_alternative< sst_null_object >(decl.get()))
            {
                sst_class semantic_class_obj;
                semantic_class_obj.m_ast = std::move(cl);
                decl.get() = std::move(semantic_class_obj);
            }
            else
            {
                throw std::runtime_error("Redeclaration of CLASS " + where.prettystring() + " in the same module.");
            }
        }

        template < typename F >
        void for_all_class(F&& f)
        {
            for (auto& pair : current_module->stuff)
            {
                auto& decl = pair.second;
                auto& name = pair.first;
                if (std::holds_alternative< sst_class >(decl.get()))
                {
                    f(std::get< sst_class >(decl.get()), name);
                }
            }
        }

        void resolve_lir(lir_type_index& idx)
        {
            // Add all classes
            for_all_class(
                [&](sst_class& cl, symbol_address const& name)
                {
                    auto type_index_v = idx.add_type(name);
                    cl.m_lir_type_index = type_index_v;
                });

            // Add class members
            for_all_class(
                [&](sst_class& cl, symbol_address const&)
                {
                    for (sst_class_member& member : cl.members)
                    {
                        // if it's a member varaible
                        if (std::holds_alternative< sst_member_variable >(member.get()))
                        {
                            // get the variable
                            auto& var = std::get< sst_member_variable >(member.get());
                            // get the type of the variable

                            auto name = var.name;
                            auto type = var.type;

                            // Check if type is resolved

                            // 770-242-3344
                            // auto type_id = idx.add_or_get_type(type);
                        }
                    }
                });

            // Seal all classes
            for_all_class(
                [&](sst_class& cl, symbol_address const&)
                {
                    idx.seal_type(*cl.m_lir_type_index);
                });

            // Finalize all classes
            for_all_class(
                [&](sst_class& cl, symbol_address const&)
                {
                    idx.typeset_finalize(*cl.m_lir_type_index);
                    // print the name and size of the type
                    std::cout << "Type " << idx.pretty_name(cl.m_lir_type_index.value()) << " has size " << *idx.get_type_size(*cl.m_lir_type_index) << std::endl;
                    // print number of members
                });
        }
    };
} // namespace rs1031
#endif // RPNX_RYANSCRIPT1031_SEMANTIC_GENERATOR_HEADER
