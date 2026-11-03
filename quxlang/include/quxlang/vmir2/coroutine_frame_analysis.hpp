// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_COROUTINE_FRAME_ANALYSIS_HEADER_GUARD
#define QUXLANG_VMIR2_COROUTINE_FRAME_ANALYSIS_HEADER_GUARD

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace quxlang::vmir2
{
    /** Identifies a storage obligation within the logical coroutine control-flow graph. */
    enum class coroutine_lifetime_event_kind : std::uint8_t
    {
        begin,
        access,
        end
    };

    /** Locates a storage obligation before driver dispatch and frame-reuse lowering. */
    struct coroutine_lifetime_event
    {
        std::size_t node = 0;
        std::size_t instruction_index = 0;
        coroutine_lifetime_event_kind kind = coroutine_lifetime_event_kind::access;
    };

    /** Describes a logical basic block, including resume, cleanup, and exceptional successors. */
    struct coroutine_cfg_node
    {
        std::vector< std::size_t > successors;
        bool may_suspend = false;
    };

    /** Describes one logical object's physical requirements and complete or conservative lifetime evidence. */
    struct coroutine_frame_object
    {
        std::uint64_t size = 0;
        std::uint64_t alignment = 1;
        std::vector< coroutine_lifetime_event > events;
        /// True only when events cover every required use, borrow, and lifetime exit on every path.
        bool lifetime_complete = false;
        /// Stable-address objects must occupy their final storage throughout the required lifetime.
        bool requires_stable_address = true;
        /// The producer has proved relocation legal without observable lifecycle operations.
        bool relocatable = false;
        /// The producer has proved reconstruction preserves semantics without retaining storage.
        bool rematerializable = false;
        /// Requires frame storage independently of body suspension, including captured arguments.
        bool force_persistent = false;
    };

    /** Supplies one invocation's compact semantic CFG and target-specific storage requirements. */
    struct coroutine_frame_input
    {
        std::vector< coroutine_cfg_node > nodes;
        std::size_t entry_node = 0;
        std::vector< coroutine_frame_object > objects;
        std::uint64_t header_size = 0;
        std::uint64_t header_alignment = 1;
        std::uint64_t maximum_frame_size = std::numeric_limits< std::uint64_t >::max();
        /// Optional tighter work limit for deterministic compiler experiments and algorithm tests.
        std::optional< std::uint64_t > refinement_work_limit;
    };

    /** Classifies the storage that lowering must preserve across activations. */
    enum class coroutine_storage_kind : std::uint8_t
    {
        activation_local,
        rematerializable,
        spillable,
        stable_address
    };

    /** Records an inclusive conservative storage interval and optional physical frame placement. */
    struct coroutine_object_placement
    {
        coroutine_storage_kind kind = coroutine_storage_kind::activation_local;
        bool reachable = false;
        std::size_t first_component = 0;
        std::size_t last_component = 0;
        std::optional< std::uint64_t > offset;
    };

    /** Reports deterministic analysis work and layout quality independently of wall-clock timing. */
    struct coroutine_frame_statistics
    {
        std::uint64_t input_size = 0;
        std::uint64_t graph_visits = 0;
        std::uint64_t lifetime_visits = 0;
        std::uint64_t refinement_limit = 0;
        std::uint64_t refinement_work = 0;
        std::uint64_t baseline_size = 0;
        std::size_t component_count = 0;
        std::size_t persistent_objects = 0;
        std::size_t baseline_cells = 0;
        bool refinement_exhausted = false;
        bool refined_layout_selected = false;
    };

    /** Contains canonical component ranks and one placement per input object. */
    struct coroutine_frame_analysis
    {
        /// Unreachable logical blocks have no component rank.
        std::vector< std::optional< std::size_t > > component_ranks;
        std::vector< coroutine_object_placement > objects;
        std::uint64_t size = 0;
        std::uint64_t alignment = 1;
        coroutine_frame_statistics statistics;
    };

    /** Returns the fixed deterministic allowance shared by optional coroutine refinements. */
    [[nodiscard]] std::uint64_t coroutine_refinement_work_limit(std::uint64_t input_size);

    /**
     * Computes a deterministic conservative frame layout in O(N log N) time and O(N) space.
     * The graph describes one logical invocation and includes initialization-to-resume boundaries.
     * Missing lifetime evidence conservatively retains storage for the entire invocation.
     * Optional dense placement shares a fixed O(N log N) work budget and linear scratch storage.
     * Invalid graph indices or non-power-of-two alignments throw std::invalid_argument.
     * A baseline layout exceeding maximum_frame_size throws std::overflow_error.
     */
    [[nodiscard]] auto analyze_coroutine_frame(coroutine_frame_input const& input) -> coroutine_frame_analysis;
}

#endif // QUXLANG_VMIR2_COROUTINE_FRAME_ANALYSIS_HEADER_GUARD
