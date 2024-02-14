// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL
#include "rylang/manipulators/llvm_symbol_relocation.hpp"

namespace rylang
{
    symbol_relocation to_symbol_relocation(llvm::object::RelocationRef const& reloc)
    {

        auto symbol = reloc.getSymbol();
        auto symname_ex = symbol->getName();
        if (!symname_ex)
        {
            assert(false);
        }

        auto symname = std::string(symname_ex.get());
        llvm::SmallVector< char, 16 > TypeName;
        reloc.getTypeName(TypeName);

        std::string type_str = std::string(TypeName.begin(), TypeName.end());

        std::cout << "Relocation " << symname << " of type " << type_str << std::endl;

        symbol_relocation reloc_info{};

        static std::map< std::string, std::function< void(llvm::object::RelocationRef const&, symbol_relocation&, std::string const&) > > reloc_map = {
            {"R_ARM_ABS32",
             [](llvm::object::RelocationRef const& reloc, symbol_relocation& reloc_info, std::string const& name)
             {
                 reloc_info = {};
                 reloc_info.target_type = relocation_target_type::address;
                 reloc_info.address_type = relocation_address_type::absolute;
                 reloc_info.write_method = relocation_write_method::clear;
                 reloc_info.byte_ordering = relocation_byte_ordering::little_endian;
                 reloc_info.bit_ordering = relocation_bit_ordering::lsb_to_msb;
                 reloc_info.bits_width = 32;
                 reloc_info.target_symbol = name;
                 reloc_info.relocation_offset = reloc.getOffset();
             }},
            {"R_ARM_CALL",
             [](llvm::object::RelocationRef const& reloc, symbol_relocation& reloc_info, std::string const& name)
             {
                 reloc_info = {};
                 reloc_info.target_type = relocation_target_type::address;
                 reloc_info.address_type = relocation_address_type::relative;
                 reloc_info.write_method = relocation_write_method::clear;
                 reloc_info.byte_ordering = relocation_byte_ordering::little_endian;
                 reloc_info.bit_ordering = relocation_bit_ordering::lsb_to_msb;
                 reloc_info.bits_width = 24;
                 reloc_info.target_symbol = name;
                 reloc_info.relocation_offset = reloc.getOffset();
                 reloc_info.target_bit_shift = 2;
             }},
            {"R_ARM_REL32",
             [](llvm::object::RelocationRef const& reloc, symbol_relocation& reloc_info, std::string const& name)
             {
                 reloc_info = {};
                 reloc_info.target_type = relocation_target_type::address;
                 reloc_info.address_type = relocation_address_type::relative;
                 reloc_info.write_method = relocation_write_method::clear;
                 reloc_info.byte_ordering = relocation_byte_ordering::little_endian;
                 reloc_info.bit_ordering = relocation_bit_ordering::lsb_to_msb;
                 reloc_info.bits_width = 32;
                 reloc_info.target_symbol = name;
                 reloc_info.relocation_offset = reloc.getOffset();
             }}

        };

        auto& func = reloc_map.at(type_str);

        func(reloc, reloc_info, symname);

        return reloc_info;
    }
} // namespace rylang
