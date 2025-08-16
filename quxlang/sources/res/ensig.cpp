#include "quxlang/data/type_symbol.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/res/ensig.hpp"



template < typename T >
static void merge_set(std::set< T >& target, std::set< T > const& source)
{
    for (auto const& item : source)
    {
        target.insert(item);
    }
}

namespace quxlang
{
    std::set< std::string > get_symbol_tempars(type_symbol const&);
    std::set< std::string > get_ensig_tempars(temploid_ensig const& input)
    {
        std::set< std::string > result;

        for (auto const& param : input.interface.positional)
        {
            auto arg_result = get_symbol_tempars(param.type);
            for (auto const& name : arg_result)
            {
                result.insert(name);
            }
        }

        for (auto const& [_, param] : input.interface.named)
        {
            auto arg_result = get_symbol_tempars(param.type);
            for (auto const& name : arg_result)
            {
                result.insert(name);
            }
        }

        return result;
    }

    struct tempar_extractor
    {

        std::set< std::string > operator()(quxlang::auto_temploidic const& input)
        {
            return {input.name};
        }

        std::set< std::string > operator()(quxlang::temploid_reference const& input)
        {
            std::set< std::string > result;

            auto a = get_ensig_tempars(input.which);
            auto b = get_symbol_tempars(input.templexoid);
            merge_set(result, a);
            merge_set(result, b);
            return result;
        }

        auto operator()(submember const & val)
        {
            return get_symbol_tempars(val.of);
        }

        auto operator()(subsymbol const & val)
        {
            return get_symbol_tempars(val.of);
        }

        auto operator()(instanciation_reference const& val)
        {
            std::set< std::string > result;

            auto a = get_ensig_tempars(val.temploid.which);
            auto b = get_symbol_tempars(val.temploid.templexoid);
            merge_set(result, a);
            merge_set(result, b);

            for (auto const& param : val.params.positional)
            {
                auto arg_result = get_symbol_tempars(param);
                merge_set(result, arg_result);
            }

            for (auto const& [_, param] : val.params.named)
            {
                auto arg_result = get_symbol_tempars(param);
                merge_set(result, arg_result);
            }

            return result;
        }



    };

    std::set< std::string > get_symbol_tempars(type_symbol const& value)
    {
        return rpnx::try_apply_visitor< std::set< std::string > >(tempar_extractor(), value);
    }

} // namespace quxlang

QUX_CO_RESOLVER_IMPL_FUNC_DEF(symbol_tempars)
{
    co_return get_symbol_tempars(input);
}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(ensig_tempars)
{
    co_return get_ensig_tempars(input);
}