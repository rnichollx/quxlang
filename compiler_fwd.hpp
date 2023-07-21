//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_COMPILER_FWD_HEADER
#define RPNX_RYANSCRIPT1031_COMPILER_FWD_HEADER

#include "dependency_resolver_chain.hpp"
#include "indexer.hpp"
#include "lir_type_index.hpp"
#include "map_alg.hpp"
#include "semantic_generator.hpp"
#include "size_index_resolver_fwd.hpp"
#include "type_to_typed_field_declaration_list_resolver_fwd.hpp"
#include <deque>
#include <memory>

namespace rs1031
{

    class compiler
    {
        // lir_type_index m_type_index;
        collector c;
        semantic_generator sg;

        std::size_t m_next_typeid = 1;

        rpnx::graph_index< compiler, sst_size_index_resolver > m_size_index;
        rpnx::graph_index< compiler, type_to_typed_field_declaration_list_resolver > m_memeber_list_index;
        std::deque< std::shared_ptr< rpnx::graph_dependency_base< compiler > > > m_processable;

      public:
        compiler(lir_machine_info machine_info)
        {
        }

        void notify_ready(std::shared_ptr< rpnx::graph_dependency_base< compiler > > dep)
        {
            m_processable.push_back(dep);
        }

        bool keyword_is_builtin_type(std::string str)
        {
            // Allowed keywords: I*  where * is a number
            // U*  where * is a number
            // F*  where * is a number
            // B*  where * is a number
            // MUTEX
            // BOOL
            // SIZE
            // IPTR
            // UPTR

            if (str.size() < 2)
                return false;
            if (str[0] == 'I' || str[0] == 'U' || str[0] == 'F' || str[0] == 'B')
            {
                return std::all_of(str.begin() + 1, str.end(),
                                   [](char c)
                                   {
                                       return std::isdigit(c);
                                   });
            }
            else if (str == "MUTEX" || str == "BOOL" || str == "SIZE" || str == "IPTR" || str == "UPTR")
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        /*
                dep_ptr< lir_type_id > step_builtin_type_id(std::string str)
                {
                    return access_or_create(m_builtin_index, str,
                                            [&]()
                                            {
                                                return this->create_step_assign_type_id();
                                            });
                }

                dep_ptr< lir_type_id > create_step_assign_type_id()
                {
                    return make_hardcoded_dep(this->assign_new_type_id());
                }

                lir_type_id assign_new_type_id()
                {
                    // TODO: Consider mulithreading/mutexes
                    auto id = m_next_typeid++;

                    return id;
                }

                dep_ptr< lir_type_id > create_step_global_lookup(static_lookup_sequence addr)
                {
                    dep_res_func< lir_type_id > res_func = [this, addr](std::optional< lir_type_id >& out, dep_func_ptr< lir_type_id > ptr)
                    {
                        lir_type_id module_root = 0;

                        assert(!addr.empty());

                        lir_type_id current_class_or_namespace = module_root;
                        for (int i = 0; i < addr.size(); i++)
                        {
                            auto next = this->step_relative_lookup(current_class_or_namespace, addr[i]);

                            if (!next->is_resolved())
                            {
                                ptr->add_dependency(next);
                                return false;
                            }
                            current_class_or_namespace = next->get();
                        }

                        out = current_class_or_namespace;
                        return true;
                    };

                    return make_dep_func< lir_type_id >(res_func);
                }

                dep_ptr< lir_type_id > step_relative_lookup(lir_type_id addr, std::string name)
                {
                    return access_or_create(m_symbol_static_lookup_index, std::make_pair(addr, name),
                                            [&]()
                                            {
                                                return this->create_step_relative_lookup(addr, name);
                                            });
                }

                dep_ptr< lir_type_id > create_step_relative_lookup(lir_type_id addr, std::string name)
                {
                    dep_res_func< lir_type_id > res_func = [this, addr, name](std::optional< lir_type_id >& out, dep_func_ptr< lir_type_id > ptr)
                    {
                        auto subclass_list = this->step_resolve_subclass_list(addr);

                        ptr->add_dependency(subclass_list);
                        if (!subclass_list->is_resolved())
                        {
                            return false;
                        }

                        // TODO
                        //   if (!subclass_list->get().count(name))
                        //   {
                        //       throw std::runtime_error("lookup failed");
                        //   }

                        out = assign_new_type_id();
                        return true;
                    };

                    return std::make_shared< dependency_func_node< lir_type_id > >(res_func);
                }

                dep_ptr< std::vector< lir_type_id > > step_resolve_subclass_list(lir_type_id addr)
                {
                    return access_or_create(m_subclass_list_index, addr,
                                            [&]()
                                            {
                                                return this->create_step_resolve_subclass_list(addr);
                                            });
                }

                dep_ptr< std::vector< lir_type_id > > create_step_resolve_subclass_list(lir_type_id addr)
                {
                    dep_res_func< std::vector< lir_type_id > > res_func = [this, addr](std::optional< std::vector< lir_type_id > >& out, dep_func_ptr< std::vector< lir_type_id > > ptr) -> bool
                    {
                        // TODO
                        // auto class_ast = this->step_class_ast_from_id(addr);
                        // if (!class_ast->is_resolved())
                        // {
                        //     ptr->add_dependency(class_ast);
                        //     return false;
                        // }
                    };

                    return make_dep_func< std::vector< lir_type_id > >(res_func);
                }

                dep_ptr< lir_type_fields_full_information > step_field_info(lir_type_id addr)
                {
                    return access_or_create(m_field_index, addr,
                                            [&]()
                                            {
                                                return this->create_step_field_info(addr);
                                            });
                }

                template < typename T >
                using alg_func = typename dependency_func_node< T >::resolver_function_type;

                dep_ptr< lir_typed_field_declaration_list > step_resolve_member_field_type_list(lir_type_id addr)
                {
                    return access_or_create(m_type_field_list_index, addr,
                                            [&]()
                                            {
                                                return this->create_step_resolve_member_field_type_list(addr);
                                            });
                }

                dep_ptr< lir_untyped_field_declaration_list > step_resolve_member_field_untyped_list(lir_type_id addr)
                {
                    return access_or_create(m_type_field_list_index, addr,
                                            [&]()
                                            {
                                                return this->create_step_resolve_member_field_untyped_list(addr);
                                            });
                }

                dep_ptr< lir_untyped_field_declaration_list > create_step_resolve_member_field_untyped_list(lir_type_id addr)
                {
                    // TODO: implement
                    return nullptr; //  std::make_shared< dependency_input< lir_untyped_field_declaration_list > >();
                }

                dep_ptr< lir_typed_field_declaration_list > create_step_resolve_member_field_type_list(lir_type_id addr)
                {
                    dep_res_func< lir_typed_field_declaration_list > res_func = [this, addr](std::optional< lir_typed_field_declaration_list >& out,
                                                                                             dep_func_ptr< lir_typed_field_declaration_list > that) -> bool
                    {
                        auto result = std::dynamic_pointer_cast< dependency_func_node< std::size_t > >(that);

                        assert(result != nullptr);
                        assert(result->is_maybe_resolvable());
                        assert(!result->has_unresolved_dependency());

                        auto member_list_resolver = this->step_resolve_member_field_untyped_list(addr);

                        if (!member_list_resolver->is_resolved())
                        {
                            result->add_dependency(member_list_resolver);
                            return false;
                        }

                        auto member_list = member_list_resolver->get();

                        // TODO: Implement alignment
                        // out = size;

                        return false;
                    };

                    return make_dep_func< lir_typed_field_declaration_list >(res_func);
                }

                dep_func_ptr< lir_type_fields_full_information > create_step_field_info(lir_type_id addr)
                {
                    dep_res_func< lir_type_fields_full_information > res_func = [this, addr](std::optional< lir_type_fields_full_information >& out,
                                                                                             dep_func_ptr< lir_type_fields_full_information > that) -> bool
                    {
                        auto result = std::dynamic_pointer_cast< dependency_func_node< std::size_t > >(that);

                        assert(result != nullptr);
                        assert(result->is_maybe_resolvable());
                        assert(!result->has_unresolved_dependency());

                        auto member_list_resolver = this->step_resolve_member_field_type_list(addr);

                        if (!member_list_resolver->is_resolved())
                        {
                            result->add_dependency(member_list_resolver);
                            return false;
                        }

                        lir_typed_field_declaration_list member_list = member_list_resolver->get();
                        for (lir_field_typeonly_information& m : member_list)
                        {
                            result->add_dependency(this->step_field_info(m.m_type));
                            // result->add_dependency(this->step_calculate_align(m.m_type));
                        }

                        if (result->has_unresolved_dependency())
                        {
                            return false;
                        }

                        lir_type_fields_full_information full_info;

                        for (lir_field_typeonly_information& m : member_list)
                        {
                            lir_type_fields_full_information f = this->step_field_info(m.m_type)->get();
                            if (f.m_align > full_info.m_align)
                            {
                                full_info.m_align = f.m_align;
                            }

                            if (full_info.m_size % f.m_align != 0)
                            {
                                full_info.m_size += f.m_align - (full_info.m_size % f.m_align);
                            }
                            lir_field_full_information full_field;
                            full_field.m_name = m.m_name;
                            full_field.m_type = m.m_type;
                            full_field.m_offset = full_info.m_size;
                            full_info.m_size += f.m_size;
                        }
                        if (full_info.m_size % full_info.m_align != 0)
                        {
                            full_info.m_size += full_info.m_align - (full_info.m_size % full_info.m_align);
                        }

                        out = full_info;

                        return true;
                    };

                    return make_dep_func< lir_type_fields_full_information >(res_func);
                }

                dep_ptr< std::size_t > step_size(lir_type_id addr)
                {
                    return access_or_create(m_size_index, addr,
                                            [&]()
                                            {
                                                return this->create_step_size(addr);
                                            });
                }

                dep_ptr< std::size_t > create_step_size(lir_type_id addr)
                {
                    return make_dep_func< std::size_t >(
                        [this, addr](std::optional< std::size_t >& out, dep_func_ptr< std::size_t > that) -> bool
                        {
                            auto result = std::dynamic_pointer_cast< dependency_func_node< std::size_t > >(that);

                            assert(result != nullptr);
                            assert(result->is_maybe_resolvable());
                            assert(!result->has_unresolved_dependency());

                            auto field_info_resolver = this->step_field_info(addr);

                            if (!field_info_resolver->is_resolved())
                            {
                                result->add_dependency(field_info_resolver);
                                return false;
                            }

                            out = field_info_resolver->get().m_size;

                            return true;
                        });
                }
                */
    };

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_COMPILER_FWD_HEADER
