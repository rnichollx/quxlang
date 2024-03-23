// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#include "quxlang/res/module_source_name_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(module_source_name)
{
    QUXLANG_DEBUG({std::cerr << "module_source_name_resolver: " << input_val << std::endl;});
    auto logical_name = input_val;

    auto target_name = c->m_configured_target;

    QUXLANG_DEBUG({std::cerr << "module_source_name_resolver: target=" << target_name << std::endl;});

    auto const & target_config = c->m_source_code->targets.at(target_name);
    auto source_name = target_config.module_configurations.at(logical_name).source;

    QUX_CO_ANSWER(source_name);
}