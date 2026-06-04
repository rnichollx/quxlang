// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/vmir2/assembler.hpp>

#include <algorithm>
#include <stdexcept>

namespace quxlang::vmir2
{
    /// Returns the assembly spelling for a VMIR atomic access mode.
    auto atomic_access_mode_assembly_name(atomic_access_mode mode) -> std::string
    {
        switch (mode)
        {
        case atomic_access_mode::nonatomic:
            return "NONATOMIC";
        case atomic_access_mode::atomic_relaxed:
            return "ATOMIC_RELAXED";
        case atomic_access_mode::atomic_release:
            return "ATOMIC_RELEASE";
        case atomic_access_mode::atomic_acquire:
            return "ATOMIC_ACQUIRE";
        case atomic_access_mode::atomic_acqrel:
            return "ATOMIC_ACQREL";
        case atomic_access_mode::atomic_seqcst:
            return "ATOMIC_SEQCST";
        }
        throw std::logic_error("unknown atomic access mode");
    }

    /// Formats an optional old-value output for read-modify-write instructions.
    auto rmw_old_value_suffix(std::optional< local_index > old_value) -> std::string
    {
        if (!old_value.has_value())
        {
            return "";
        }
        return " -> %" + std::to_string(*old_value);
    }

    /// Formats the atomic-mode suffix shared by atomic-capable VMIR instructions.
    auto atomic_access_mode_suffix(atomic_access_mode mode) -> std::string
    {
        return " [" + atomic_access_mode_assembly_name(mode) + "]";
    }

    indexed_source_file::indexed_source_file(source_file_name name, std::string contents) : name(std::move(name)), contents(std::move(contents))
    {
        this->line_starts.push_back(0);
        for (std::size_t i = 0; i < this->contents.size(); ++i)
        {
            if (this->contents[i] == '\n')
            {
                this->line_starts.push_back(i + 1);
            }
        }
    }

    auto indexed_source_file::path() const -> std::string
    {
        if (this->name.source_module.empty())
        {
            return this->name.relative_path;
        }
        if (this->name.relative_path.empty())
        {
            return this->name.source_module;
        }

        auto const bundle_module_prefix = "modules/" + this->name.source_module + "/sources/";
        if (this->name.relative_path.starts_with(bundle_module_prefix))
        {
            return this->name.relative_path;
        }

        return this->name.source_module + "/" + this->name.relative_path;
    }

    auto indexed_source_file::position(std::size_t offset) const -> source_position
    {
        offset = std::min(offset, this->contents.size());
        auto line_iter = std::upper_bound(this->line_starts.begin(), this->line_starts.end(), offset);
        auto const line_index = static_cast< std::size_t >(std::distance(this->line_starts.begin(), line_iter) - 1);
        return source_position{
            .line = line_index + 1,
            .column = offset - this->line_starts.at(line_index) + 1,
        };
    }

    source_index::source_index(source_file_index const& file_index, source_bundle const& bundle)
    {
        for (auto const& [file_id, name] : file_index.id_to_file)
        {
            auto module_iter = bundle.module_sources.find(name.source_module);
            if (module_iter == bundle.module_sources.end())
            {
                throw quxlang::compiler_bug("source_index received a source file index with an unknown source module");
            }

            auto file_iter = module_iter->second.files.find(name.relative_path);
            if (file_iter == module_iter->second.files.end())
            {
                throw quxlang::compiler_bug("source_index received a source file index with an unknown relative path");
            }

            this->files.emplace(file_id, indexed_source_file{name, file_iter->second->contents});
        }
    }

    auto source_index::format(std::optional< source_location > const& location) const -> std::string
    {
        if (!location.has_value())
        {
            return {};
        }

        auto file_iter = this->files.find(location->file_id);
        if (file_iter == this->files.end())
        {
            return quxlang::source_location_suffix(location);
        }

        auto const begin = file_iter->second.position(location->begin_index);
        std::string result = " @@ " + file_iter->second.path() + ":" + std::to_string(begin.line) + ":" + std::to_string(begin.column);
        if (location->end_index.has_value())
        {
            auto const end = file_iter->second.position(*location->end_index);
            result += "," + std::to_string(end.line) + ":" + std::to_string(end.column);
        }
        return result;
    }

    std::string assembler::source_location_suffix(std::optional< source_location > const& location) const
    {
        if (this->source_index.has_value())
        {
            return this->source_index->format(location);
        }
        return quxlang::source_location_suffix(location);
    }

    std::string assembler::append_source_location_suffix(std::string result, std::optional< source_location > const& location) const
    {
        auto suffix = this->source_location_suffix(location);
        if (suffix.empty())
        {
            return result;
        }

        auto const comment_position = result.find(" //");
        if (comment_position == std::string::npos)
        {
            result += suffix;
            return result;
        }

        result.insert(comment_position, suffix);
        return result;
    }

    std::string assembler::to_string(vmir2::functanoid_routine3 fnc)
    {
        std::string output;

        static const std::string indent = "    ";
        output += "[DTors]:\n";

        for (auto const& [type, dtor] : fnc.non_trivial_dtors)
        {
            output += indent + quxlang::to_string(type) + " USES " + quxlang::to_string(dtor) + "\n";
        }

        output += "[Slots]:\n";

        for (std::size_t i = 0; i < fnc.local_types.size(); i++)
        {
            output += indent + std::to_string(i) + ": " + this->to_string(fnc.local_types.at(i));
            output += "\n";
        }

        output += "[Parameters]:\n";
        output += indent + quxlang::to_string(fnc.parameters) + "\n";

        output += "[Blocks]:\n";

        for (block_index i = block_index(0); i < fnc.blocks.size(); i++)
        {
            std::string block_name;
            if (fnc.block_names.contains(i))
            {
                block_name = "BLOCK" + std::to_string(i) + "[" + fnc.block_names.at(i) + "]";
            }
            else
            {
                block_name = "BLOCK" + std::to_string(i);
            }

            output += block_name + " " + to_string(fnc.blocks.at(i).entry_state);
            if (print_comments && fnc.blocks.at(i).dbg_name.has_value())
            {
                output += " // " + fnc.blocks.at(i).dbg_name.value();
            }
            output += "\n";
            output += this->to_string(fnc.blocks.at(i));
            output += "\n";
        }

        return output;
    }
    std::string assembler::to_string(vmir2::executable_block const& inst)
    {

        state = inst.entry_state;

        std::string output;
        static const std::string indent = "    ";
        for (auto& i : inst.instructions)
        {
            output += indent + this->to_string(i);
            output += "\n";

            try
            {
                auto const& what = m_what;
                codegen_state_engine(this->state, what.local_types, {}).apply(i);
                if (print_states)
                {
                    output += indent + "// state: " + this->to_string(this->state) + "\n";
                }
            }
            catch (std::exception const& e)
            {
                if (print_states)
                {
                    output += indent + "// state: exception: " + e.what() + "\n";
                }

            }
        }
        if (!inst.terminator.has_value())
        {
            output += indent + "MISSING_TERMINATOR\n";
        }
        else
        {
            output += indent + this->to_string(inst.terminator.value()) + "\n";
        }
        return output;
    }
    std::string assembler::to_string(vmir2::state_map const& state)
    {
        std::string output;
        output += " [< ";
        bool first = true;
        for (auto& [k, v] : state)
        {
            output += this->to_string(k, v);
        }

        output += " >]";
        return output;
    }
    std::string assembler::to_string(std::size_t index, vmir2::slot_state const& v)
    {
        std::string output;
        bool first = true;
        if (v.alive() || v.storage_valid)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += "%" + std::to_string(index);

            if (v.stage == slot_stage::full)
            {
                output += "[A]";
            }
            else if (v.stage == slot_stage::partial)
            {
                output += "[P]";
            }
            else if (v.storage_valid)
            {
                output += "[S]";
            }

            if (v.delegate_of)
            {
                output += "[D <- %" + std::to_string(*v.delegate_of) + "]";
            }

            if (v.array_delegate_of_initializer)
            {
                output += "[D <== %" + std::to_string(*v.array_delegate_of_initializer) + "]";
            }
            if (v.delegates.has_value())
            {
                auto const& d = v.delegates.value();
                for (auto const& [name, idx] : d.named)
                {
                    output += "[D @" + name + " -> %" + std::to_string(idx) + "]";
                }
                std::size_t i = 0;
                for (auto const& idx : d.positional)
                {

                    output += "[D %["+std::to_string(i++) + "] -> %" + std::to_string(idx) + "]";
                }
            }
        }
        return output;
    }
    std::string assembler::to_string(vmir2::vm_instruction inst)
    {
        auto result = rpnx::apply_visitor< std::string >(
            inst,
            [&](auto&& x)
            {
                return this->to_string_internal(x);
            });
        return this->append_source_location_suffix(result, vmir2::get_location(inst));
    }

    std::string assembler::to_string(vmir2::vm_terminator inst)
    {
        auto result = rpnx::apply_visitor< std::string >(
            inst,
            [&](auto&& x)
            {
                return this->to_string_internal(x);
            });
        return this->append_source_location_suffix(result, vmir2::get_location(inst));
    }

    std::string assembler::to_string(vmir2::local_type lct)
    {
        return quxlang::to_string(lct.type);
    }

    std::string assembler::to_string(vmir2::vm_slot slt)
    {
        std::string output;

        switch (slt.kind)
        {
        case vmir2::slot_kind::local:
            output = "LOCAL";
            break;
        case vmir2::slot_kind::positional_arg:
        case vmir2::slot_kind::named_arg:
            output = "ARG";
            break;
        case vmir2::slot_kind::binding:
            output = "BINDING";
            break;
        case vmir2::slot_kind::literal:
            output = "LITERAL";
            break;
        default:
            throw quxlang::compiler_bug("Invalid slot kind");
        }

        output += " " + quxlang::to_string(slt.type);

        if (slt.literal_value)
        {
            output += " " + *slt.literal_value;
        }

        if (slt.binding_of)
        {
            output += " BINDS %" + std::to_string(*slt.binding_of);
        }

        if (print_comments && slt.name)
        {
            output += " // " + *slt.name;
        }

        return output;
    }

    std::string assembler::to_string_internal(vmir2::storage_init inst)
    {
        return "STORAGE_INIT %" + std::to_string(inst.storage);
    }

    std::string assembler::to_string_internal(vmir2::storage_init_start inst)
    {
        return "STORAGE_INIT_START %" + std::to_string(inst.on_storage) + " -> %" + std::to_string(inst.target_value);
    }

    std::string assembler::to_string_internal(vmir2::storage_deinit_start inst)
    {
        return "STORAGE_DEINIT_START %" + std::to_string(inst.on_storage) + " -> %" + std::to_string(inst.target_value);
    }

    std::string assembler::to_string_internal(vmir2::storage_pun inst)
    {
        return "STORAGE_PUN %" + std::to_string(inst.from_storage) + " AS " + quxlang::to_string(inst.as_type) + " -> %" + std::to_string(inst.to_reference);
    }

    std::string assembler::to_string_internal(vmir2::constexpr_alloc inst)
    {
        return "CONSTEXPR_ALLOC " + quxlang::to_string(inst.storage_type) + " -> %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::constexpr_alloc_multiple inst)
    {
        return "CONSTEXPR_ALLOC_MULTIPLE " + quxlang::to_string(inst.storage_type) + ", %" + std::to_string(inst.count) + " -> %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::constexpr_dealloc inst)
    {
        return "CONSTEXPR_DEALLOC " + quxlang::to_string(inst.storage_type) + ", %" + std::to_string(inst.pointer);
    }

    std::string assembler::to_string_internal(vmir2::constexpr_dealloc_multiple inst)
    {
        return "CONSTEXPR_DEALLOC_MULTIPLE " + quxlang::to_string(inst.storage_type) + ", %" + std::to_string(inst.pointer) + ", %" + std::to_string(inst.count);
    }

    std::string assembler::to_string_internal(vmir2::get_global_storage inst)
    {
        return "GET_GLOBAL_STORAGE " + quxlang::to_string(inst.symbol) + " -> %" + std::to_string(inst.target_ref);
    }

    std::string assembler::to_string_internal(vmir2::get_global_ref inst)
    {
        return "GET_GLOBAL_REF " + quxlang::to_string(inst.symbol) + " -> %" + std::to_string(inst.target_ref);
    }

    std::string assembler::to_string_internal(vmir2::get_antestatal_ref inst)
    {
        return "GET_ANTESTATAL_REF " + quxlang::to_string(inst.symbol) + " -> %" + std::to_string(inst.target_ref);
    }

    std::string assembler::to_string_internal(vmir2::initguard_global_get_ref inst)
    {
        return "INITGUARD_GLOBAL_GET_REF " + quxlang::to_string(inst.symbol) + " -> %" + std::to_string(inst.target_ref);
    }

    std::string assembler::to_string_internal(vmir2::initguard_release inst)
    {
        return "INITGUARD_RELEASE %" + std::to_string(inst.lock);
    }

    std::string assembler::to_string_internal(vmir2::initguard_abort inst)
    {
        return "INITGUARD_ABORT %" + std::to_string(inst.lock);
    }

    std::string assembler::to_string_internal(vmir2::access_field inst)
    {
        std::string result = "ACCESS_FIELD %" + std::to_string(inst.base_index) + ", %" + std::to_string(inst.store_index) + ", " + inst.field_name;
        // Use apply_visitor since both types have the same logic

        if (print_comments)
        {
            result += " // type1=";
            result += quxlang::to_string(m_what.local_types.at(inst.base_index).type);
            result += " type2=";
            result += quxlang::to_string(m_what.local_types.at(inst.store_index).type);
        }

        return result;
    }

    std::string assembler::to_string_internal(vmir2::decrement inst)
    {
        return "DEC %" + std::to_string(inst.value) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::preincrement inst)
    {
        return "PREINC %" + std::to_string(inst.target) + ", %" + std::to_string(inst.target2);
    }

    std::string assembler::to_string_internal(vmir2::predecrement inst)
    {
        return "PREDEC %" + std::to_string(inst.target) + ", %" + std::to_string(inst.target2);
    }

    std::string assembler::to_string_internal(vmir2::assert_instr const& asrt)
    {
        std::string message;
        // TODO: Escape the message so it wont look weird if it has quotes in it
        message = "\"" + asrt.message + "\"";

        return "ASSERT %" + std::to_string(asrt.condition) + ", " + message;
    }
    std::string assembler::to_string_internal(vmir2::increment inst)
    {
        return "INC %" + std::to_string(inst.value) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::to_bool inst)
    {
        std::string result = "TOBOOL %" + std::to_string(inst.from) + ", %" + std::to_string(inst.to);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::iconv inst)
    {
        std::string result = "ICONV %" + std::to_string(inst.from) + ", %" + std::to_string(inst.to);
        result += ", ";
        result += rpnx::enum_traits<conversion_class>::to_string(inst.convtype);
        // Helpful comment with types
        if (print_comments)
        {
            result += " // from=" + quxlang::to_string(m_what.local_types.at(inst.from).type);
            result += " to=" + quxlang::to_string(m_what.local_types.at(inst.to).type);
        }
        return result;
    }

    std::string assembler::to_string_internal(vmir2::unimplemented unimpl)
    {
        std::string result = "UNIMPLEMENTED";
        if (print_comments && unimpl.message.has_value())
        {
            result += " // " + unimpl.message.value();
        }
        return result;
    }



    std::string assembler::to_string_internal(vmir2::to_bool_not inst)
    {
        std::string result = "TBN %" + std::to_string(inst.from) + ", %" + std::to_string(inst.to);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::array_init_start ani)
    {
        return std::string("ARRAY_INIT_START %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.on_value);
    }

    std::string assembler::to_string_internal(vmir2::array_init_index ani)
    {
        return std::string("ARRAY_INIT_INDEX %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.result);
    }


    std::string assembler::to_string_internal(vmir2::array_init_more ani)
    {
        return std::string("ARRAY_INIT_MORE %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.result);
    }

    std::string assembler::to_string_internal(vmir2::array_init_element ani)
    {
        return std::string("ARRAY_INIT_ELEMENT %") + std::to_string(ani.initializer) + ", %" + std::to_string(ani.target);
    }

    std::string assembler::to_string_internal(vmir2::array_init_finish ani)
    {
        return std::string("ARRAY_INIT_FINISH %") + std::to_string(ani.initializer);
    }



    std::string assembler::to_string_internal(vmir2::access_array inst)
    {
        std::string result;
        result += "ACCESS_ARRAY %" + std::to_string(inst.base_index) + ", %" + std::to_string(inst.index_index) + ", %" + std::to_string(inst.store_index);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::access_pointer inst)
    {
        std::string result;
        result += "ACCESS_POINTER %" + std::to_string(inst.base_index) + ", %" + std::to_string(inst.index_index) + ", %" + std::to_string(inst.store_index);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::invoke inst)
    {
        return "INVOKE " + quxlang::to_string(inst.what) + ", " + this->to_string_internal(inst.args);
    }

    std::string assembler::to_string_internal(vmir2::interface_init inst)
    {
        std::string output = "INTERFACE_INIT %" + std::to_string(inst.target) + ", " + quxlang::to_string(inst.interface_type);
        if (inst.is_default)
        {
            output += ", DEFAULT";
            return output;
        }

        output += ", [";
        bool first = true;
        for (std::pair< interface_slot_key const, type_symbol > const& entry : inst.functions)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;
            interface_slot_key const& slot = entry.first;
            type_symbol const& function = entry.second;
            output += quxlang::to_string(slot) + " = " + quxlang::to_string(function);
        }
        output += "]";
        return output;
    }

    std::string assembler::to_string_internal(vmir2::interface_invoke inst)
    {
        std::string output = "INTERFACE_INVOKE %" + std::to_string(inst.interface_value) + ", " + quxlang::to_string(inst.slot) + ", " + this->to_string_internal(inst.args);
        if (inst.default_function.has_value())
        {
            output += " DEFAULT " + quxlang::to_string(*inst.default_function);
        }
        return output;
    }

    std::string assembler::to_string_internal(vmir2::interface_is_default inst)
    {
        return "INTERFACE_IS_DEFAULT %" + std::to_string(inst.interface_value) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::invoke_indirect inst)
    {
        return "INVOKE_INDIRECT %" + std::to_string(inst.what_index) + ", " + this->to_string_internal(inst.args);
    }

    std::string assembler::to_string_internal(vmir2::get_procedure_ptr inst)
    {
        return "GET_PROCEDURE_PTR " + quxlang::to_string(inst.routine) + ", " + inst.calling_convention + ", %" + std::to_string(inst.pointer_index);
    }

    std::string assembler::to_string_internal(vmir2::make_reference inst)
    {
        std::string result = "MAKEREF %" + std::to_string(inst.value_index) + ", %" + std::to_string(inst.reference_index);

        // Use apply_visitor since both types have the same logic

        if (print_comments)
        {
            result += " // type1=";
            result += quxlang::to_string(m_what.local_types.at(inst.value_index).type);
            result += " type2=";
            result += quxlang::to_string(m_what.local_types.at(inst.reference_index).type);
        }

        return result;
    }

    std::string assembler::to_string_internal(vmir2::cast_ptrref inst)
    {
        std::string result = "CONVERT_PTRREF %" + std::to_string(inst.source_index) + ", %" + std::to_string(inst.target_index);

        // Use apply_visitor since both types have the same logic

        if (print_comments)
        {
            result += " // type1=";
            result += quxlang::to_string(m_what.local_types.at(inst.source_index).type);
            result += " type2=";
            result += quxlang::to_string(m_what.local_types.at(inst.target_index).type);
        }

        return result;
    }
    std::string assembler::to_string_internal(vmir2::copy_reference cpr)
    {
        std::string result = "COPYREF %" + std::to_string(cpr.from_index) + ", %" + std::to_string(cpr.to_index);
        return result;
    }
    std::string assembler::to_string_internal(vmir2::constexpr_set_result inst)
    {
        std::string result = "CE_SETRESULT %" + std::to_string(inst.target);
        return result;
    }
    std::string assembler::to_string_internal(vmir2::constexpr_set_result2 inst)
    {
        std::string result = "CE_SETRESULT_ANTESTATAL %" + std::to_string(inst.target);
        if (inst.result_id != 0 || inst.target_mode != vmir2::constexpr_result_target_mode::value)
        {
            result += ", #" + std::to_string(inst.result_id);
            if (inst.target_mode == vmir2::constexpr_result_target_mode::referenced_object)
            {
                result += ", REF_OBJECT";
            }
        }
        return result;
    }
    std::string assembler::to_string_internal(vmir2::constexpr_make_proxy inst)
    {
        return "CE_MAKE_PROXY %" + std::to_string(inst.target) + ", #" + std::to_string(inst.result_id);
    }
    std::string assembler::to_string_internal(vmir2::constexpr_output_byte inst)
    {
        return "CE_OUTPUT_BYTE %" + std::to_string(inst.proxy) + ", %" + std::to_string(inst.value);
    }
    std::string assembler::to_string_internal(vmir2::jump inst)
    {
        std::string result;
        result += "JUMP !" + std::to_string(inst.target);
        return result;
    }
    std::string assembler::to_string_internal(vmir2::branch inst)
    {
        std::string result;
        result += "BRANCH %" + std::to_string(inst.condition) + ", !" + std::to_string(inst.target_true) + ", !" + std::to_string(inst.target_false);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::runtime_constexpr inst)
    {
        std::string result;
        result += "RUNTIME_CONSTEXPR !" + std::to_string(inst.target_constexpr) + ", !" + std::to_string(inst.target_native);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::initguard_try_acquire inst)
    {
        std::string result;
        result += "INITGUARD_TRY_ACQUIRE " + quxlang::to_string(inst.symbol) + " -> %" + std::to_string(inst.target_lock) + ", !" + std::to_string(inst.target_acquired) + ", !" + std::to_string(inst.target_already_initialized);
        return result;
    }

    std::string assembler::to_string_internal(vmir2::ret inst)
    {
        return "RET";
    }

    std::string assembler::to_string_internal(vmir2::invocation_args inst)
    {
        std::string output = "[";
        bool first = true;
        for (auto& i : inst.named)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;

            output += i.first + "=" + std::to_string(i.second);
        }
        for (auto& i : inst.positional)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;

            output += std::to_string(i);
        }

        output += "]";
        return output;
    }
    std::string assembler::to_string_internal(vmir2::load_const_value inst)
    {
        std::string output = "INITVAL %" + std::to_string(inst.target) + ", {";

        bool first = true;
        for (auto& i : inst.value)
        {
            if (!first)
            {
                output += ", ";
            }

            first = false;

            int8_t val = static_cast< uint8_t >(i);

            uint8_t lower = val & 0x0F;
            uint8_t upper = (val & 0xF0) >> 4;

            if (upper < 10)
            {
                output += std::to_string(upper);
            }
            else
            {
                output += 'A' + upper - 10;
            }

            if (lower < 10)
            {
                output += std::to_string(lower);
            }
            else
            {
                output += 'A' + lower - 10;
            }
        }

        output += "}";
        return output;
    }
    std::string assembler::to_string_internal(vmir2::load_const_bool inst)
    {
        std::string result = "INITBOOL %" + std::to_string(inst.target) + ", ";
        if (inst.value)
        {
            result += "true";
        }
        else
        {
            result += "false";
        }
        return result;
    }
    std::string assembler::to_string_internal(vmir2::load_const_int inst)
    {
        return "INIT_INT %" + std::to_string(inst.target) + ", " + inst.value;
    }
    std::string assembler::to_string_internal(vmir2::load_const_float inst)
    {
        return std::string(inst.require_exact ? "INIT_FLOAT_EXACT %" : "INIT_FLOAT_APPROX %") + std::to_string(inst.target) + ", " + inst.value;
    }
    std::string assembler::to_string_internal(vmir2::canonicalize_float inst)
    {
        return "FCANON %" + std::to_string(inst.source) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::get_value_byte inst)
    {
        return "GET_BYTE %" + std::to_string(inst.source_reference) + ", " + std::to_string(inst.offset) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::set_value_byte inst)
    {
        return "SET_BYTE %" + std::to_string(inst.target_reference) + ", " + std::to_string(inst.offset) + ", %" + std::to_string(inst.value);
    }

    std::string assembler::to_string_internal(vmir2::make_pointer_to inst)
    {
        return "MAKE_PTR %" + std::to_string(inst.of_index) + ", %" + std::to_string(inst.pointer_index);
    }
    std::string assembler::to_string_internal(vmir2::swap swp)
    {
        return "SWAP %" + std::to_string(swp.a) + ", %" + std::to_string(swp.b);
    }
    std::string assembler::to_string_internal(vmir2::load_from_ref inst)
    {
        return "LOAD" + atomic_access_mode_suffix(inst.access_mode) + " %" + std::to_string(inst.from_reference) + ", %" + std::to_string(inst.to_value);
    }
    std::string assembler::to_string_internal(vmir2::store_to_ref inst)
    {
        return "STORE" + atomic_access_mode_suffix(inst.access_mode) + " %" + std::to_string(inst.from_value) + ", %" + std::to_string(inst.to_reference);
    }
    std::string assembler::to_string_internal(vmir2::compare_exchange inst)
    {
        return "CMPXCHG [" + atomic_access_mode_assembly_name(inst.success_mode) + ", " + atomic_access_mode_assembly_name(inst.failure_mode) + "] %" + std::to_string(inst.target_reference) + ", %" + std::to_string(inst.expected_reference) + ", %" + std::to_string(inst.desired_value) + " -> %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::dereference_pointer inst)
    {
        return "DEREF %" + std::to_string(inst.from_pointer) + ", %" + std::to_string(inst.to_reference);
    }
    std::string assembler::to_string_internal(vmir2::int_add add)
    {
        return "IADD %" + std::to_string(add.a) + ", %" + std::to_string(add.b) + ", %" + std::to_string(add.result);
    }

    std::string assembler::to_string_internal(vmir2::int_sub sub)
    {
        return "ISUB %" + std::to_string(sub.a) + ", %" + std::to_string(sub.b) + ", %" + std::to_string(sub.result);
    }

    std::string assembler::to_string_internal(vmir2::int_mul mul)
    {
        return "IMUL %" + std::to_string(mul.a) + ", %" + std::to_string(mul.b) + ", %" + std::to_string(mul.result);
    }

    std::string assembler::to_string_internal(vmir2::int_div div)
    {
        return "IDIV %" + std::to_string(div.a) + ", %" + std::to_string(div.b) + ", %" + std::to_string(div.result);
    }

    std::string assembler::to_string_internal(vmir2::int_mod mod)
    {
        return "IMOD %" + std::to_string(mod.a) + ", %" + std::to_string(mod.b) + ", %" + std::to_string(mod.result);
    }

    std::string assembler::to_string_internal(vmir2::mut_int_add op)
    {
        return "MUT_IADD" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_int_sub op)
    {
        return "MUT_ISUB" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_int_mul op)
    {
        return "MUT_IMUL" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_int_div op)
    {
        return "MUT_IDIV" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_int_mod op)
    {
        return "MUT_IMOD" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }

    std::string assembler::to_string_internal(vmir2::float_add add)
    {
        return "FADD %" + std::to_string(add.a) + ", %" + std::to_string(add.b) + ", %" + std::to_string(add.result);
    }
    std::string assembler::to_string_internal(vmir2::float_sub sub)
    {
        return "FSUB %" + std::to_string(sub.a) + ", %" + std::to_string(sub.b) + ", %" + std::to_string(sub.result);
    }
    std::string assembler::to_string_internal(vmir2::float_mul mul)
    {
        return "FMUL %" + std::to_string(mul.a) + ", %" + std::to_string(mul.b) + ", %" + std::to_string(mul.result);
    }
    std::string assembler::to_string_internal(vmir2::float_div div)
    {
        return "FDIV %" + std::to_string(div.a) + ", %" + std::to_string(div.b) + ", %" + std::to_string(div.result);
    }
    std::string assembler::to_string_internal(vmir2::mut_float_add op)
    {
        return "MUT_FADD %" + std::to_string(op.target) + ", %" + std::to_string(op.value);
    }
    std::string assembler::to_string_internal(vmir2::mut_float_sub op)
    {
        return "MUT_FSUB %" + std::to_string(op.target) + ", %" + std::to_string(op.value);
    }
    std::string assembler::to_string_internal(vmir2::mut_float_mul op)
    {
        return "MUT_FMUL %" + std::to_string(op.target) + ", %" + std::to_string(op.value);
    }
    std::string assembler::to_string_internal(vmir2::mut_float_div op)
    {
        return "MUT_FDIV %" + std::to_string(op.target) + ", %" + std::to_string(op.value);
    }
    std::string assembler::to_string_internal(vmir2::float_from_int op)
    {
        return "ITOF %" + std::to_string(op.source) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::float_ieee_eq op)
    {
        return "IEEE_FEQ %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::float_ieee_ne op)
    {
        return "IEEE_FNE %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::float_ieee_lt op)
    {
        return "IEEE_FLT %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::float_ieee_gt op)
    {
        return "IEEE_FGT %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }

    // Bitwise operations
    std::string assembler::to_string_internal(vmir2::bitwise_and op)
    {
        return "BITWISE_AND %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_or op)
    {
        return "BITWISE_OR %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_xor op)
    {
        return "BITWISE_XOR %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_nand op)
    {
        return "BITWISE_NAND %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_nor op)
    {
        return "BITWISE_NOR %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_nxor op)
    {
        return "BITWISE_NXOR %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_implies op)
    {
        return "BITWISE_IMPLIES %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_implied op)
    {
        return "BITWISE_IMPLIED %" + std::to_string(op.a) + ", %" + std::to_string(op.b) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_shift_up op)
    {
        return "BITWISE_SHIFT_UP %" + std::to_string(op.value) + ", %" + std::to_string(op.amount) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_shift_down op)
    {
        return "BITWISE_SHIFT_DOWN %" + std::to_string(op.value) + ", %" + std::to_string(op.amount) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_rotate_up op)
    {
        return "BITWISE_ROTATE_UP %" + std::to_string(op.value) + ", %" + std::to_string(op.amount) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_rotate_down op)
    {
        return "BITWISE_ROTATE_DOWN %" + std::to_string(op.value) + ", %" + std::to_string(op.amount) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::bitwise_inverse op)
    {
        return "BITWISE_INVERSE %" + std::to_string(op.value) + ", %" + std::to_string(op.result);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_and op)
    {
        return "MUT_BITWISE_AND" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_or op)
    {
        return "MUT_BITWISE_OR" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_xor op)
    {
        return "MUT_BITWISE_XOR" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_nand op)
    {
        return "MUT_BITWISE_NAND" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_nor op)
    {
        return "MUT_BITWISE_NOR" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_nxor op)
    {
        return "MUT_BITWISE_NXOR" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_implies op)
    {
        return "MUT_BITWISE_IMPLIES" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_implied op)
    {
        return "MUT_BITWISE_IMPLIED" + atomic_access_mode_suffix(op.access_mode) + " %" + std::to_string(op.target) + ", %" + std::to_string(op.value) + rmw_old_value_suffix(op.old_value);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_shift_up op)
    {
        return "MUT_BITWISE_SHIFT_UP %" + std::to_string(op.target) + ", %" + std::to_string(op.amount);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_shift_down op)
    {
        return "MUT_BITWISE_SHIFT_DOWN %" + std::to_string(op.target) + ", %" + std::to_string(op.amount);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_rotate_up op)
    {
        return "MUT_BITWISE_ROTATE_UP %" + std::to_string(op.target) + ", %" + std::to_string(op.amount);
    }
    std::string assembler::to_string_internal(vmir2::mut_bitwise_rotate_down op)
    {
        return "MUT_BITWISE_ROTATE_DOWN %" + std::to_string(op.target) + ", %" + std::to_string(op.amount);
    }
    std::string assembler::to_string_internal(vmir2::load_const_zero inst)
    {
        return "INIT_ZERO %" + std::to_string(inst.target);
    }
    std::string assembler::to_string_internal(vmir2::cmp_eq inst)
    {
        return "CMP_EQ %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::cmp_ne inst)
    {
        return "CMP_NE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::cmp_lt inst)
    {
        return "CMP_LT %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::cmp_ge inst)
    {
        return "CMP_GE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }

    // Pointer compare instructions
    std::string assembler::to_string_internal(vmir2::pcmp_eq inst)
    {
        return "PTR_CMP_EQ %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::pcmp_ne inst)
    {
        return "PTR_CMP_NE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::pcmp_lt inst)
    {
        return "PTR_CMP_LT %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::pcmp_ge inst)
    {
        return "PTR_CMP_GE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }


    std::string assembler::to_string_internal(vmir2::gcmp_eq inst)
    {
        return "GLB_CMP_EQ %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::gcmp_ne inst)
    {
        return "GLB_CMP_NE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::gcmp_lt inst)
    {
        return "GLB_CMP_LT %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::gcmp_ge inst)
    {
        return "GLB_CMP_GE %" + std::to_string(inst.a) + ", %" + std::to_string(inst.b) + ", %" + std::to_string(inst.result);
    }
    std::string assembler::to_string_internal(vmir2::defer_nontrivial_dtor dntd)
    {
        return "DEFER_DTOR " + quxlang::to_string(dntd.func) + ", %" + std::to_string(dntd.on_value) + ", %" + this->to_string_internal(dntd.args);
    }
    std::string assembler::to_string_internal(vmir2::struct_init_start sdn)
    {
        return "STRUCT_INIT_START %" + std::to_string(sdn.on_value) + ", " + this->to_string_internal(sdn.fields);
    }
    std::string assembler::to_string_internal(vmir2::struct_init_finish scn)
    {
        return "STRUCT_INIT_FINISH %" + std::to_string(scn.on_value);
    }
    std::string assembler::to_string_internal(vmir2::end_lifetime elt)
    {
        return "END_LIFETIME %" + std::to_string(elt.of);
    }

    std::string assembler::to_string_internal(vmir2::destroy dst)
    {
        return "DESTROY %" + std::to_string(dst.of);
    }
    std::string assembler::to_string_internal(vmir2::pointer_arith inst)
    {
        return "PTR_ARITH %" + std::to_string(inst.from) + ", " + std::to_string(inst.multiplier) + " %" + std::to_string(inst.offset) + ", %" + std::to_string(inst.result);
    }

    std::string assembler::to_string_internal(vmir2::pointer_diff inst)
    {
        std::string result = "PTR_DIFF %" + std::to_string(inst.from) + ", %" + std::to_string(inst.to) + ", %" + std::to_string(inst.result);
        return result;
    }

} // namespace quxlang::vmir2
