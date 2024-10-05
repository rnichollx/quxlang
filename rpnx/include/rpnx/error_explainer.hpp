// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include <string>

namespace rpnx
{
    template <typename T>
    class node_base;

    template < typename T >
    inline std::string get_error_string(rpnx::node_base< T > const& node)
    {
        try
        {
            auto ptr = node.get_error();
            std::rethrow_exception(ptr);
        }
        catch (std::exception const& er)
        {
            return er.what();
        }
        catch (...)
        {
        }
        return "?";
    }

    template < typename T >
    inline std::string explain_error(rpnx::node_base< T > const& node)
    {
        std::string error_string = get_error_string(node);

        if (error_string.empty())
        {
            return "Unknown error";
        }

        auto errors = node.error_dependencies();

        if (!errors.empty())
        {
            for (auto& node : errors)
            {

                error_string += "\n";
                for (auto& e : errors)
                {
                    error_string += std::string() + "Caused by: " + typeid(*node).name() + ": " + explain_error(*node);
                }
            }
        }

        return error_string;
    }
} // namespace quxlang