//
// Created by Ryan Nicholl on 10/6/23.
//

#include <gtest/gtest.h>

#include "rylang/manipulators/merge_entity.hpp"

TEST(rylang_modules, merge_entities)
{
    rylang::entity_ast a;
    //a.m_name = "foo";
    a.m_sub_entities = { {"bar", rylang::entity_ast{} } };


    rylang::entity_ast b;
}
