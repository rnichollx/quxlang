//
// Created by Ryan Nicholl on 2/28/23.
//

#ifndef RPNX_RYANSCRIPT1031_SEMANTIC_GENERATOR_HEADER
#define RPNX_RYANSCRIPT1031_SEMANTIC_GENERATOR_HEADER

#include "ast.hpp"
#include "input_source.hpp"
#include "lir_type_index.hpp"
#include "lookup_sequence.hpp"
#include "sst.hpp"
#include <cassert>
#include <set>

namespace rs1031
{

    class semantic_generator
    {
        std::shared_ptr< sst_module > current_module;
        std::map< std::string, std::shared_ptr< sst_module > > imported_modules;

        sst_module_declaration& declaration(static_lookup_sequence const& where)
        {
            return current_module->stuff[where];
        }

        // std::multimap< sst_class*, sst_class* > unresolved_typedeps;

      public:
        inline semantic_generator()
        {
            current_module = std::make_shared< sst_module >();
        }
        void add(static_lookup_sequence const& where, ast_function const& function, input_source loc = input_source())
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

        void add(static_lookup_sequence const& where, ast_class cl, input_source loc = input_source())
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
    };
} // namespace rs1031
#endif // RPNX_RYANSCRIPT1031_SEMANTIC_GENERATOR_HEADER
