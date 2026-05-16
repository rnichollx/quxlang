// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/instanciation_concrete_params_spec.hpp>

namespace quxlang
{
    rpnx::querygraph::coroutine< instanciation_concrete_params_spec > instanciation_concrete_params_impl(instanciation_reference input)
    {
        instatype result = input.params;

        auto owning_member_type = [&](instanciation_reference const& inst) -> rpnx::querygraph::coroutine< instanciation_concrete_params_spec >::cosubroutine< type_symbol >
        {
            type_symbol owner_symbol = inst.temploid.templexoid;
            if (typeis< instanciation_reference >(owner_symbol))
            {
                owner_symbol = as< instanciation_reference >(owner_symbol).temploid.templexoid;
            }

            if (!typeis< submember >(owner_symbol))
            {
                throw compiler_bug("Concrete THISTYPE lowering requires a member instantiation context");
            }

            co_return as< submember >(owner_symbol).of;
        };

        auto lower_direct_thistype =
            [&](auto&& self, type_symbol type) -> rpnx::querygraph::coroutine< instanciation_concrete_params_spec >::cosubroutine< type_symbol >
        {
            if (typeis< thistype >(type))
            {
                co_return co_await owning_member_type(input);
            }

            if (typeis< ptrref_type >(type))
            {
                ptrref_type output = as< ptrref_type >(type);
                output.target = co_await self(self, output.target);
                co_return output;
            }
            if (typeis< nvalue_slot >(type))
            {
                nvalue_slot output = as< nvalue_slot >(type);
                output.target = co_await self(self, output.target);
                co_return output;
            }
            if (typeis< dvalue_slot >(type))
            {
                dvalue_slot output = as< dvalue_slot >(type);
                output.target = co_await self(self, output.target);
                co_return output;
            }
            if (typeis< attached_type_reference >(type))
            {
                attached_type_reference output = as< attached_type_reference >(type);
                output.carrying_type = co_await self(self, output.carrying_type);
                output.attached_symbol = co_await self(self, output.attached_symbol);
                co_return output;
            }
            if (typeis< array_type >(type))
            {
                array_type output = as< array_type >(type);
                output.element_type = co_await self(self, output.element_type);
                co_return output;
            }
            if (typeis< array_initializer_type >(type))
            {
                array_initializer_type output = as< array_initializer_type >(type);
                output.element_type = co_await self(self, output.element_type);
                co_return output;
            }
            if (typeis< procedure_type >(type))
            {
                procedure_type output = as< procedure_type >(type);
                for (auto& [name, param_type] : output.signature.params.named)
                {
                    (void)name;
                    param_type = co_await self(self, param_type);
                }
                for (type_symbol& param_type : output.signature.params.positional)
                {
                    param_type = co_await self(self, param_type);
                }
                if (output.signature.return_type.has_value())
                {
                    output.signature.return_type = co_await self(self, *output.signature.return_type);
                }
                co_return output;
            }
            if (typeis< storage >(type))
            {
                storage output;
                for (type_symbol const& storable_type : as< storage >(type).storable_types)
                {
                    output.storable_types.insert(co_await self(self, storable_type));
                }
                co_return output;
            }
            if (typeis< aligned_storage >(type))
            {
                co_return type;
            }
            if (typeis< initialization_reference >(type))
            {
                initialization_reference output = as< initialization_reference >(type);
                output.initializee = co_await self(self, output.initializee);
                if (output.context.has_value())
                {
                    output.context = co_await self(self, *output.context);
                }
                for (parameter_instantiation& param : output.parameters.positional)
                {
                    type_symbol lowered = co_await self(self, parameter_instantiation_type(param));
                    if (param.template type_is< parameter_value_instantiation >())
                    {
                        param.template get_as< parameter_value_instantiation >().type = std::move(lowered);
                    }
                    else
                    {
                        param = make_type_instantiation(std::move(lowered));
                    }
                }
                for (auto& [name, param] : output.parameters.named)
                {
                    (void)name;
                    type_symbol lowered = co_await self(self, parameter_instantiation_type(param));
                    if (param.template type_is< parameter_value_instantiation >())
                    {
                        param.template get_as< parameter_value_instantiation >().type = std::move(lowered);
                    }
                    else
                    {
                        param = make_type_instantiation(std::move(lowered));
                    }
                }
                co_return output;
            }
            co_return type;
        };

        auto canonicalize_param = [&](parameter_instantiation& param) -> rpnx::querygraph::coroutine< instanciation_concrete_params_spec >::cosubroutine< void >
        {
            type_symbol concrete_type = co_await lower_direct_thistype(lower_direct_thistype, parameter_instantiation_type(param));
            if (param.template type_is< parameter_value_instantiation >())
            {
                param.template get_as< parameter_value_instantiation >().type = std::move(concrete_type);
            }
            else
            {
                param = make_type_instantiation(std::move(concrete_type));
            }
            co_return;
        };

        for (parameter_instantiation& param : result.positional)
        {
            co_await canonicalize_param(param);
        }
        for (auto& [_, param] : result.named)
        {
            co_await canonicalize_param(param);
        }

        co_return result;
    }
} // namespace quxlang
