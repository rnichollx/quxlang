//
// Created by Ryan Nicholl on 10/6/23.
//

#include <gtest/gtest.h>

#include "rylang/manipulators/merge_entity.hpp"

TEST(rylang_modules, merge_entities)
{
    rylang::entity_ast a(rylang::class_entity_ast{}, false, {{"foo", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}});

    rylang::entity_ast b(rylang::class_entity_ast{}, false, {{"bar", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}});

    rylang::entity_ast c(rylang::class_entity_ast{}, false, {});

    rylang::entity_ast e(rylang::class_entity_ast{}, false, {{"foo", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}, {"bar", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}});

    rylang::merge_entity(c, a);
    rylang::merge_entity(c, b);

    ASSERT_EQ(c, e);
}
