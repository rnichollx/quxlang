// Copyright (c) 2025 Ryan P. Nicholl <rnicholl@protonmail.com> http://rpnx.net/
#ifndef RPNX_DEMANGLE_HPP
#define RPNX_DEMANGLE_HPP

#include <string>
#include <cxxabi.h>
#include <cstdlib>

namespace rpnx
{
    inline std::string demangle(const char* name)
    {
        int status = 0;
        char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);

        if (status != 0)
        {
            throw std::runtime_error("Failed to demangle name: " + std::string(name));
        }
        std::string result = demangled;
        free(demangled);
        return result;
    }
} // namespace rpnx

#endif //DEMANGLE_H
