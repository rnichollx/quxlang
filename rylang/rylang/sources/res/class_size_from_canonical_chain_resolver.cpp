//
// Created by Ryan Nicholl on 10/20/23.
//
#include "rylang/res/class_size_from_canonical_chain_resolver.hpp"

#include "rylang/compiler.hpp"

#include <iostream>
void rylang::class_size_from_canonical_chain_resolver::process(compiler* c)
{
    auto& chain = this->m_chain;

    // TODO:

    // 1 get class field information
    auto class_field_dp = get_dependency(
        [&]
        {
            return c->lk_class_field_list_from_canonical_chain(chain);
        });

    if (!ready())
        return;

    std::vector< class_field > class_fields = class_field_dp->get();

    // requires: class_field_from_canonical_chain_resolver

    std::size_t total_size = 0;

    // naive implementation, add all field sizes together

    for (auto& f : class_fields)
    {
        std::cout << "Field: " << f.name << " " << std::endl;

        auto fields_type = f.type;

        contextual_type_reference ctx_type_ref;
        ctx_type_ref.type = fields_type;
        ctx_type_ref.context = chain;

        auto canonical_type_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_type_from_contextual_type(ctx_type_ref);
            });

        if (!ready())
            return;

        auto canonical_type = canonical_type_dp->get();

        auto canonical_type_size_dp = get_dependency(
            [&]
            {
                return c->lk_type_size_from_canonical_type(canonical_type);
            });

        if (!ready())
            return;

        auto canonical_type_size = canonical_type_size_dp->get();

        total_size += canonical_type_size;
    }

    set_value(total_size);
}
