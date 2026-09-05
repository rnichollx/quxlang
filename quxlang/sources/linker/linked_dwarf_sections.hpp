// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_LINKED_DWARF_SECTIONS_HEADER_GUARD
#define QUXLANG_LINKED_DWARF_SECTIONS_HEADER_GUARD
#include <llvm/DWARFLinker/AddressesMap.h>
#include <map>
#include <quxlang/data/machine.hpp>
#include <vector>
namespace quxlang::detail
{
    /** Debug sections from one object, with code addresses already relocated. */
    struct relocated_dwarf_input
    {
        std::map< std::string, std::vector< std::byte > > sections;
    };
    /** Supplies identity address adjustments for already relocated object debug information. */
    class relocated_dwarf_addresses final : public llvm::dwarf_linker::AddressesMap
    {
      public:
        /** All addresses presented to this map have already been resolved by the native linker. */
        bool hasValidRelocs() override
        {
            return true;
        }
        /** Retains expressions without applying code-address adjustments a second time. */
        std::optional< std::int64_t > getExprOpAddressRelocAdjustment(llvm::DWARFUnit&, llvm::DWARFExpression::Operation const&, std::uint64_t, std::uint64_t, bool) override
        {
            return 0;
        }
        /** Retains subprograms whose code addresses were relocated before DWARF linking. */
        std::optional< std::int64_t > getSubprogramRelocAdjustment(llvm::DWARFDie const&, bool) override
        {
            return 0;
        }
        /** No dynamic-library install name is attached to these executable units. */
        std::optional< llvm::StringRef > getLibraryInstallName() override
        {
            return std::nullopt;
        }
        /** Relocations are already applied to the section buffers. */
        bool applyValidRelocs(llvm::MutableArrayRef< char >, std::uint64_t, bool) override
        {
            return false;
        }
        /** Relocations need not be saved for another link. */
        bool needToSaveValidRelocs() override
        {
            return false;
        }
        /** No saved relocations need address updates. */
        void updateAndSaveValidRelocs(bool, std::uint64_t, std::int64_t, std::uint64_t, std::uint64_t) override
        {
        }
        /** No saved relocations refer to compilation-unit offsets. */
        void updateRelocationsWithUnitOffset(std::uint64_t, std::uint64_t) override
        {
        }
        /** This identity map owns no per-object state. */
        void clear() override
        {
        }
    };
    /** Rebuilds cross-unit DWARF offsets and indexes from independently relocated objects. */
    auto link_dwarf_sections(std::vector< relocated_dwarf_input > const& objects, machine_target_info const& machine) -> std::map< std::string, std::vector< std::byte > >;
} // namespace quxlang::detail
#endif
