// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include "linked_dwarf_sections.hpp"
#include <llvm/DWARFLinker/Parallel/DWARFLinker.h>
#include <llvm/Support/MemoryBuffer.h>
#include <mutex>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/llvm_lookup.hpp>

auto quxlang::detail::link_dwarf_sections(std::vector< relocated_dwarf_input > const& objects, machine_target_info const& machine) -> std::map< std::string, std::vector< std::byte > >
{
    std::mutex mutex;
    std::string errors;
    auto report_error = [&](llvm::Twine const& message, llvm::StringRef context, llvm::DWARFDie const*)
    {
        std::lock_guard< std::mutex > lock(mutex);
        errors += context.str() + ": " + message.str() + "\n";
    };
    std::unique_ptr< llvm::dwarf_linker::parallel::DWARFLinker > linker = llvm::dwarf_linker::parallel::DWARFLinker::createLinker(report_error, report_error);
    linker->setNumThreads(4);
    linker->setNoODR(true);
    linker->setAllowNonDeterministicOutput(false);
    linker->addAccelTableKind(llvm::dwarf_linker::DWARFLinkerBase::AccelTableKind::DebugNames);
    if (llvm::Error error = linker->setTargetDWARFVersion(5))
    {
        throw semantic_compilation_error(llvm::toString(std::move(error)));
    }
    std::map< std::string, std::vector< std::byte > > result;
    linker->setOutputDWARFHandler(llvm::Triple(lookup_llvm_triple(machine)),
                                  [&](std::shared_ptr< llvm::dwarf_linker::parallel::SectionDescriptorBase > section)
                                  {
                                      llvm::StringRef contents = section->getContents();
                                      if (contents.empty())
                                      {
                                          return;
                                      }
                                      std::lock_guard< std::mutex > lock(mutex);
                                      std::byte const* begin = reinterpret_cast< std::byte const* >(contents.data());
                                      std::vector< std::byte >& output = result[section->getName().str()];
                                      output.insert(output.end(), begin, begin + contents.size());
                                  });
    std::vector< llvm::StringMap< std::unique_ptr< llvm::MemoryBuffer > > > buffers(objects.size());
    std::vector< std::string > names(objects.size());
    std::vector< std::unique_ptr< llvm::dwarf_linker::DWARFFile > > files;
    for (std::size_t index = 0; index < objects.size(); ++index)
    {
        if (!objects[index].sections.contains("debug_info"))
        {
            continue;
        }
        for (std::pair< std::string const, std::vector< std::byte > > const& section : objects[index].sections)
        {
            buffers[index][section.first] = llvm::MemoryBuffer::getMemBufferCopy(llvm::StringRef(reinterpret_cast< char const* >(section.second.data()), section.second.size()), section.first);
        }
        names[index] = "object-" + std::to_string(index);
        files.push_back(std::make_unique< llvm::dwarf_linker::DWARFFile >(names[index], llvm::DWARFContext::create(buffers[index], machine.pointer_size_bytes()), std::make_unique< relocated_dwarf_addresses >()));
        linker->addObjectFile(*files.back());
    }
    if (llvm::Error error = linker->link())
    {
        throw semantic_compilation_error("DWARF link failed: " + llvm::toString(std::move(error)));
    }
    if (!errors.empty())
    {
        throw semantic_compilation_error("DWARF link failed: " + errors);
    }
    return result;
}
