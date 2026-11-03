// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/coroutine_frame_analysis.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace quxlang::vmir2
{
    /** Constructs a deterministic acyclic CFG for analysis contracts that do not require cycles. */
    auto coroutine_chain_input(std::size_t count) -> coroutine_frame_input
    {
        coroutine_frame_input input;
        input.nodes.resize(count);
        for (std::size_t index = 1; index < count; ++index)
        {
            input.nodes[index - 1].successors.push_back(index);
        }
        return input;
    }

    TEST(CoroutineFrameAnalysis, NonsuspendingStorageRemainsActivationLocal)
    {
        coroutine_frame_input input = coroutine_chain_input(3);
        input.header_size = 4;
        input.header_alignment = 4;
        input.objects.push_back({.size = 1024, .alignment = 64,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {2, 0, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.size, 4U);
        EXPECT_EQ(result.alignment, 4U);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::activation_local);
        EXPECT_FALSE(result.objects[0].offset.has_value());
    }

    TEST(CoroutineFrameAnalysis, CapturedArgumentPersistsWithoutBodyAwait)
    {
        coroutine_frame_input input = coroutine_chain_input(1);
        input.header_size = 1;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 1, coroutine_lifetime_event_kind::access}}, .lifetime_complete = true, .force_persistent = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.size, 16U);
        EXPECT_EQ(result.objects[0].offset, 8U);
        EXPECT_EQ(result.statistics.persistent_objects, 1U);
    }

    TEST(CoroutineFrameAnalysis, InitializationBoundaryRetainsArguments)
    {
        coroutine_frame_input input = coroutine_chain_input(2);
        input.nodes[0].may_suspend = true;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {1, 0, coroutine_lifetime_event_kind::access}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::stable_address);
        EXPECT_EQ(result.size, 8U);
    }

    TEST(CoroutineFrameAnalysis, BackedgeRetainsAValueUsedBeforeAwait)
    {
        coroutine_frame_input input = coroutine_chain_input(3);
        input.nodes[2].successors.push_back(1);
        input.nodes[2].may_suspend = true;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {1, 0, coroutine_lifetime_event_kind::access}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.component_ranks[1], result.component_ranks[2]);
        EXPECT_EQ(result.statistics.component_count, 2U);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::stable_address);
    }

    TEST(CoroutineFrameAnalysis, IrreducibleMultipleEntryCycleIsConservative)
    {
        coroutine_frame_input input;
        input.nodes = {{{1, 2}, false}, {{3}, false}, {{3}, false}, {{1, 2, 4}, true}, {{}, false}};
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {1, 0, coroutine_lifetime_event_kind::access}, {4, 0, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.component_ranks[1], result.component_ranks[2]);
        EXPECT_EQ(result.component_ranks[2], result.component_ranks[3]);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::stable_address);
    }

    TEST(CoroutineFrameAnalysis, UnknownLifetimeExitWidensToInvocation)
    {
        coroutine_frame_input input = coroutine_chain_input(3);
        input.nodes[1].may_suspend = true;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {0, 2, coroutine_lifetime_event_kind::end}}});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.objects[0].first_component, 0U);
        EXPECT_EQ(result.objects[0].last_component, 2U);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::stable_address);
    }

    TEST(CoroutineFrameAnalysis, CompleteBlockLocalLifetimeInsideSuspendingCycleStaysLocal)
    {
        coroutine_frame_input input = coroutine_chain_input(2);
        input.nodes[1].successors.push_back(0);
        input.nodes[1].may_suspend = true;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 1, coroutine_lifetime_event_kind::begin}, {0, 2, coroutine_lifetime_event_kind::access}, {0, 3, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.statistics.component_count, 1U);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::activation_local);
    }

    TEST(CoroutineFrameAnalysis, EndBeforeBeginDoesNotProveBlockLocalLifetime)
    {
        coroutine_frame_input input = coroutine_chain_input(2);
        input.nodes[1].successors.push_back(0);
        input.nodes[1].may_suspend = true;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 1, coroutine_lifetime_event_kind::end}, {0, 3, coroutine_lifetime_event_kind::begin}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::stable_address);
    }

    TEST(CoroutineFrameAnalysis, UnreachableSuspensionDoesNotRetainReachableStorage)
    {
        coroutine_frame_input input = coroutine_chain_input(3);
        input.nodes[0].successors = {2};
        input.nodes[1].may_suspend = true;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {2, 0, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        input.objects.push_back({.size = 64, .alignment = 64,
            .events = {{1, 0, coroutine_lifetime_event_kind::access}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_FALSE(result.component_ranks[1].has_value());
        EXPECT_FALSE(result.objects[1].reachable);
        EXPECT_EQ(result.size, 0U);
    }

    TEST(CoroutineFrameAnalysis, EqualComponentRanksRequireDistinctStorage)
    {
        coroutine_frame_input input = coroutine_chain_input(1);
        input.nodes[0].may_suspend = true;
        input.objects.resize(2);
        for (coroutine_frame_object& object : input.objects)
        {
            object.size = 8;
            object.alignment = 8;
        }
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.size, 16U);
        EXPECT_NE(result.objects[0].offset, result.objects[1].offset);
    }

    TEST(CoroutineFrameAnalysis, DisjointInclusiveIntervalsReuseStorage)
    {
        coroutine_frame_input input = coroutine_chain_input(4);
        input.nodes[0].may_suspend = true;
        input.nodes[2].may_suspend = true;
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {1, 0, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        input.objects.push_back({.size = 8, .alignment = 8,
            .events = {{2, 0, coroutine_lifetime_event_kind::begin}, {3, 0, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.size, 8U);
        EXPECT_EQ(result.objects[0].offset, result.objects[1].offset);
    }

    TEST(CoroutineFrameAnalysis, DensePlacementOverlaysSimultaneousSmallObjectsOnEarlierLargeObject)
    {
        coroutine_frame_input input = coroutine_chain_input(4);
        input.nodes[0].may_suspend = true;
        input.nodes[2].may_suspend = true;
        input.objects.push_back({.size = 64, .alignment = 8,
            .events = {{0, 0, coroutine_lifetime_event_kind::begin}, {1, 0, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        for (std::size_t index = 0; index < 2; ++index)
        {
            input.objects.push_back({.size = 32, .alignment = 8,
                .events = {{2, 0, coroutine_lifetime_event_kind::begin}, {3, 0, coroutine_lifetime_event_kind::end}}, .lifetime_complete = true});
        }
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.statistics.baseline_size, 128U);
        EXPECT_EQ(result.size, 64U);
        EXPECT_TRUE(result.statistics.refined_layout_selected);
        EXPECT_EQ(result.objects[0].offset, 0U);
        EXPECT_EQ(result.objects[1].offset, 0U);
        EXPECT_EQ(result.objects[2].offset, 32U);
    }

    TEST(CoroutineFrameAnalysis, RelocationAndReconstructionRequireProducerProof)
    {
        coroutine_frame_input input = coroutine_chain_input(1);
        input.nodes[0].may_suspend = true;
        input.objects.resize(4);
        for (coroutine_frame_object& object : input.objects)
        {
            object.size = 8;
            object.alignment = 8;
        }
        input.objects[0].relocatable = true;
        input.objects[1].requires_stable_address = false;
        input.objects[1].relocatable = true;
        input.objects[2].requires_stable_address = false;
        input.objects[2].rematerializable = true;
        input.objects[3].rematerializable = true;
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.objects[0].kind, coroutine_storage_kind::stable_address);
        EXPECT_EQ(result.objects[1].kind, coroutine_storage_kind::spillable);
        EXPECT_EQ(result.objects[2].kind, coroutine_storage_kind::rematerializable);
        EXPECT_FALSE(result.objects[2].offset.has_value());
        EXPECT_EQ(result.objects[3].kind, coroutine_storage_kind::stable_address);
    }

    TEST(CoroutineFrameAnalysis, ZeroBudgetPreservesConservativeLayout)
    {
        coroutine_frame_input input = coroutine_chain_input(1);
        input.nodes[0].may_suspend = true;
        input.refinement_work_limit = 0;
        input.objects.resize(2);
        input.objects[0].size = 8;
        input.objects[1].size = 8;
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.size, result.statistics.baseline_size);
        EXPECT_EQ(result.statistics.refinement_work, 0U);
        EXPECT_TRUE(result.statistics.refinement_exhausted);
    }

    TEST(CoroutineFrameAnalysis, InterruptedPlacementNeverOverlaysConflictingStorage)
    {
        for (std::uint64_t budget = 0; budget < 300; ++budget)
        {
            coroutine_frame_input input = coroutine_chain_input(1);
            input.nodes[0].may_suspend = true;
            input.refinement_work_limit = budget;
            input.objects.resize(9);
            for (std::size_t index = 0; index < input.objects.size(); ++index)
            {
                input.objects[index].size = (index + 1) * 3;
                input.objects[index].alignment = std::uint64_t{1} << (index % 4);
            }
            coroutine_frame_analysis result = analyze_coroutine_frame(input);
            EXPECT_LE(result.statistics.refinement_work, budget);
            EXPECT_LE(result.size, result.statistics.baseline_size);
            for (std::size_t first = 0; first < input.objects.size(); ++first)
            {
                ASSERT_TRUE(result.objects[first].offset.has_value());
                std::uint64_t first_offset = *result.objects[first].offset;
                EXPECT_EQ(first_offset % input.objects[first].alignment, 0U);
                for (std::size_t second = 0; second < first; ++second)
                {
                    std::uint64_t second_offset = *result.objects[second].offset;
                    EXPECT_TRUE(first_offset + input.objects[first].size <= second_offset ||
                        second_offset + input.objects[second].size <= first_offset);
                }
            }
        }
    }

    TEST(CoroutineFrameAnalysis, DeepGraphsAndDenseLifetimesHaveBoundedWork)
    {
        coroutine_frame_input input = coroutine_chain_input(20000);
        input.nodes.back().successors.push_back(0);
        input.nodes.back().may_suspend = true;
        input.objects.resize(6000);
        for (coroutine_frame_object& object : input.objects)
        {
            object.size = 8;
            object.alignment = 8;
        }
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.statistics.component_count, 1U);
        EXPECT_EQ(result.size, 48000U);
        EXPECT_LT(result.statistics.graph_visits, 12 * result.statistics.input_size);
        EXPECT_LE(result.statistics.refinement_work, result.statistics.refinement_limit);
        EXPECT_TRUE(result.statistics.refinement_exhausted);
    }

    TEST(CoroutineFrameAnalysis, InputOrderAndBudgetProduceDeterministicResults)
    {
        coroutine_frame_input input = coroutine_chain_input(5);
        input.nodes[2].may_suspend = true;
        input.objects.resize(40);
        for (std::size_t index = 0; index < input.objects.size(); ++index)
        {
            input.objects[index].size = 3 + index % 17;
            input.objects[index].alignment = std::uint64_t{1} << (index % 5);
        }
        input.refinement_work_limit = 391;
        coroutine_frame_analysis first = analyze_coroutine_frame(input);
        coroutine_frame_analysis second = analyze_coroutine_frame(input);
        EXPECT_EQ(first.component_ranks, second.component_ranks);
        EXPECT_EQ(first.size, second.size);
        EXPECT_EQ(first.statistics.refinement_work, second.statistics.refinement_work);
        for (std::size_t index = 0; index < input.objects.size(); ++index)
        {
            EXPECT_EQ(first.objects[index].offset, second.objects[index].offset);
        }
    }

    TEST(CoroutineFrameAnalysis, ZeroExtentPreservesAlignmentAndHeader)
    {
        coroutine_frame_input input;
        input.header_size = 1;
        input.objects.push_back({.size = 0, .alignment = 32, .force_persistent = true});
        coroutine_frame_analysis result = analyze_coroutine_frame(input);
        EXPECT_EQ(result.size, 32U);
        EXPECT_EQ(result.alignment, 32U);
        EXPECT_EQ(result.objects[0].offset, 32U);
    }

    TEST(CoroutineFrameAnalysis, InvalidIndicesAlignmentAndOverflowAreRejected)
    {
        coroutine_frame_input input = coroutine_chain_input(1);
        input.nodes[0].successors.push_back(1);
        EXPECT_THROW(static_cast< void >(analyze_coroutine_frame(input)), std::invalid_argument);
        input.nodes[0].successors.clear();
        input.objects.push_back({.alignment = 3});
        EXPECT_THROW(static_cast< void >(analyze_coroutine_frame(input)), std::invalid_argument);
        input.objects[0].alignment = 1;
        input.objects[0].events = {{1, 0, coroutine_lifetime_event_kind::access}};
        EXPECT_THROW(static_cast< void >(analyze_coroutine_frame(input)), std::invalid_argument);
        input.objects[0].events.clear();
        input.objects[0].force_persistent = true;
        input.objects[0].size = std::numeric_limits< std::uint64_t >::max();
        input.header_size = 1;
        EXPECT_THROW(static_cast< void >(analyze_coroutine_frame(input)), std::overflow_error);
    }
}
