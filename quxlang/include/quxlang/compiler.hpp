// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_COMPILER_HEADER_GUARD
#define QUXLANG_COMPILER_HEADER_GUARD

#include "data/target_configuration.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/filelist.hpp"
#include "quxlang/res/called_functanoids_resolver.hpp"
#include "quxlang/res/class.hpp"
#include "quxlang/res/class_field_list_resolver.hpp"
#include "quxlang/res/constexpr.hpp"
#include "quxlang/res/constructor.hpp"
#include "quxlang/res/expr_ir2.hpp"
#include "quxlang/res/filelist_resolver.hpp"
#include "quxlang/res/functanoid.hpp"
#include "quxlang/res/function.hpp"
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/res/module_ast_resolver.hpp"
#include "quxlang/res/pointer.hpp"
#include "quxlang/res/temploid.hpp"
#include "quxlang/res/type_placement_info_resolver.hpp"
#include "quxlang/res/types.hpp"
#include "quxlang/res/variable.hpp"
#include "quxlang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"
#include <quxlang/res/serialization.hpp>
#include <quxlang/res/asm_procedure_from_symbol_resolver.hpp>
#include <quxlang/res/declaroids_resolver.hpp>
#include <quxlang/res/ensig.hpp>
#include <quxlang/res/extern_linksymbol_resolver.hpp>
#include <quxlang/res/functum.hpp>
#include <quxlang/res/instanciation.hpp>
#include <quxlang/res/interpret_bool_resolver.hpp>
#include <quxlang/res/interpret_value_resolver.hpp>
#include <quxlang/res/lookup.hpp>
#include <quxlang/res/module_source_name_resolver.hpp>
#include <quxlang/res/module_sources_resolver.hpp>
#include <quxlang/res/procedure_linksymbol_resolver.hpp>
#include <quxlang/res/static_test.hpp>
#include <quxlang/res/symboid_resolver.hpp>
#include <quxlang/res/symboid_subdeclaroids.hpp>
#include <quxlang/res/symbol_type.hpp>
#include <quxlang/res/template_instanciation.hpp>
#include <quxlang/res/template_instanciation_ast_resolver.hpp>
#include <quxlang/res/template_instanciation_parameter_set_resolver.hpp>
#include <quxlang/res/templex_select_template.hpp>
#include <quxlang/res/vm_procedure2.hpp>

#include <mutex>
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

        template < typename G >
        friend auto type_size_from_canonical_type_question_f(G* g, type_symbol type) -> rpnx::resolver_coroutine< G, std::size_t >;

        template < typename G >
        friend auto type_placement_info_question_f(G* g, type_symbol type) -> rpnx::resolver_coroutine< G, type_placement_info >;

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
        // index< file_content_resolver > m_file_contents_index;

        COMPILER_INDEX(asm_procedure_from_symbol)
        COMPILER_INDEX(constexpr_bool)
        COMPILER_INDEX(constexpr_u64)
        COMPILER_INDEX(constexpr_eval)
        COMPILER_INDEX(constexpr_routine)
        COMPILER_INDEX(class_layout)
        COMPILER_INDEX(class_field_list)
        COMPILER_INDEX(class_builtin)
        COMPILER_INDEX(declaroids)
        COMPILER_INDEX(extern_linksymbol)
        COMPILER_INDEX(exists)
        COMPILER_INDEX(functanoid_parameter_map)
        COMPILER_INDEX(functanoid_return_type)
        COMPILER_INDEX(functum_builtin_overloads)
        COMPILER_INDEX(functanoid_sigtype)
        COMPILER_INDEX(function_positional_parameter_names)
        COMPILER_INDEX(function_builtin)
        COMPILER_INDEX(function_param_names)
        COMPILER_INDEX(functum_exists_and_is_callable_with)
        COMPILER_INDEX(functum_initialize)
        COMPILER_INDEX(functum_select_function)
        COMPILER_INDEX(function_declaration)
        COMPILER_INDEX(function_instanciation)
        COMPILER_INDEX(have_nontrivial_member_ctor)
        COMPILER_INDEX(have_nontrivial_member_dtor)
        COMPILER_INDEX(class_field_declaration_list)
        COMPILER_INDEX(interpret_bool)
        COMPILER_INDEX(interpret_value)
        COMPILER_INDEX(functum_primitive_overloads)
        COMPILER_INDEX(functum_map_user_formal_ensigs)
        COMPILER_INDEX(list_primitive_constructors)
        COMPILER_INDEX(class_requires_gen_default_ctor)
        COMPILER_INDEX(class_requires_gen_copy_ctor)
        COMPILER_INDEX(class_requires_gen_move_ctor)
        //COMPILER_INDEX(class_requires_gen_swap)
        COMPILER_INDEX(function_primitive)
        COMPILER_INDEX(class_requires_gen_default_dtor)
        COMPILER_INDEX(functum_overloads)
        COMPILER_INDEX(functum_list_user_overload_declarations)
        COMPILER_INDEX(list_user_functum_formal_paratypes)
        COMPILER_INDEX(list_static_tests)
        COMPILER_INDEX(lookup);
        COMPILER_INDEX(functum_list_user_ensig_declarations)
        COMPILER_INDEX(module_ast)
        COMPILER_INDEX(module_source_name)
        COMPILER_INDEX(class_default_dtor)
        COMPILER_INDEX(class_default_ctor)
        COMPILER_INDEX(function_ensig_initialize_with)
        COMPILER_INDEX(procedure_linksymbol)
        COMPILER_INDEX(symbol_type)
        COMPILER_INDEX(symboid)
        COMPILER_INDEX(symbol_tempars)
        COMPILER_INDEX(ensig_tempars)
        COMPILER_INDEX(symboid_subdeclaroids)
        COMPILER_INDEX(template_instanciation)
        COMPILER_INDEX(template_instanciation_ast)
        COMPILER_INDEX(templex_select_template)
        COMPILER_INDEX(class_trivially_constructible)
        COMPILER_INDEX(class_trivially_destructible)
        COMPILER_INDEX(instanciation)
        COMPILER_INDEX(template_instanciation_parameter_set)
        COMPILER_INDEX(instanciation_parameter_map)
        COMPILER_INDEX(templexoid_user_header_list)
        COMPILER_INDEX(templexoid_user_ensig_set)
        COMPILER_INDEX(templexoid_ensig_set)
        COMPILER_INDEX(variable_type)
        COMPILER_INDEX(vm_procedure3)
        COMPILER_INDEX(user_vm_procedure3)
        COMPILER_INDEX(user_default_dtor_exists)
        COMPILER_INDEX(user_default_ctor_exists)
        COMPILER_INDEX(user_copy_ctor_exists)
        COMPILER_INDEX(user_move_ctor_exists)
        COMPILER_INDEX(user_swap_exists)
        COMPILER_INDEX(builtin_vm_procedure3)
        COMPILER_INDEX(builtin_default_ctor_vm_procedure3)
        COMPILER_INDEX(builtin_copy_ctor_vm_procedure3)
        COMPILER_INDEX(builtin_move_ctor_vm_procedure3)
        COMPILER_INDEX(builtin_dtor_vm_procedure3)
        COMPILER_INDEX(functum_user_overloads)
        COMPILER_INDEX(type_placement_info)
        COMPILER_INDEX(uintpointer_type)
        COMPILER_INDEX(implicitly_convertible_to);
        COMPILER_INDEX(module_sources);
        COMPILER_INDEX(user_serialize_exists)
        COMPILER_INDEX(user_deserialize_exists)
        COMPILER_INDEX(type_is_implicitly_datatype)
        COMPILER_INDEX(type_should_autogen_serialize)
        COMPILER_INDEX(type_should_autogen_deserialize);



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
    };

} // namespace quxlang

#endif // QUXLANG_COMPILER_HEADER_GUARD

