// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_CO_COROUTINE_CLEANUP_STORAGE_HEADER_GUARD
#define QUXLANG_CO_COROUTINE_CLEANUP_STORAGE_HEADER_GUARD

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/vmir2/coroutine_vmir_analysis.hpp>

#include <array>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>
#include <vector>

namespace quxlang
{
    /** Materializes bounded cleanup bookkeeping after all semantic cleanup locals exist. */
    template < codegen_co_base CoroutineBaseType >
    class coroutine_cleanup_storage_lowering
    {
        using generator_type = co_vmir_generator2< CoroutineBaseType >;
        using block_index = vmir2::block_index;
        using local_index = vmir2::local_index;
        template < typename T >
        using co_type = typename CoroutineBaseType::template cosubroutine< T >;

        generator_type& generator;
        type_symbol byte_type = int_type{.bits = 8, .has_sign = false};
        type_symbol word_type = int_type{.bits = 64, .has_sign = false};
        std::map< vmir2::coroutine_cleanup_area, type_symbol > element_types;
        std::map< vmir2::coroutine_cleanup_area, value_index > arrays;
        std::vector< value_index > globals;
        std::optional< static_snapshot_ref > program;
        std::map< vmir2::state_map::snapshot_node const*, std::pair< vmir2::state_map::snapshot, vmir2::state_map > > augmented;
        std::size_t semantic_locals = 0;
        std::map< local_index, value_index > forwarded_identities;
        std::size_t context_capacity;
        std::set< local_index > retirement_slots;

        /** Adds the same persistent bookkeeping roots to each distinct semantic snapshot once. */
        vmir2::state_map with_storage(vmir2::state_map const& original)
        {
            typename decltype(augmented)::const_iterator existing = augmented.find(original.snapshot_root().get());
            if (existing != augmented.end()) return existing->second.second;
            vmir2::state_map result = original;
            for (std::pair< vmir2::coroutine_cleanup_area const, value_index > const& entry : arrays)
            {
                result[generator.get_local_index(entry.second)] = vmir2::slot_state{
                    .stage = vmir2::slot_stage::full, .storage_valid = true, .persistent_storage = true};
            }
            for (value_index global : globals)
                result[generator.get_local_index(global)] = vmir2::slot_state{
                    .stage = vmir2::slot_stage::full, .storage_valid = true, .persistent_storage = true};
            augmented.emplace(original.snapshot_root().get(), std::pair{original.snapshot_root(), result});
            return result;
        }

        /** Stores one retirement byte without reading the corresponding semantic object. */
        void mark(block_index block, local_index slot, std::uint8_t status)
        {
            if (static_cast< std::uint64_t >(slot) >= semantic_locals || status > 2)
                throw compiler_bug("Invalid coroutine cleanup retirement marker");
            typename decltype(forwarded_identities)::const_iterator forwarded = forwarded_identities.find(slot);
            if (forwarded == forwarded_identities.end() && !retirement_slots.contains(slot)) return;
            value_index array = arrays.at(vmir2::coroutine_cleanup_area::retirement);
            type_symbol type = generator.current_type(block, array);
            value_index reference = generator.create_reference(block, array, make_mref(type));
            value_index offset;
            if (forwarded == forwarded_identities.end()) offset = generator.create_small_uint_value(block, static_cast< std::uint64_t >(slot), word_type);
            else
            {
                value_index borrowed = generator.create_reference(block, forwarded->second, make_cref(word_type));
                offset = generator.create_local_value(word_type);
                generator.emit(block, vmir2::load_from_ref{.from_reference = generator.get_local_index(borrowed), .to_value = generator.get_local_index(offset)});
            }
            value_index target = generator.create_local_value(make_mref(byte_type));
            generator.emit(block, vmir2::access_array{.base_index = generator.get_local_index(reference),
                .index_index = generator.get_local_index(offset), .store_index = generator.get_local_index(target)});
            value_index value = generator.create_small_uint_value(block, status, byte_type);
            generator.emit(block, vmir2::store_to_ref{.from_value = generator.get_local_index(value), .to_reference = generator.get_local_index(target)});
        }

        /** Exchanges two block identities in control-flow metadata without traversing lifetime maps. */
        template < typename Value >
        static void exchange_blocks(Value& value, block_index first, block_index second)
        {
            using value_type = std::remove_cvref_t< Value >;
            if constexpr (std::is_same_v< value_type, block_index >)
            {
                if (value == first) value = second;
                else if (value == second) value = first;
            }
            else if constexpr (std::is_same_v< value_type, type_symbol > || std::is_same_v< value_type, expression > ||
                std::is_same_v< value_type, source_location > || std::is_same_v< value_type, vmir2::state_map > ||
                std::is_same_v< value_type, std::string >)
            {
            }
            else if constexpr (requires { value.template apply_visitor< void >([](auto&) {}); })
                value.template apply_visitor< void >([&](auto& item) { exchange_blocks(item, first, second); });
            else if constexpr (requires { value.tie(); })
                std::apply([&](auto&... fields) { (exchange_blocks(fields, first, second), ...); }, value.tie());
            else if constexpr (requires { value.has_value(); *value; })
            {
                if (value.has_value()) exchange_blocks(*value, first, second);
            }
            else if constexpr (requires { value.begin(); value.end(); })
            {
                for (typename value_type::value_type& item : value) exchange_blocks(item, first, second);
            }
        }

      public:
        /** Binds the generator whose completed semantic locals determine storage capacity. */
        explicit coroutine_cleanup_storage_lowering(generator_type& selected, std::vector< value_index > global_values = {},
            std::optional< static_snapshot_ref > selected_program = std::nullopt,
            std::map< local_index, value_index > selected_identities = {}, std::size_t selected_capacity = 1,
            std::set< local_index > selected_retirement_slots = {})
            : generator(selected), globals(std::move(global_values)), program(std::move(selected_program)),
              forwarded_identities(std::move(selected_identities)), context_capacity(selected_capacity), retirement_slots(std::move(selected_retirement_slots)) {}

        /** Replaces semantic storage operations with ordinary array construction, addressing, and stores. */
        auto lower() -> co_type< void >
        {
            for (typename generator_type::codegen_block const& block : generator.state.blocks)
            {
                for (vmir2::vm_instruction const& instruction : block.instructions)
                {
                    if (instruction.template type_is< vmir2::co_cleanup_mark >())
                        element_types.emplace(vmir2::coroutine_cleanup_area::retirement, byte_type);
                    if (!instruction.template type_is< vmir2::co_cleanup_storage >()) continue;
                    vmir2::co_cleanup_storage const& access = instruction.template get_as< vmir2::co_cleanup_storage >();
                    if (access.area == vmir2::coroutine_cleanup_area::program)
                    {
                        if (!program.has_value()) throw compiler_bug("Coroutine cleanup program has not been frozen");
                        continue;
                    }
                    type_symbol const& result = generator.state.locals.at(access.result).type;
                    if (!result.template type_is< ptrref_type >() || result.template get_as< ptrref_type >().ptr_class != pointer_class::array)
                        throw compiler_bug("Coroutine cleanup storage requires an array-pointer result");
                    type_symbol element = result.template get_as< ptrref_type >().target;
                    std::pair< typename decltype(element_types)::iterator, bool > inserted = element_types.emplace(access.area, element);
                    if ((!inserted.second && inserted.first->second != element) ||
                        (access.area == vmir2::coroutine_cleanup_area::retirement && element != byte_type))
                        throw compiler_bug("Inconsistent coroutine cleanup storage element type");
                }
            }
            if (element_types.empty()) co_return;
            element_types.emplace(vmir2::coroutine_cleanup_area::retirement, byte_type);
            semantic_locals = generator.state.locals.size();
            if (semantic_locals == std::numeric_limits< std::size_t >::max()) throw compiler_bug("Coroutine cleanup storage extent overflow");
            std::size_t original_blocks = generator.state.blocks.size();
            block_index entry(0);
            generator.state.catcher.reset();
            generator.state.catch_exception.reset();
            generator.state.catch_region.reset();
            generator.state.coroutine_handler_root.reset();
            generator.state.coroutine_handler_exception.reset();
            block_index prologue = generator.generate_subblock(entry, "coroutine_cleanup_storage");
            block_index current = prologue;
            generator.state.blocks.at(prologue).entry_state = generator.state.blocks.at(entry).entry_state;
            generator.state.blocks.at(prologue).current_state = generator.state.blocks.at(prologue).entry_state;
            for (value_index global : globals)
            {
                local_index local = generator.get_local_index(global);
                type_symbol const& type = generator.state.locals.at(local).type;
                if (!type.template type_is< int_type >() && !type.template type_is< ptrref_type >() && !type.template type_is< bool_type >())
                    throw compiler_bug("Coroutine cleanup global requires scalar storage");
                generator.emit(current, vmir2::load_const_zero{.target = local});
                generator.state.coroutine->cleanup_retained.insert(local);
            }
            for (std::pair< vmir2::coroutine_cleanup_area const, type_symbol > const& area : element_types)
            {
                type_symbol type = array_type{.element_type = area.second,
                    .element_count = expression_numeric_literal{.value = std::to_string(
                        area.first == vmir2::coroutine_cleanup_area::contexts ? context_capacity : semantic_locals + 1)}};
                value_index array = co_await generator.co_gen_call_ctor(current, type, {});
                arrays.emplace(area.first, array);
                generator.state.coroutine->cleanup_retained.insert(generator.get_local_index(array));
                std::optional< type_symbol > destructor = co_await generator.co_select_default_destructor_entry(type, false);
                if (destructor.has_value()) generator.state.non_trivial_dtors.emplace(type, *destructor);
            }
            for (std::size_t ordinal = 0; ordinal < original_blocks; ++ordinal)
            {
                block_index block(ordinal);
                typename generator_type::codegen_block& target = generator.state.blocks.at(block);
                target.entry_state = with_storage(target.entry_state);
                target.current_state = target.entry_state;
                std::vector< vmir2::vm_instruction > instructions = std::move(target.instructions);
                target.instructions.clear();
                generator.state.catcher = target.catcher;
                generator.state.catch_exception = target.catch_exception;
                generator.state.catch_region = target.catch_region;
                generator.state.coroutine_handler_root = target.coroutine_handler_root;
                generator.state.coroutine_handler_exception = target.coroutine_handler_exception;
                for (vmir2::vm_instruction instruction : instructions)
                {
                    if (instruction.template type_is< vmir2::co_cleanup_storage >())
                    {
                        vmir2::co_cleanup_storage const access = instruction.template get_as< vmir2::co_cleanup_storage >();
                        type_symbol type;
                        value_index reference;
                        if (access.area == vmir2::coroutine_cleanup_area::program)
                        {
                            type = generator.state.static_snapshots.at(*program).type;
                            reference = generator.create_local_value(make_cref(type));
                            generator.emit(block, vmir2::get_antestatal_ref{.symbol = *program, .target_ref = generator.get_local_index(reference)});
                        }
                        else
                        {
                            value_index array = arrays.at(access.area);
                            type = generator.current_type(block, array);
                            reference = generator.create_reference(block, array, make_mref(type));
                        }
                        value_index pointer = co_await generator.co_gen_call_functum(block, submember{.of = type, .name = "BEGIN"},
                            codegen_invocation_args{.named = {{"THIS", reference}}});
                        generator.emit(block, vmir2::cast_ptrref{.source_index = generator.get_local_index(pointer), .target_index = access.result});
                        continue;
                    }
                    if (instruction.template type_is< vmir2::co_cleanup_mark >())
                    {
                        vmir2::co_cleanup_mark const& selected = instruction.template get_as< vmir2::co_cleanup_mark >();
                        mark(block, selected.slot, selected.status);
                        continue;
                    }
                    bool transfer = instruction.template type_is< vmir2::lifetime_transfer >() || instruction.template type_is< vmir2::lifetime_import >();
                    if (instruction.template type_is< vmir2::lifetime_transfer >())
                        instruction.template get_as< vmir2::lifetime_transfer >().state = with_storage(instruction.template get_as< vmir2::lifetime_transfer >().state);
                    std::vector< local_index > touched = vmir2::coroutine_instruction_locals(instruction, generator.state.blocks.at(block).current_state);
                    std::vector< bool > initialized;
                    initialized.reserve(touched.size());
                    for (local_index local : touched)
                    {
                        vmir2::state_map const& state = generator.state.blocks.at(block).current_state;
                        vmir2::state_map::const_iterator previous = state.find(local);
                        initialized.push_back(previous != state.end() && (previous->second.alive() || previous->second.storage_valid));
                    }
                    if (instruction.template type_is< vmir2::destroy >()) mark(block, instruction.template get_as< vmir2::destroy >().of, 1);
                    generator.emit(block, instruction);
                    if (instruction.template type_is< vmir2::begin_destruction >()) mark(block, instruction.template get_as< vmir2::begin_destruction >().of, 1);
                    if (instruction.template type_is< vmir2::end_lifetime >()) mark(block, instruction.template get_as< vmir2::end_lifetime >().of, 2);
                    if (instruction.template type_is< vmir2::destroy >()) mark(block, instruction.template get_as< vmir2::destroy >().of, 2);
                    if (transfer) continue;
                    for (std::size_t position = 0; position < touched.size(); ++position)
                    {
                        local_index local = touched[position];
                        if (static_cast< std::uint64_t >(local) >= semantic_locals) continue;
                        vmir2::state_map const& state = generator.state.blocks.at(block).current_state;
                        vmir2::state_map::const_iterator after = state.find(local);
                        // Uninitialized storage can already own a retained address that cleanup must retire.
                        bool active = after != state.end() && (after->second.alive() || after->second.storage_valid);
                        if (!initialized[position] && active) mark(block, local, 0);
                        else if (initialized[position] && !active) mark(block, local, 2);
                    }
                }
                if (generator.state.blocks.at(block).terminator.has_value() &&
                    generator.state.blocks.at(block).terminator->template type_is< vmir2::initguard_try_acquire >())
                {
                    // The lock identity is inactive before either acquisition outcome.
                    mark(block, generator.state.blocks.at(block).terminator->template get_as< vmir2::initguard_try_acquire >().target_lock, 0);
                }
                if (generator.state.blocks.at(block).terminator.has_value() &&
                    generator.state.blocks.at(block).terminator->template type_is< vmir2::co_complete >())
                {
                    for (std::pair< vmir2::coroutine_cleanup_area const, value_index > const& array : arrays)
                        generator.emit(block, vmir2::destroy{.of = generator.get_local_index(array.second)});
                    for (value_index global : globals) generator.emit(block, vmir2::end_lifetime{.of = generator.get_local_index(global)});
                }
            }
            generator.generate_jump(current, entry);
            std::swap(generator.state.blocks.at(entry), generator.state.blocks.at(prologue));
            for (typename generator_type::codegen_block& block : generator.state.blocks)
            {
                exchange_blocks(block.terminator, entry, prologue);
                exchange_blocks(block.catcher, entry, prologue);
                exchange_blocks(block.catch_region, entry, prologue);
            }
            exchange_blocks(generator.state.coroutine->continuations, entry, prologue);
            co_return;
        }
    };
}

#endif // QUXLANG_CO_COROUTINE_CLEANUP_STORAGE_HEADER_GUARD
