//
// Created by Ryan Nicholl on 2/13/24.
//

#include "rylang/manipulators/convert_llvm_object.hpp"
#include "rylang/manipulators/llvm_symbol_relocation.hpp"
#include "rylang/manipulators/symbolmap.hpp"

void rylang::convert_llvm_object(llvm::object::ObjectFile const& obj, std::function< void(rylang::object_symbol) > callback)
{

    std::vector< rylang::object_symbol > outputs;

    for (auto const& section : obj.sections())
    {
        auto section_name_ex = section.getName();
        if (!section_name_ex)
        {
            throw std::runtime_error("Section has no name");
        }

        std::string section_name = section_name_ex.get().str();
        if (section_name == "")
        {
            // Undefined symbols
            continue;
        }

        auto data_ex = section.getContents();

        if (!data_ex)
        {
            throw std::runtime_error("Section has no data");
        }

        auto data = data_ex.get();

        std::vector< rylang::symbol_map_info_input > symbol_map;

        for (auto const& sym : obj.symbols())
        {

            auto sym_name_ex = sym.getName();
            if (!sym_name_ex)
            {
                throw std::runtime_error("Symbol has no name");
            }

            std::string sym_name = sym_name_ex.get().str();

            if (sym_name == "")
            {
                continue;
            }

            auto sym_section_ex = sym.getSection();
            if (!sym_section_ex)
            {
                throw std::runtime_error("Symbol has no section");
            }

            auto sym_section = sym_section_ex.get();

            std::string sym_section_name;
            if (sym_section->getIndex() >= std::distance(obj.section_begin(), obj.section_end()))
            {
                continue;
            }
            auto sym_section_name_ex = sym_section->getName();
            if (!sym_section_name_ex)

            {
                // get the error message

                auto err = sym_section_name_ex.takeError();
                std::string errorMessage;
                llvm::handleAllErrors(std::move(err),
                                      [&](llvm::ErrorInfoBase& eib)
                                      {
                                          if (!errorMessage.empty())
                                              errorMessage += ", ";
                                          errorMessage += eib.message();
                                      });

                throw std::runtime_error(errorMessage);
            }
            else
            {
                sym_section_name = sym_section_name_ex.get().str();
            }

            if (sym_section->getIndex() != section.getIndex())
            {
                // Symbol is in another section
                continue;
            }

            symbol_map_info_input x{
                .m_position = section.getAddress(),
                .m_name = sym_name,
            };

            symbol_map.push_back(x);
        }

        calc_symbol_positions(symbol_map.begin(), symbol_map.end(), data.size(),
                              [&](symbol_map_info_output const& output)
                              {
                                  rylang::object_symbol sym;

                                  if (output.position >= data.size())
                                  {
                                      throw std::runtime_error("Symbol position is out of range");
                                  }
                                  if (output.position_end > data.size())
                                  {
                                      throw std::runtime_error("Symbol position_end is out of range");
                                  }

                                  auto vdata = std::vector< std::uint8_t >(data.begin() + output.position, data.begin() + output.position_end);
                                  for (auto i : vdata)
                                  {
                                      sym.data.push_back(static_cast< std::byte >(i));
                                  }

                                  sym.name = output.name;

                                  sym.section = section_name;

                                  std::cout << "symbol name: " << sym.name << " at " << std::dec << output.position << " to " << std::dec << output.position_end << std::endl;

                                  for (auto const& relocation : section.relocations())
                                  {
                                      auto reloc = to_symbol_relocation(relocation);

                                      if (reloc.relocation_section != section_name)
                                      {
                                          continue;
                                      }

                                      if (reloc.relocation_offset >= output.position && reloc.relocation_offset < output.position_end)
                                      {
                                          reloc.relocation_offset -= output.position;
                                          sym.relocations.push_back(reloc);
                                      }

                                      // TODO: make this more efficient
                                  }

                                  callback(sym);
                              });
    }
}