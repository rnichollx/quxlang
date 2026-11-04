// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_ROUTINE_SNAPSHOT_ARCHIVE_HEADER_GUARD
#define QUXLANG_VMIR2_ROUTINE_SNAPSHOT_ARCHIVE_HEADER_GUARD

#include <quxlang/vmir2/lifetime_snapshot_archive.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include <rpnx/serialization4.hpp>

namespace quxlang::vmir2
{
    /** Replaces shared variable-size slot metadata with archive-local indices. */
    struct archived_lifetime_slot
    {
        slot_state scalar;
        std::optional< std::size_t > destructor;
        std::optional< std::size_t > struct_delegates;
        std::optional< std::size_t > array_delegates;
        std::optional< std::size_t > array_elements;

        RPNX_MEMBER_METADATA(archived_lifetime_slot, scalar, destructor, struct_delegates, array_delegates, array_elements);
    };

    /** Stores a routine and its shared lifetime graphs without expanding block live sets. */
    struct routine_snapshot_archive
    {
        std::optional< coroutine_body > coroutine;
        bool is_noexcept = false;
        std::vector< local_type > local_types;
        routine_parameters parameters;
        /// Block instructions remain in source order; their entry states are represented by roots.
        std::vector< executable_block > blocks;
        std::map< block_index, std::string > block_names;
        std::map< type_symbol, type_symbol > non_trivial_dtors;
        std::set< local_index > asynchronous_cleanup_slots;
        std::map< static_snapshot_ref, localdata_entry > static_snapshots;
        std::map< local_index, retained_storage_binding > retained_bindings;
        std::vector< std::optional< std::size_t > > roots;
        std::vector< lifetime_snapshot_record< local_index, archived_lifetime_slot > > states;
        std::vector< dtor_spec > destructors;
        std::vector< std::vector< struct_init_delegate > > struct_delegates;
        std::vector< invocation_args > array_delegates;
        std::vector< lifetime_snapshot_record< local_index, bool > > array_elements;

        RPNX_MEMBER_METADATA(routine_snapshot_archive, coroutine, is_noexcept, local_types, parameters, blocks, block_names, non_trivial_dtors, asynchronous_cleanup_slots,
            static_snapshots, retained_bindings, roots, states, destructors, struct_delegates, array_delegates, array_elements);
    };

    /** Captures the routine's immutable lifetime DAGs and shared delegate payloads. */
    auto archive_routine_snapshots(functanoid_routine3 const& routine) -> routine_snapshot_archive;

    /** Restores a routine after validating its graph roots and metadata references. */
    auto restore_routine_snapshots(routine_snapshot_archive archive) -> functanoid_routine3;
}

namespace rpnx::serial4
{
    /** Serializes VMIR lifetime snapshots by shared node identity within each routine. */
    template <>
    class binary_serial_traits< quxlang::vmir2::functanoid_routine3 >
    {
      public:
        template < typename It >
        static auto serialize_iter(quxlang::vmir2::functanoid_routine3 const& input, It output)
        {
            return serial4::serialize_iter(quxlang::vmir2::archive_routine_snapshots(input), output);
        }

        template < typename It >
        static auto serialize_iter(quxlang::vmir2::functanoid_routine3 const& input, It output, It end)
        {
            return serial4::serialize_iter(quxlang::vmir2::archive_routine_snapshots(input), output, end);
        }

        template < typename It >
        static auto deserialize_iter(quxlang::vmir2::functanoid_routine3& output, It input)
        {
            quxlang::vmir2::routine_snapshot_archive archive;
            It result = serial4::deserialize_iter(archive, input);
            output = quxlang::vmir2::restore_routine_snapshots(std::move(archive));
            return result;
        }

        template < typename It >
        static auto deserialize_iter(quxlang::vmir2::functanoid_routine3& output, It input, It end)
        {
            quxlang::vmir2::routine_snapshot_archive archive;
            It result = serial4::deserialize_iter(archive, input, end);
            output = quxlang::vmir2::restore_routine_snapshots(std::move(archive));
            return result;
        }
    };

    /** Includes the compact routine wire representation in cache type signatures. */
    template <>
    struct typesig_hashing_traits< quxlang::vmir2::functanoid_routine3 >
    {
        template < typename Hasher >
        static constexpr void partial_hash(detail::typesig16_hashing_context& context, Hasher& hash)
        {
            typesig_hashing_traits< quxlang::vmir2::routine_snapshot_archive >::partial_hash(context, hash);
        }
    };

    /** Uses graph records in JSON diagnostics to preserve represented metadata size. */
    template <>
    class json_serialization_traits< quxlang::vmir2::functanoid_routine3 >
    {
      public:
        template < typename It >
        static auto serialize_iter(quxlang::vmir2::functanoid_routine3 const& input, It output)
        {
            return serial4::json_serialize_iter(quxlang::vmir2::archive_routine_snapshots(input), output);
        }
    };
}

#endif // QUXLANG_VMIR2_ROUTINE_SNAPSHOT_ARCHIVE_HEADER_GUARD
