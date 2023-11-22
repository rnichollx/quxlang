//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_HEADER

#include "rylang/ast/file_ast.hpp"
#include "rylang/ast/module_ast_precursor1.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_parameter_information.hpp"
#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/data/class_layout.hpp"
#include "rylang/data/function_frame_information.hpp"
#include "rylang/data/llvm_proxy_types.hpp"
#include "rylang/data/lookup_chain.hpp"
#include "rylang/data/machine_info.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/data/vm_procedure.hpp"
#include "rylang/filelist.hpp"
#include "rylang/res/canonical_symbol_from_contextual_symbol_resolver.hpp"
#include "rylang/res/canonical_type_is_implicitly_convertible_to_resolver.hpp"
#include "rylang/res/class_field_list_from_canonical_chain_resolver.hpp"
#include "rylang/res/class_layout_from_canonical_chain_resolver.hpp"
#include "rylang/res/class_list_resolver.hpp"
#include "rylang/res/class_should_autogen_default_constructor_resolver.hpp"
#include "rylang/res/class_size_from_canonical_chain_resolver.hpp"
#include "rylang/res/contextualized_reference_resolver.hpp"
#include "rylang/res/entity_ast_from_canonical_chain_resolver.hpp"
#include "rylang/res/entity_ast_from_chain_resolver.hpp"
#include "rylang/res/entity_canonical_chain_exists_resolver.hpp"
#include "rylang/res/file_ast_resolver.hpp"
#include "rylang/res/file_content_resolver.hpp"
#include "rylang/res/file_module_map_resolver.hpp"
#include "rylang/res/filelist_resolver.hpp"
#include "rylang/res/files_in_module_resolver.hpp"
#include "rylang/res/function_ast_resolver.hpp"
#include "rylang/res/function_overload_selection_resolver.hpp"
#include "rylang/res/function_qualified_reference_resolver.hpp"
#include "rylang/res/functum_exists_and_is_callable_with_resolver.hpp"
#include "rylang/res/module_ast_precursor1_resolver.hpp"
#include "rylang/res/module_ast_resolver.hpp"
#include "rylang/res/operator_is_overloaded_with_resolver.hpp"
#include "rylang/res/overload_set_is_callable_with_resolver.hpp"
#include "rylang/res/symbol_canonical_chain_exists_resolver.hpp"
#include "rylang/res/type_placement_info_from_canonical_type_resolver.hpp"
#include "rylang/res/type_size_from_canonical_type_resolver.hpp"
#include "rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"
#include "rylang/res/list_functum_overloads_resolver.hpp"
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
        friend class canonical_symbol_from_contextual_symbol_resolver;
        friend class type_size_from_canonical_type_resolver;
        friend class class_layout_from_canonical_chain_resolver;
        friend class class_placement_info_from_cannonical_chain_resolver;
        friend class type_placement_info_from_canonical_type_resolver;
        friend class entity_canonical_chain_exists_resolver;
        friend class llvm_code_generator;
        friend class canonical_type_is_implicitly_convertible_to_resolver;
        friend class overload_set_is_callable_with_resolver;
        friend class function_overload_selection_resolver;
        friend class function_qualified_reference_resolver;
        friend class contextualized_reference_resolver;
        friend class function_ast_resolver;
        friend class vm_procedure_from_canonical_functanoid_resolver;
        friend class function_frame_information_resolver;
        friend class operator_is_overloaded_with_resolver;
        friend class symbol_canonical_chain_exists_resolver;
        friend class class_should_autogen_default_constructor_resolver;
        friend class functum_exists_and_is_callable_with_resolver;
        friend class list_functum_overloads_resolver;

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
        index< entity_ast_from_chain_resolver > m_entity_ast_from_chain_index;
        index< entity_ast_from_canonical_chain_resolver > m_entity_ast_from_cannonical_chain_index;
        index< module_ast_resolver > m_module_ast_index;
        index< module_ast_precursor1_resolver > m_module_ast_precursor1_index;

        index< list_functum_overloads_resolver> m_list_functum_overloads_index;
        out< std::optional<std::set< call_parameter_information > > > lk_list_functum_overloads(qualified_symbol_reference const& chain)
        {
            return m_list_functum_overloads_index.lookup(chain);
        }

        index < functum_exists_and_is_callable_with_resolver > m_functum_exists_and_is_callable_with_index;
        out< bool > lk_functum_exists_and_is_callable_with(qualified_symbol_reference const& chain, call_parameter_information const& os)
        {
            return m_functum_exists_and_is_callable_with_index.lookup(std::make_pair(chain, os));
        }

        index< class_should_autogen_default_constructor_resolver > m_class_should_autogen_default_constructor_index;
        out< bool > lk_class_should_autogen_default_constructor(qualified_symbol_reference const &cls)
        {
            return m_class_should_autogen_default_constructor_index.lookup(cls);//
        }

        index< symbol_canonical_chain_exists_resolver > m_symbol_canonical_chain_exists_index;
        out< bool > lk_symbol_canonical_chain_exists(qualified_symbol_reference chain)
        {
            return m_symbol_canonical_chain_exists_index.lookup(chain);
        }

        index< operator_is_overloaded_with_resolver> m_operator_is_overloaded_with_index;
        out< std::optional< qualified_symbol_reference > > lk_operator_is_overloaded_with(std::string op, qualified_symbol_reference lhs, qualified_symbol_reference rhs)
        {
            return m_operator_is_overloaded_with_index.lookup(std::make_tuple(op, lhs, rhs));
        }



        index< function_ast_resolver > m_function_ast_index;
        out< function_ast > lk_function_ast(qualified_symbol_reference func_addr)
        {
            return m_function_ast_index.lookup(func_addr);
        }

        index< vm_procedure_from_canonical_functanoid_resolver > m_vm_procedure_from_canonical_functanoid_index;
        out< vm_procedure > lk_vm_procedure_from_canonical_functanoid(qualified_symbol_reference func_addr)
        {
            return m_vm_procedure_from_canonical_functanoid_index.lookup(func_addr);
        }

      public:
        vm_procedure get_vm_procedure_from_canonical_functanoid(qualified_symbol_reference func_addr)
        {
            auto node = lk_vm_procedure_from_canonical_functanoid(func_addr);
            m_solver.solve(this, node);
            return node->get();
        }

      private:
        index< contextualized_reference_resolver > m_contextualized_reference_index;
        out< qualified_symbol_reference > lk_contextualized_reference(qualified_symbol_reference symbol, qualified_symbol_reference context)
        {
            return m_contextualized_reference_index.lookup(std::make_pair(symbol, context));
        }

        index< function_qualified_reference_resolver > m_function_qualname_index [[deprecated]] ;
        out< qualified_symbol_reference > lk_function_qualname [[deprecated]] (qualified_symbol_reference f, call_parameter_information args)
        {
            return m_function_qualname_index.lookup(std::make_pair(f, args));
        }

      public:
        qualified_symbol_reference get_function_qualname [[deprecated]] (qualified_symbol_reference name, call_parameter_information args);

      private:
        // index< class_list_resolver > m_class_list_index;

        index< function_overload_selection_resolver > m_function_overload_selection_index;
        out< call_parameter_information > lk_function_overload_selection(qualified_symbol_reference const& chain, call_parameter_information const& os)
        {
            return m_function_overload_selection_index.lookup(std::make_pair(chain, os));
        }

        index< overload_set_is_callable_with_resolver > m_overload_set_is_callable_with_index;
        out< bool > lk_overload_set_is_callable_with(std::pair< call_parameter_information, call_parameter_information > const& input)
        {
            return m_overload_set_is_callable_with_index.lookup(input);
        }

        out< bool > lk_overload_set_is_callable_with(call_parameter_information what, call_parameter_information with)
        {
            return m_overload_set_is_callable_with_index.lookup(std::make_pair(std::move(what), std::move(with)));
        }

        index< canonical_type_is_implicitly_convertible_to_resolver > m_canonical_type_is_implicitly_convertible_to_index;
        out< bool > lk_canonical_type_is_implicitly_convertible_to(qualified_symbol_reference from, qualified_symbol_reference to)
        {
            return m_canonical_type_is_implicitly_convertible_to_index.lookup(std::make_pair(from, to));
        }

        out< bool > lk_canonical_type_is_implicitly_convertible_to(std::pair< qualified_symbol_reference, qualified_symbol_reference > const& input)
        {
            return m_canonical_type_is_implicitly_convertible_to_index.lookup(input);
        }

        index< entity_canonical_chain_exists_resolver > m_entity_canonical_chain_exists_index;
        out< bool > lk_entity_canonical_chain_exists(qualified_symbol_reference const& chain)
        {
            return m_entity_canonical_chain_exists_index.lookup(chain);
        }

        index< class_layout_from_canonical_chain_resolver > m_class_layout_from_canonical_chain_index;
        out< class_layout > lk_class_layout_from_canonical_chain(qualified_symbol_reference const& chain)
        {
            return m_class_layout_from_canonical_chain_index.lookup(chain);
        }

        index< type_placement_info_from_canonical_type_resolver > m_type_placement_info_from_canonical_chain_index;
        out< type_placement_info > lk_type_placement_info_from_canonical_type(qualified_symbol_reference const& ref)
        {
            assert(!typeis<numeric_literal_reference>(ref));
            return m_type_placement_info_from_canonical_chain_index.lookup(ref);
        }

        index< class_field_list_from_canonical_chain_resolver > m_class_field_list_from_canonical_chain_index;
        out< std::vector< class_field_declaration > > lk_class_field_declaration_list_from_canonical_chain(qualified_symbol_reference const& chain)
        {
            return m_class_field_list_from_canonical_chain_index.lookup(chain);
        }

        index< class_size_from_canonical_chain_resolver > m_class_size_from_canonical_chain_index;
        out< std::size_t > lk_class_size_from_canonical_lookup_chain(qualified_symbol_reference const& chain)
        {
            return m_class_size_from_canonical_chain_index.lookup(chain);
        }

        index< files_in_module_resolver > m_files_in_module_resolver;

        index< canonical_symbol_from_contextual_symbol_resolver > m_canonical_type_ref_from_contextual_type_ref_resolver;
        out< qualified_symbol_reference > lk_canonical_type_from_contextual_type(contextual_type_reference const& ref)
        {
            return m_canonical_type_ref_from_contextual_type_ref_resolver.lookup(ref);
        }
        out< qualified_symbol_reference > lk_canonical_type_from_contextual_type(qualified_symbol_reference type, qualified_symbol_reference context)
        {
            return m_canonical_type_ref_from_contextual_type_ref_resolver.lookup(contextual_type_reference{context, type});
        }

        index< type_size_from_canonical_type_resolver > m_type_size_from_canonical_type_index;
        out< std::size_t > lk_type_size_from_canonical_type(qualified_symbol_reference const& ref)
        {
            return m_type_size_from_canonical_type_index.lookup(ref);
        }

        machine_info m_machine_info;
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

        // Look up the AST for a given glass given a paritcular cannonical chain
        out< entity_ast > lk_entity_ast_from_canonical_chain(qualified_symbol_reference const& chain)
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

        std::size_t get_class_size(qualified_symbol_reference const& chain)
        {
            auto size = lk_class_size_from_canonical_lookup_chain(chain);
            m_solver.solve(this, size);
            return size->get();
        }

        type_placement_info get_class_placement_info(qualified_symbol_reference const& chain)
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
        llvm_proxy_type get_llvm_proxy_return_type_of(qualified_symbol_reference chain);
        std::vector< llvm_proxy_type > get_llvm_proxy_argument_types_of(qualified_symbol_reference chain);

        function_ast get_function_ast_of_overload(qualified_symbol_reference chain);
        call_parameter_information get_function_overload_selection(qualified_symbol_reference chain, call_parameter_information args);
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_COMPILER_HEADER
