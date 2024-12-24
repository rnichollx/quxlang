// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"



quxlang::compiler::compiler(cow<source_bundle> srcs, std::string target)
    : m_source_code(std::move(srcs)),
      m_configured_target(target)
{
   init_output_info();
}


void quxlang::compiler::init_output_info()
{
    m_output_info = m_source_code.get().targets.at(m_configured_target).target_output_config;
}
