//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "rylang/ast/class_ast.hpp"
#include "rylang/ast/file_ast.hpp"
#include "rylang/ast/module_ast_precursor1.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_overload_set.hpp"
#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/data/class_layout.hpp"
#include "rylang/data/llvm_proxy_types.hpp"
#include "rylang/data/lookup_chain.hpp"
#include "rylang/data/machine_info.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/filelist.hpp"
#include "rylang/res/canonical_chain_resolver.hpp"
#include "rylang/res/canonical_type_is_implicitly_convertible_to_resolver.hpp"
#include "rylang/res/canonical_type_ref_from_contextual_type_ref_resolver.hpp"
#include "rylang/res/class_field_list_from_canonical_chain_resolver.hpp"
#include "rylang/res/class_layout_from_canonical_chain_resolver.hpp"
#include "rylang/res/class_list_resolver.hpp"
#include "rylang/res/class_size_from_canonical_chain_resolver.hpp"
#include "rylang/res/entity_ast_from_canonical_chain_resolver.hpp"
#include "rylang/res/entity_ast_from_chain_resolver.hpp"
#include "rylang/res/entity_canonical_chain_exists_resolver.hpp"
#include "rylang/res/file_ast_resolver.hpp"
#include "rylang/res/file_content_resolver.hpp"
#include "rylang/res/file_module_map_resolver.hpp"
#include "rylang/res/filelist_resolver.hpp"
#include "rylang/res/files_in_module_resolver.hpp"
#include "rylang/res/function_overload_selection_resolver.hpp"
#include "rylang/res/function_qualified_reference_resolver.hpp"
#include "rylang/res/module_ast_precursor1_resolver.hpp"
#include "rylang/res/module_ast_resolver.hpp"
#include "rylang/res/overload_set_is_callable_with_resolver.hpp"
#include "rylang/res/type_placement_info_from_canonical_type_resolver.hpp"
#include "rylang/res/type_size_from_canonical_type_resolver.hpp"
#include <mutex>
#include <shared_mutex>

namespace rylang
{
    class compiler
    {
        friend class filelist_resolver;
        friend class class_list_resolver;
        friend class file_ast_resolver;
        friend class file_content_resolver;
        friend class classes_per_file_resolver;
        friend class type_id_assignment_resolver;
        friend class class_id_ast_resolver;
        friend class files_in_module_resolver;
        friend class file_module_map_resolver;
        friend class class_field_list_from_canonical_chain_resolver;
        friend class entity_ast_from_canonical_chain_resolver;
        friend class entity_ast_from_chain_resolver;
        friend class module_ast_precursor1_resolver;
        friend class class_size_from_canonical_chain_resolver;
        friend class module_ast_resolver;
        friend class canonical_type_ref_from_contextual_type_ref_resolver;
        friend class type_size_from_canonical_type_resolver;
        friend class class_layout_from_canonical_chain_resolver;
        friend class class_placement_info_from_cannonical_chain_resolver;
        friend class type_placement_info_from_canonical_type_resolver;
        friend class entity_canonical_chain_exists_resolver;
        friend class llvm_code_generator;
        friend class canonical_type_is_implicitly_convertible_to_resolver;
        friend class overload_set_is_callable_with_resolver;
        friend class function_overload_selection_resolver;


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
        class_list_resolver m_class_list_resolver;
        singleton< file_module_map_resolver > m_file_module_map_resolver;
        index< file_content_resolver > m_file_contents_index;
        index< file_ast_resolver > m_file_ast_index;
        index< canonical_chain_resolver > m_cannonical_chain_index;
        index< entity_ast_from_chain_resolver > m_entity_ast_from_chain_index;
        index< entity_ast_from_canonical_chain_resolver > m_entity_ast_from_cannonical_chain_index;
        index< module_ast_resolver > m_module_ast_index;
        index< module_ast_precursor1_resolver > m_module_ast_precursor1_index;

        index< function_qualified_reference_resolver > m_function_qualname_index;
        out< qualified_symbol_reference > lk_function_qualname(qualified_symbol_reference f, call_overload_set args)
        {
            return m_function_qualname_index.lookup(std::make_pair(f, args));
        }
        //index< class_list_resolver > m_class_list_index;

        index<function_overload_selection_resolver> m_function_overload_selection_index;
        out < call_overload_set > lk_function_overload_selection(std::pair< canonical_lookup_chain, call_overload_set > const& input)
        {
            return m_function_overload_selection_index.lookup(input);
        }



        index< overload_set_is_callable_with_resolver > m_overload_set_is_callable_with_index;
        out< bool > lk_overload_set_is_callable_with(std::pair< call_overload_set, call_overload_set > const& input)
        {
            return m_overload_set_is_callable_with_index.lookup(input);
        }

        index< canonical_type_is_implicitly_convertible_to_resolver > m_canonical_type_is_implicitly_convertible_to_index;
        out< bool > lk_canonical_type_is_implicitly_convertible_to(std::pair< canonical_type_reference, canonical_type_reference > const& input)
        {
            return m_canonical_type_is_implicitly_convertible_to_index.lookup(input);
        }

        index< entity_canonical_chain_exists_resolver > m_entity_canonical_chain_exists_index;
        out< bool > lk_entity_canonical_chain_exists(canonical_lookup_chain const& chain)
        {
            return m_entity_canonical_chain_exists_index.lookup(chain);
        }

        index< class_layout_from_canonical_chain_resolver > m_class_layout_from_canonical_chain_index;
        out< class_layout > lk_class_layout_from_canonical_chain(canonical_lookup_chain const& chain)
        {
            return m_class_layout_from_canonical_chain_index.lookup(chain);
        }

        index< type_placement_info_from_canonical_type_resolver > m_type_placement_info_from_canonical_chain_index;
        out< type_placement_info > lk_type_placement_info_from_canonical_type(canonical_type_reference const& ref)
        {
            return m_type_placement_info_from_canonical_chain_index.lookup(ref);
        }

        index< class_field_list_from_canonical_chain_resolver > m_class_field_list_from_canonical_chain_index;
        out< std::vector< class_field_declaration > > lk_class_field_declaration_list_from_canonical_chain(canonical_lookup_chain const& chain)
        {
            return m_class_field_list_from_canonical_chain_index.lookup(chain);
        }

        index< class_size_from_canonical_chain_resolver > m_class_size_from_canonical_chain_index;
        out< std::size_t > lk_class_size_from_canonical_lookup_chain(canonical_lookup_chain const& chain)
        {
            return m_class_size_from_canonical_chain_index.lookup(chain);
        }

        index< files_in_module_resolver > m_files_in_module_resolver;

        index< canonical_type_ref_from_contextual_type_ref_resolver > m_canonical_type_ref_from_contextual_type_ref_resolver;
        out< canonical_type_reference > lk_canonical_type_from_contextual_type(contextual_type_reference const& ref)
        {
            return m_canonical_type_ref_from_contextual_type_ref_resolver.lookup(ref);
        }

        index< type_size_from_canonical_type_resolver > m_type_size_from_canonical_type_index;
        out< std::size_t > lk_type_size_from_canonical_type(canonical_type_reference const& ref)
        {
            return m_type_size_from_canonical_type_index.lookup(ref);
        }

        machine_info m_machine_info;

        // machine_info_resolver m_machine_info_resolver;
        // out< machine_info > lk_machine_info()
        //{
        //    return &m_machine_info_resolver;
        //}

        rpnx::single_thread_graph_solver< compiler > m_solver;

        std::shared_mutex m_mutex;

        std::size_t m_type_id_next = 1;

        std::size_t assign_type_id()
        {
            std::unique_lock< std::shared_mutex > lock(m_mutex);
            return m_type_id_next++;
        }

      public:
        compiler(int argc, char** argv);

      private:
        // The lk_* functions are used by resolvers to solve the graph

        // Get the parsed AST for a file
        out< file_ast > lk_file_ast(std::string const& filename);

        out< module_ast > lk_module_ast(std::string const& module_name)
        {
            return m_module_ast_index.lookup(module_name);
        }

        // out< class_ast > lk_class_ast(symbol_id id);
        // out< symbol_id > lk_global_lookup(std::string const& name);
        // out< symbol_id > lk_lookup_full_chain(lookup_chain const& chain);
        // out< symbol_id > lk_lookup_relative(symbol_id, lookup_singular const& part);

        out< filelist > lk_file_list()
        {
            return m_filelist_resolver.lookup();
        }

        // Look up the precursor AST for the module
        //  The precursor AST is the non-preprocessed AST
        //  This works in two steps, first the function looks up the files in the module
        //  Then it merges the ASTs of all files in the same module.
        out< module_ast_precursor1 > lk_module_ast_precursor1(std::string module_id)
        {
            return m_module_ast_precursor1_index.lookup(module_id);
        }

        out< file_module_map > lk_file_module_map()
        {
            return m_file_module_map_resolver.lookup();
        }

        // This function lists the files that belong to a module
        //   It works by looking up the file list, then checking the AST of every file to see if it is in the module
        inline out< filelist > lk_files_in_module(std::string module_id)
        {
            return m_files_in_module_resolver.lookup(module_id);
        }

        // Should actually be named lk_file_contents, but kept for legacy reasons
        // DEPRECATED: Use lk_file_contents instead
        out< std::string > file_contents(std::string const& filename);

        // Migrate to using lk_file_contents as the name
        out< std::string > lk_file_contents(std::string const& filename)
        {
            return file_contents(filename);
        }

        out< std::size_t > lk_class_size_from_symref(lookup_chain const& chain);

        // Lookup the cannonical chain for a chain in a given context.
        out< canonical_lookup_chain > lk_canonical_chain(lookup_chain const& chain, lookup_chain const& context)
        {
            return m_cannonical_chain_index.lookup(std::make_tuple(chain, context));
        }

        // Look up the AST for a given glass given a paritcular cannonical chain
        out< entity_ast > lk_entity_ast_from_canonical_chain(canonical_lookup_chain const& chain)
        {
            return m_entity_ast_from_cannonical_chain_index.lookup(chain);
        }

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

        file_ast get_file_ast(std::string const& filename)
        {
            auto node = lk_file_ast(filename);
            m_solver.solve(this, node);
            return node->get();
        }

        std::size_t get_class_size(canonical_lookup_chain const& chain)
        {
            auto size = lk_class_size_from_canonical_lookup_chain(chain);
            m_solver.solve(this, size);
            return size->get();
        }

        type_placement_info get_class_placement_info(canonical_lookup_chain const& chain)
        {
            auto size = lk_type_placement_info_from_canonical_type(chain);
            m_solver.solve(this, size);
            return size->get();
        }

        class_list get_class_list()
        {
            auto node = &m_class_list_resolver;
            m_solver.solve(this, node);
            return node->get();
        }

        filelist get_file_list()
        {
            auto node = lk_file_list();
            m_solver.solve(this, node);
            return node->get();
        }
        llvm_proxy_type get_llvm_proxy_return_type_of(rylang::canonical_resolved_function_chain chain);
        std::vector< llvm_proxy_type > get_llvm_proxy_argument_types_of(canonical_resolved_function_chain chain);

        function_ast get_function_ast_of_overload(canonical_resolved_function_chain chain);
        call_overload_set get_function_overload_selection(canonical_lookup_chain chain, call_overload_set args);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
