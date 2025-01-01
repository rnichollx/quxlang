// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_COMPILER_HEADER_GUARD
#define QUXLANG_COMPILER_HEADER_GUARD

#include "data/target_configuration.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/function_frame_information.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/res/constexpr.hpp"
#include "quxlang/data/vm_procedure.hpp"
#include "quxlang/filelist.hpp"
#include "quxlang/res/constructor.hpp"
#include "quxlang/res/function.hpp"
#include "quxlang/res/functanoid.hpp"
#include "quxlang/res/call_params_of_function_ast_resolver.hpp"
#include "quxlang/res/called_functanoids_resolver.hpp"
#include "quxlang/res/canonical_symbol_from_contextual_symbol_resolver.hpp"
#include "quxlang/res/class_field_list_resolver.hpp"
#include "quxlang/res/class_layout_resolver.hpp"
#include "quxlang/res/class_should_autogen_default_constructor_resolver.hpp"
#include "quxlang/res/class_size_from_canonical_chain_resolver.hpp"
#include "quxlang/res/contextualized_reference_resolver.hpp"
#include "quxlang/res/entity_ast_from_canonical_chain_resolver.hpp"
#include "quxlang/res/entity_canonical_chain_exists_resolver.hpp"
#include "quxlang/res/expr_ir2.hpp"
#include "quxlang/res/file_ast_resolver.hpp"
#include "quxlang/res/file_module_map_resolver.hpp"
#include "quxlang/res/filelist_resolver.hpp"
#include "quxlang/res/files_in_module_resolver.hpp"
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/res/list_builtin_functum_overloads_resolver.hpp"
#include "quxlang/res/module_ast_resolver.hpp"
#include "quxlang/res/overload_set_instanciate_with_resolver.hpp"
#include "quxlang/res/overloads.hpp"
#include "quxlang/res/symbol_canonical_chain_exists_resolver.hpp"
#include "quxlang/res/type_placement_info_from_canonical_type_resolver.hpp"
#include "quxlang/res/type_size_from_canonical_type_resolver.hpp"
#include "quxlang/res/variable.hpp"
#include "quxlang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"
#include <mutex>
#include <quxlang/res/asm_procedure_from_symbol_resolver.hpp>
#include <quxlang/res/declaroids_resolver.hpp>
#include <quxlang/res/extern_linksymbol_resolver.hpp>
#include <quxlang/res/functum_instanciation.hpp>
#include <quxlang/res/functum_select_function.hpp>
#include <quxlang/res/functum_exists_and_is_callable_with_resolver.hpp>
#include <quxlang/res/instanciation.hpp>
#include <quxlang/res/interpret_bool_resolver.hpp>
#include <quxlang/res/interpret_value_resolver.hpp>
#include <quxlang/res/lookup.hpp>
#include <quxlang/res/module_source_name_resolver.hpp>
#include <quxlang/res/module_sources_resolver.hpp>
#include <quxlang/res/procedure_linksymbol_resolver.hpp>
#include <quxlang/res/symboid_resolver.hpp>
#include <quxlang/res/symboid_subdeclaroids.hpp>
#include <quxlang/res/symbol_type.hpp>
#include <quxlang/res/template_instanciation.hpp>
#include <quxlang/res/template_instanciation_ast_resolver.hpp>
#include <quxlang/res/template_instanciation_parameter_set_resolver.hpp>
#include <quxlang/res/templex_select_template.hpp>
#include <quxlang/res/temploid_instanciation_parameter_set_resolver.hpp>
#include <quxlang/res/vm_procedure2.hpp>
#include <shared_mutex>

// clang-format off
#define COMPILER_INDEX(x) friend class x ## _resolver; private: index < x ## _resolver > m_ ## x ## _index; x ## _resolver::outptr_type lk_ ## x ( x ## _resolver::input_type const & input ) { return this->m_ ## x ## _index.lookup(input); } public: auto get_ ## x ( x ## _resolver::input_type const & input ) { auto node = lk_ ## x (input); m_solver.solve(this, node); return node->get(); }
// clang-format on

namespace quxlang
{
    class compiler_binder;
    class compiler
    {
        friend class compiler_binder;
        friend class filelist_resolver;
        friend class class_list_resolver;
        friend class file_ast_resolver;
        friend class class_field_list_resolver;
        friend class entity_ast_from_chain_resolver;
        friend class module_ast_precursor1_resolver;
        friend class class_size_from_canonical_chain_resolver;
        friend class module_ast_resolver;
        friend class canonical_symbol_from_contextual_symbol_resolver;
        friend class type_size_from_canonical_type_resolver;
        friend class class_layout_resolver;
        friend class class_placement_info_from_cannonical_chain_resolver;
        friend class type_placement_info_from_canonical_type_resolver;
        friend class entity_canonical_chain_exists_resolver;
        friend class llvm_code_generator;
        friend class implicitly_convertible_to_resolver;
        friend class overload_set_is_callable_with_resolver;
        friend class function_overload_selection_resolver;
        friend class function_qualified_reference_resolver;
        friend class contextualized_reference_resolver;
        friend class vm_procedure_from_canonical_functanoid_resolver;
        friend class function_frame_information_resolver;
        friend class operator_is_overloaded_with_resolver;
        friend class symbol_canonical_chain_exists_resolver;
        friend class class_should_autogen_default_constructor_resolver;
        friend class functum_exists_and_is_callable_with_resolver;
        friend class list_functum_overloads_resolver;
        friend class functanoid_return_type_resolver;
        friend class called_functanoids_resolver;
        friend class list_builtin_functum_overloads_resolver;
        friend class call_params_of_function_ast_resolver;
        friend class overload_set_instanciate_with_resolver;
        friend class type_map_resolver;
        friend class temploid_instanciation_ast_resolver;
        friend class template_instanciation_parameter_set_resolver;
        friend class template_instanciation_ast_resolver;
        friend class temploid_instanciation_parameter_set_resolver;
        friend class functum_instanciation_parameter_map_resolver;
        //friend class co_vmir_expression_emitter;

        template < typename G >
        friend auto type_size_from_canonical_type_question_f(G* g, type_symbol type) -> rpnx::resolver_coroutine< G, std::size_t >;

        template < typename G >
        friend auto type_placement_info_from_canonical_type_question_f(G* g, type_symbol type) -> rpnx::resolver_coroutine< G, type_placement_info >;

        template < typename T >
        using index = rpnx::index< compiler, T >;

        template < typename T >
        using singleton = rpnx::singleton< compiler, T >;

      public:
        template < typename T >
        using out = rpnx::output_ptr< compiler, T >;

      private:
        filelist m_file_list;
        singleton< filelist_resolver > m_filelist_resolver;
        // class_list_resolver m_class_list_resolver;
        //index< file_content_resolver > m_file_contents_index;
        index< file_ast_resolver > m_file_ast_index;

        COMPILER_INDEX(asm_procedure_from_symbol)
        COMPILER_INDEX(constexpr_bool)
        COMPILER_INDEX(canonical_symbol_from_contextual_symbol)
        COMPILER_INDEX(class_layout)
        COMPILER_INDEX(class_field_list)
        COMPILER_INDEX(declaroids)
        COMPILER_INDEX(extern_linksymbol)
        COMPILER_INDEX(expr_ir2)
        COMPILER_INDEX(exists)
        COMPILER_INDEX(functanoid_parameter_map)
        COMPILER_INDEX(functanoid_return_type)
        COMPILER_INDEX(functanoid_param_names)
        COMPILER_INDEX(functanoid_sigtype)
        COMPILER_INDEX(function_positional_parameter_names)
        COMPILER_INDEX(function_builtin)
        COMPILER_INDEX(functum_exists_and_is_callable_with)
        COMPILER_INDEX(functum_instanciation)
        COMPILER_INDEX(functum_select_function)
        COMPILER_INDEX(function_declaration)
        COMPILER_INDEX(function_instanciation)
        COMPILER_INDEX(class_field_declaration_list)
        COMPILER_INDEX(interpret_bool)
        COMPILER_INDEX(interpret_value)
        COMPILER_INDEX(list_builtin_functum_overloads)
        COMPILER_INDEX(list_builtin_constructors)
        COMPILER_INDEX(list_functum_overloads)
        COMPILER_INDEX(list_user_functum_overloads)
        COMPILER_INDEX(list_user_functum_overload_declarations)
        COMPILER_INDEX(list_user_functum_formal_paratypes)
        COMPILER_INDEX(lookup);
        COMPILER_INDEX(module_ast)
        COMPILER_INDEX(module_source_name)
        COMPILER_INDEX(nontrivial_default_dtor)
        COMPILER_INDEX(overload_set_instanciate_with)
        COMPILER_INDEX(procedure_linksymbol)
        COMPILER_INDEX(symbol_type)
        COMPILER_INDEX(symboid)
        COMPILER_INDEX(symboid_subdeclaroids)
        COMPILER_INDEX(template_instanciation)
        COMPILER_INDEX(template_instanciation_ast)
        //COMPILER_INDEX(templex_instanciation)
        COMPILER_INDEX(templex_select_template)
        COMPILER_INDEX(instanciation)
        COMPILER_INDEX(template_instanciation_parameter_set)
        COMPILER_INDEX(temploid_instanciation_parameter_set)
        COMPILER_INDEX(variable_type)
        COMPILER_INDEX(vm_procedure_from_canonical_functanoid)
        COMPILER_INDEX(vm_procedure2)
        COMPILER_INDEX(user_vm_procedure2)
        COMPILER_INDEX(builtin_vm_procedure2)
        COMPILER_INDEX(builtin_ctor_vm_procedure2)
        COMPILER_INDEX(builtin_dtor_vm_procedure2)
        COMPILER_INDEX(type_placement_info_from_canonical_type)

        index< called_functanoids_resolver > m_called_functanoids_index;

        out< std::set< type_symbol > > lk_called_functanoids(type_symbol func_addr)
        {
            return m_called_functanoids_index.lookup(func_addr);
        }

        index< class_should_autogen_default_constructor_resolver > m_class_should_autogen_default_constructor_index;

        out< bool > lk_class_should_autogen_default_constructor(type_symbol const& cls)
        {
            return m_class_should_autogen_default_constructor_index.lookup(cls); //
        }

        index< symbol_canonical_chain_exists_resolver > m_symbol_canonical_chain_exists_index;

        out< bool > lk_symbol_canonical_chain_exists(type_symbol chain)
        {
            return m_symbol_canonical_chain_exists_index.lookup(chain);
        }

      public:
        vm_procedure get_vm_procedure_from_canonical_functanoid(initialization_reference func_addr)
        {
            auto node = lk_vm_procedure_from_canonical_functanoid(func_addr);
            m_solver.solve(this, node);
            return node->get();
        }

        vmir2::functanoid_routine2 get_vm_procedure2(initialization_reference func_addr)
        {
            auto node = lk_vm_procedure2(func_addr);
            m_solver.solve(this, node);
            return node->get();
        }



        asm_procedure get_asm_procedure_from_canonical_symbol(type_symbol func_addr)
        {
            auto node = lk_asm_procedure_from_symbol(func_addr);
            m_solver.solve(this, node);
            return node->get();
        }

      private:
        index< contextualized_reference_resolver > m_contextualized_reference_index;

        out< type_symbol > lk_contextualized_reference(type_symbol symbol, type_symbol context)
        {
            return m_contextualized_reference_index.lookup({symbol, context});
        }

      public:
      private:
        // index< class_list_resolver > m_class_list_index;
        
          COMPILER_INDEX(implicitly_convertible_to);


        auto lk_implicitly_convertible_to(type_symbol from, type_symbol to)
        {
            return m_implicitly_convertible_to_index.lookup(implicitly_convertible_to_query{from, to});
        }

  

        index< entity_canonical_chain_exists_resolver > m_entity_canonical_chain_exists_index;

        out< bool > lk_entity_canonical_chain_exists(type_symbol const& chain)
        {
            return m_entity_canonical_chain_exists_index.lookup(chain);
        }

      public:

      private:






        index< class_size_from_canonical_chain_resolver > m_class_size_from_canonical_chain_index;

        out< std::size_t > lk_class_size_from_canonical_lookup_chain(type_symbol const& chain)
        {
            return m_class_size_from_canonical_chain_index.lookup(chain);
        }

        COMPILER_INDEX(module_sources);

        out< type_symbol > lk_canonical_symbol_from_contextual_symbol(type_symbol type, type_symbol context)
        {
            return m_canonical_symbol_from_contextual_symbol_index.lookup(contextual_type_reference{.context = context, .type = type});
        }

        index< type_size_from_canonical_type_resolver > m_type_size_from_canonical_type_index;

        out< std::size_t > lk_type_size_from_canonical_type(type_symbol const& ref)
        {
            return m_type_size_from_canonical_type_index.lookup(ref);
        }

        output_info m_output_info;
        rpnx::single_thread_graph_solver< compiler > m_solver;
        std::shared_mutex m_mutex;
        std::size_t m_type_id_next = 1;

        std::size_t assign_type_id()
        {
            std::unique_lock< std::shared_mutex > lock(m_mutex);
            return m_type_id_next++;
        }

        cow< source_bundle > m_source_code;
        std::string m_configured_target;

      public:
        compiler(cow< source_bundle > source_code, std::string target);

      private:
        void init_output_info();
        // The lk_* functions are used by resolvers to solve the graph

        // Get the parsed AST for a file

        // Should actually be named lk_file_contents, but kept for legacy reasons
        // DEPRECATED: Use lk_file_contents instead
        out< std::string > file_contents(std::string const& filename);

        // Migrate to using lk_file_contents as the name
        out< std::string > lk_file_contents(std::string const& filename)
        {
            return file_contents(filename);
        }

        // Look up the AST for a given glass given a paritcular cannonical chain

        // get_* functions are used only for debugging and by non-resolver consumers of the class
        // Each get_* function calls the lk_* function and then solves the graph
      public:
        // Gets the content of a named file
        std::string get_file_contents(std::string const& filename)
        {
            auto node = file_contents(filename);
            m_solver.solve(this, node);
            return node->get();
        }

        std::size_t get_class_size(type_symbol const& chain)
        {
            auto size = lk_class_size_from_canonical_lookup_chain(chain);
            m_solver.solve(this, size);
            return size->get();
        }

        type_placement_info get_class_placement_info(type_symbol const& chain)
        {
            auto size = lk_type_placement_info_from_canonical_type(chain);
            m_solver.solve(this, size);
            return size->get();
        }


        //function_ast get_function_ast_of_overload(type_symbol chain);
    };

} // namespace quxlang

#endif // QUXLANG_COMPILER_HEADER_GUARD