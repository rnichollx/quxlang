//
// Created by Ryan Nicholl on 11/26/23.
//

#ifndef QUXLANG_RES_DETAIL_VM_PROCEDURE_BUILDER_HEADER_GUARD
#define QUXLANG_RES_DETAIL_VM_PROCEDURE_BUILDER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/vm_generation_frameinfo.hpp"

namespace quxlang
{
    struct context_frame2
        {
            std::size_t exception_ct = 0;
            bool closed = false;

          public:
            context_frame2(type_symbol func, compiler* c, vm_generation_frame_info& frame, vm_block& block);
            explicit context_frame2(context_frame2 & other);

            struct condition_t
            {
            };
            struct then_t
            {
            };
            struct else_t
            {
            };
            struct loop_t
            {
            };

            static constexpr condition_t condition_tag = condition_t{};
            static constexpr then_t then_tag = then_t{};
            static constexpr else_t else_tag = else_t{};
            static constexpr loop_t loop_tag = loop_t{};

            void comment(std::string const & str);

            explicit context_frame2(context_frame2 & other, vm_if& insertion_point, condition_t);
            explicit context_frame2(context_frame2 & other, vm_if& insertion_point, then_t);
            explicit context_frame2(context_frame2 & other, vm_if& insertion_point, else_t);
            explicit context_frame2(context_frame2 & other, vm_while& insertion_point, condition_t);
            explicit context_frame2(context_frame2 & other, vm_while& insertion_point, loop_t);
            context_frame2(context_frame2&& other) = delete;

            ~context_frame2() noexcept(false);

            bool close();
            void discard();

            [[nodiscard]] inline std::pair< bool, std::size_t > create_variable_storage(std::string name, type_symbol type);
            [[nodiscard]] inline std::pair< bool, std::size_t > create_value_storage(std::optional< std::string > name, type_symbol type);
            [[nodiscard]] inline std::pair< bool, std::size_t > create_temporary_storage(type_symbol type);
            [[nodiscard]] std::pair< bool, vm_value > load_temporary(std::size_t index);
            [[nodiscard]] std::pair< bool, vm_value > load_temporary_as_new(std::size_t index);
            [[nodiscard]] bool set_return_value(vm_value);
            [[nodiscard]] inline std::pair< bool, vm_value > load_variable(std::size_t index)
            {
                return load_value(index, true, false);
            }
            [[nodiscard]] inline std::pair< bool, vm_value > load_variable_as_new(std::size_t index)
            {
                return load_value(index, false, false);
            }
            [[nodiscard]] bool set_value_alive(std::size_t index);
            [[nodiscard]] bool set_value_dead(std::size_t index);
            [[nodiscard]] std::pair< bool, std::size_t > construct_new_temporary(type_symbol type, std::vector< vm_value > args);
            [[nodiscard]] std::pair< bool, std::size_t > adopt_value_as_temporary(vm_value val);
            [[nodiscard]] std::pair< bool, std::optional< std::size_t > > try_get_variable_index(std::string name);
            [[nodiscard]] std::pair< bool, type_symbol > get_variable_type(std::size_t index);
            [[nodiscard]] std::pair< bool, std::optional< type_symbol > > try_get_variable_type(std::string name);

            [[nodiscard]] std::pair< bool, std::optional< vm_value > > try_load_variable(std::string name);
            [[nodiscard]] std::pair< bool, vm_value > load_value(std::size_t index, bool alive, bool temp);
            [[nodiscard]] std::pair< bool, vm_value > load_value_as_desctructable(std::size_t index);

            [[nodiscard]] inline std::pair< bool, vm_value > load_variable(std::string name);
            [[nodiscard]] bool construct_new_variable(std::string name, type_symbol type, std::vector< vm_value > args);
            [[nodiscard]] bool destroy_value(std::size_t index);
            [[nodiscard]] bool frame_return(vm_value val);
            [[nodiscard]] bool frame_return();
            [[nodiscard]] bool run_value_destructor(std::size_t index);
            [[nodiscard]] bool run_value_constructor(std::size_t index, std::vector< vm_value > args);

          public:
            type_symbol current_context() const;

            void push(vm_executable_unit s)
            {
                m_new_block.code.push_back(std::move(s));
            }

            class compiler* compiler() const
            {
                return m_c;
            }

          private:
        public:
            class compiler* m_c;
            vm_generation_frame_info& m_frame;
            type_symbol m_ctx;
            //vm_block& m_block;
            vm_block m_new_block;
            std::function< void(vm_block) > m_insertion_point;
            vm_procedure_from_canonical_functanoid_resolver* m_resolver;
        };
}

#endif //VM_PROCEDURE_BUILDER_HEADER_GUARD
