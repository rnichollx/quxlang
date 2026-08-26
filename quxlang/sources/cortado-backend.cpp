// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/blake2b.hpp>
#include <quxlang/cortado-backend.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/parse_int.hpp>
#include <quxlang/vmir2/assembler.hpp>

#include <rpnx/cortado/builders.hpp>
#include <rpnx/cortado/errors.hpp>
#include <rpnx/cortado/jar.hpp>
#include <rpnx/cortado/validation.hpp>
#include <rpnx/unimplemented.hpp>

#include <bit>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace quxlang::cortado_backend
{
    /** Owns deterministic JVM class generation and JAR assembly for one Cortado packet. */
    struct cortado_jar_emitter_impl
    {
        using code_builder = rpnx::cortado::code_builder;
        using label = rpnx::cortado::label;
        using local_variable_index = rpnx::cortado::local_variable_index;
        using newarray_type = rpnx::cortado::newarray_type;
        using newarray_instruction = rpnx::cortado::newarray_instruction;
        using opcode = rpnx::cortado::opcode;

        /** Identifies the JVM carrier category used to lower a Quxlang VMIR value. */
        enum class jvm_value_kind {
            void_,
            integer,
            long_,
            big_integer,
            float_,
            double_,
            reference,
        };

        /** Records the generated JVM identity and calling convention of one Quxlang routine. */
        struct routine_jvm_info
        {
            std::string class_name;
            std::string descriptor;
            std::optional< std::string > argument_frame_class_name;
            jvm_value_kind return_kind = jvm_value_kind::void_;
        };

        /** Identifies one addressable cell in an antestatal global's managed object graph. */
        struct resolved_antestatal_access
        {
            type_symbol root_symbol;
            type_symbol target_type;
            std::vector< std::uint64_t > cell_path;
        };

        /** Selects one managed cell array used while filling an antestatal object graph. */
        enum class antestatal_cell_array_kind : std::uint8_t {
            values,
            initialized,
        };

        /** Supplies the generated-class hierarchy information required by Cortado validation. */
        struct jvm_class_hierarchy
        {
            /** Returns the closest modeled common superclass of two generated verifier types. */
            auto common_superclass(std::string_view left, std::string_view right) const -> std::string
            {
                if (left == right)
                {
                    return std::string(left);
                }
                if ((left == "java/lang/String" && right == "java/lang/CharSequence") || (right == "java/lang/String" && left == "java/lang/CharSequence"))
                {
                    return "java/lang/CharSequence";
                }
                bool const left_is_throwable_subtype = left == "java/lang/RuntimeException" || left == "java/lang/UnsupportedOperationException" || left == "java/lang/OutOfMemoryError";
                bool const right_is_throwable_subtype = right == "java/lang/RuntimeException" || right == "java/lang/UnsupportedOperationException" || right == "java/lang/OutOfMemoryError";
                if ((left_is_throwable_subtype && right == "java/lang/Throwable") || (right_is_throwable_subtype && left == "java/lang/Throwable"))
                {
                    return "java/lang/Throwable";
                }
                if ((left == "java/lang/UnsupportedOperationException" && right == "java/lang/RuntimeException") || (right == "java/lang/UnsupportedOperationException" && left == "java/lang/RuntimeException"))
                {
                    return "java/lang/RuntimeException";
                }
                return "java/lang/Object";
            }
        };

        /** Converts a VMIR local identifier to a vector index. */
        static auto local_slot(vmir2::local_index index) -> std::size_t
        {
            return static_cast< std::size_t >(std::uint64_t(index));
        }

        /** Converts a VMIR block identifier to a vector index. */
        static auto block_slot(vmir2::block_index index) -> std::size_t
        {
            return static_cast< std::size_t >(std::uint64_t(index));
        }

        /** Copies textual JAR content into its byte representation. */
        static auto bytes_from_string(std::string_view text) -> std::vector< std::byte >
        {
            std::vector< std::byte > result;
            result.reserve(text.size());
            for (char const character : text)
            {
                result.push_back(static_cast< std::byte >(static_cast< std::uint8_t >(character)));
            }
            return result;
        }

        /** Returns the deterministic bounded JVM internal name for a Quxlang procedure. */
        static auto procedure_class_name(type_symbol const& symbol) -> std::string
        {
            std::string const display_name = to_string(symbol);
            std::span< std::byte const > const bytes(reinterpret_cast< std::byte const* >(display_name.data()), display_name.size());
            return "quxlang/generated/P" + blake2b::hex(bytes).substr(0, 32);
        }

        /** Returns the deterministic JVM internal name for a procedure-value adapter. */
        static auto callable_adapter_class_name(type_symbol const& symbol) -> std::string
        {
            std::string const display_name = to_string(symbol);
            std::span< std::byte const > const bytes(reinterpret_cast< std::byte const* >(display_name.data()), display_name.size());
            return "quxlang/generated/C" + blake2b::hex(bytes).substr(0, 32);
        }

        /** Returns the deterministic GeneratedGlobals value field for a Quxlang global. */
        static auto global_field_name(type_symbol const& symbol) -> std::string
        {
            std::string const display_name = to_string(symbol);
            std::span< std::byte const > const bytes(reinterpret_cast< std::byte const* >(display_name.data()), display_name.size());
            return "g" + blake2b::hex(bytes).substr(0, 32);
        }

        /** Returns the deterministic GeneratedGlobals initialization-state field for a Quxlang global. */
        static auto global_initialization_field_name(type_symbol const& symbol) -> std::string
        {
            std::string const display_name = to_string(symbol);
            std::span< std::byte const > const bytes(reinterpret_cast< std::byte const* >(display_name.data()), display_name.size());
            return "i" + blake2b::hex(bytes).substr(0, 32);
        }

        /** Returns the deterministic JVM internal name for a TYPED_STORAGE declaration. */
        static auto typed_storage_class_name(type_symbol const& storage_type) -> std::string
        {
            std::string const display_name = to_string(storage_type);
            std::span< std::byte const > const bytes(reinterpret_cast< std::byte const* >(display_name.data()), display_name.size());
            return "quxlang/generated/S" + blake2b::hex(bytes).substr(0, 32);
        }

        /** Removes Quxlang value-slot wrappers from a semantic type. */
        static auto unwrapped_type(type_symbol type) -> type_symbol
        {
            while (type.type_is< nvalue_slot >() || type.type_is< dvalue_slot >())
            {
                if (type.type_is< nvalue_slot >())
                {
                    type = type.as< nvalue_slot >().target;
                }
                else
                {
                    type = type.as< dvalue_slot >().target;
                }
            }
            return type;
        }

        /** Selects the JVM carrier category for a semantic Quxlang type. */
        static auto value_kind(cortado_compilable_unit const& input, type_symbol type) -> jvm_value_kind
        {
            type = unwrapped_type(std::move(type));
            if (type.type_is< void_type >())
            {
                return jvm_value_kind::void_;
            }
            if (type.type_is< bool_type >() || type.type_is< byte_type >())
            {
                return jvm_value_kind::integer;
            }
            if (type.type_is< builtin_symbol >() && type.as< builtin_symbol >().name == "ORDER")
            {
                return jvm_value_kind::integer;
            }
            if (type.type_is< size_type >() || type.type_is< type_index_type >())
            {
                return jvm_value_kind::long_;
            }
            if (type.type_is< int_type >())
            {
                std::size_t const bits = type.as< int_type >().bits;
                if (bits == 0)
                {
                    throw rpnx::unimplemented();
                }
                if (bits > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend cannot lower an integer width that exceeds java.math.BigInteger's index limit");
                }
                return bits <= 32 ? jvm_value_kind::integer : bits <= 64 ? jvm_value_kind::long_ : jvm_value_kind::big_integer;
            }
            if (type.type_is< float_type >())
            {
                float_type const& format = type.as< float_type >();
                if (format.bits == 32 && format.exponent_bits == 8)
                {
                    return jvm_value_kind::float_;
                }
                if (format.bits == 64 && format.exponent_bits == 11)
                {
                    return jvm_value_kind::double_;
                }
                throw rpnx::unimplemented();
            }
            std::map< type_symbol, enum_info >::const_iterator const enum_iter = input.enum_definitions.find(type);
            if (enum_iter != input.enum_definitions.end())
            {
                if (enum_iter->second.format.bit_width == 0)
                {
                    throw rpnx::unimplemented();
                }
                if (enum_iter->second.format.bit_width > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend cannot lower an enum width that exceeds java.math.BigInteger's index limit");
                }
                return enum_iter->second.format.bit_width <= 32 ? jvm_value_kind::integer : enum_iter->second.format.bit_width <= 64 ? jvm_value_kind::long_ : jvm_value_kind::big_integer;
            }
            std::map< type_symbol, flagset_info >::const_iterator const flagset_iter = input.flagset_definitions.find(type);
            if (flagset_iter != input.flagset_definitions.end())
            {
                if (flagset_iter->second.bits == 0)
                {
                    throw rpnx::unimplemented();
                }
                if (flagset_iter->second.bits > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend cannot lower a flagset width that exceeds java.math.BigInteger's index limit");
                }
                return flagset_iter->second.bits <= 32 ? jvm_value_kind::integer : flagset_iter->second.bits <= 64 ? jvm_value_kind::long_ : jvm_value_kind::big_integer;
            }
            return jvm_value_kind::reference;
        }

        /** Returns the JVM descriptor for a carrier category. */
        static auto descriptor_for_kind(jvm_value_kind kind) -> std::string
        {
            switch (kind)
            {
            case jvm_value_kind::void_:
                return "V";
            case jvm_value_kind::integer:
                return "I";
            case jvm_value_kind::long_:
                return "J";
            case jvm_value_kind::big_integer:
                return "Ljava/math/BigInteger;";
            case jvm_value_kind::float_:
                return "F";
            case jvm_value_kind::double_:
                return "D";
            case jvm_value_kind::reference:
                return "Ljava/lang/Object;";
            }
            throw compiler_bug("Unknown Quxlang Cortado backend JVM value kind");
        }

        /** Returns the JVM local-variable slot count consumed by a carrier category. */
        static auto value_slot_count(jvm_value_kind kind) -> std::uint16_t
        {
            return kind == jvm_value_kind::long_ || kind == jvm_value_kind::double_ ? 2 : 1;
        }

        /** Returns the JVM carrier category of a routine's Quxlang return value. */
        static auto routine_return_kind(cortado_compilable_unit const& input, vmir2::functanoid_routine3 const& routine) -> jvm_value_kind
        {
            std::map< std::string, vmir2::routine_parameter >::const_iterator const return_iter = routine.parameters.named.find("RETURN");
            if (return_iter == routine.parameters.named.end())
            {
                return jvm_value_kind::void_;
            }
            return value_kind(input, routine.local_types.at(local_slot(return_iter->second.local_index)).type);
        }

        /** Builds the JVM method descriptor for a lowered Quxlang routine. */
        static auto routine_descriptor(cortado_compilable_unit const& input, vmir2::functanoid_routine3 const& routine, std::optional< std::string > const& argument_frame_class_name) -> std::string
        {
            if (argument_frame_class_name.has_value())
            {
                return "(L" + *argument_frame_class_name + ";)" + descriptor_for_kind(routine_return_kind(input, routine));
            }
            std::string descriptor = "(";
            for (vmir2::routine_parameter const& parameter : routine.parameters.positional)
            {
                jvm_value_kind const kind = value_kind(input, routine.local_types.at(local_slot(parameter.local_index)).type);
                descriptor += descriptor_for_kind(kind);
            }
            for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine.parameters.named)
            {
                if (parameter.first == "RETURN")
                {
                    continue;
                }
                jvm_value_kind const kind = value_kind(input, routine.local_types.at(local_slot(parameter.second.local_index)).type);
                descriptor += descriptor_for_kind(kind);
            }
            descriptor += ")";
            descriptor += descriptor_for_kind(routine_return_kind(input, routine));
            return descriptor;
        }

        /** Counts JVM parameter slots required by a direct static invocation. */
        static auto routine_parameter_slot_count(cortado_compilable_unit const& input, vmir2::functanoid_routine3 const& routine) -> std::uint32_t
        {
            std::uint32_t parameter_slots = 0;
            for (vmir2::routine_parameter const& parameter : routine.parameters.positional)
            {
                parameter_slots += value_slot_count(value_kind(input, routine.local_types.at(local_slot(parameter.local_index)).type));
            }
            for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine.parameters.named)
            {
                if (parameter.first == "RETURN")
                {
                    continue;
                }
                parameter_slots += value_slot_count(value_kind(input, routine.local_types.at(local_slot(parameter.second.local_index)).type));
            }
            return parameter_slots;
        }

        /** Emits the shortest supported JVM instruction sequence for a 32-bit constant. */
        static void emit_int_constant(code_builder& code, std::uint32_t value)
        {
            bool started = false;
            for (int shift = 24; shift >= 0; shift -= 8)
            {
                std::uint32_t const byte = (value >> shift) & 0xffU;
                if (!started)
                {
                    if (byte == 0 && shift != 0)
                    {
                        continue;
                    }
                    code.sipush(static_cast< std::int16_t >(byte));
                    started = true;
                }
                else
                {
                    code.bipush(8).append< opcode::ishl >();
                    if (byte != 0)
                    {
                        code.sipush(static_cast< std::int16_t >(byte)).append< opcode::ior >();
                    }
                }
            }
        }

        /** Emits a JVM long constant without depending on Java source compilation. */
        static void emit_long_constant(code_builder& code, std::uint64_t value)
        {
            bool started = false;
            for (int shift = 56; shift >= 0; shift -= 8)
            {
                std::uint64_t const byte = (value >> shift) & 0xffU;
                if (!started)
                {
                    if (byte == 0 && shift != 0)
                    {
                        continue;
                    }
                    code.sipush(static_cast< std::int16_t >(byte)).append< opcode::i2l >();
                    started = true;
                }
                else
                {
                    code.bipush(8).append< opcode::lshl >();
                    if (byte != 0)
                    {
                        code.sipush(static_cast< std::int16_t >(byte)).append< opcode::i2l >().append< opcode::lor >();
                    }
                }
            }
        }

        /** Emits a positive BigInteger from a little-endian byte representation. */
        static void emit_big_integer_from_little_endian(code_builder& code, std::span< std::byte const > value)
        {
            if (value.size() > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
            {
                throw lowering_compilation_error("Quxlang's Cortado backend cannot materialize an integer that exceeds the JVM array-index limit");
            }
            code.new_("java/math/BigInteger").append< opcode::dup >().append< opcode::iconst_1 >();
            emit_int_constant(code, static_cast< std::uint32_t >(value.size()));
            code.append(newarray_instruction{.element_type = newarray_type::byte});
            for (std::size_t index = 0; index < value.size(); ++index)
            {
                std::uint8_t const byte = std::to_integer< std::uint8_t >(value[value.size() - index - 1]);
                if (byte == 0)
                {
                    continue;
                }
                code.append< opcode::dup >();
                emit_int_constant(code, static_cast< std::uint32_t >(index));
                code.bipush(static_cast< std::int8_t >(byte)).append< opcode::bastore >();
            }
            code.invokespecial("java/math/BigInteger", "<init>", "(I[B)V");
        }

        /** Emits a BigInteger parsed from a validated Quxlang decimal integer literal. */
        static void emit_big_integer_from_decimal(code_builder& code, std::string const& value)
        {
            code.new_("java/math/BigInteger").append< opcode::dup >().ldc_string(value).invokespecial("java/math/BigInteger", "<init>", "(Ljava/lang/String;)V");
        }

        /** Canonicalizes a BigInteger operand-stack value to a fixed Quxlang width. */
        static void emit_big_integer_canonicalization(code_builder& code, std::uint32_t bit_width, bool has_sign)
        {
            if (has_sign)
            {
                code.getstatic("java/math/BigInteger", "ONE", "Ljava/math/BigInteger;");
                emit_int_constant(code, bit_width - 1);
                code.invokevirtual("java/math/BigInteger", "shiftLeft", "(I)Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "add", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
            }
            code.getstatic("java/math/BigInteger", "ONE", "Ljava/math/BigInteger;");
            emit_int_constant(code, bit_width);
            code.invokevirtual("java/math/BigInteger", "shiftLeft", "(I)Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "mod", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
            if (has_sign)
            {
                code.getstatic("java/math/BigInteger", "ONE", "Ljava/math/BigInteger;");
                emit_int_constant(code, bit_width - 1);
                code.invokevirtual("java/math/BigInteger", "shiftLeft", "(I)Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "subtract", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
            }
        }

        /** Parses a Quxlang integer literal into its low 64 bits. */
        static auto parse_integer_bits(std::string const& text) -> std::uint64_t
        {
            std::size_t consumed = 0;
            if (!text.empty() && text.front() == '-')
            {
                std::int64_t const value = std::stoll(text, &consumed, 10);
                if (consumed != text.size())
                {
                    throw compiler_bug("Quxlang's Cortado backend received an invalid integer constant: " + text);
                }
                return std::bit_cast< std::uint64_t >(value);
            }
            std::uint64_t const value = std::stoull(text, &consumed, 10);
            if (consumed != text.size())
            {
                throw compiler_bug("Quxlang's Cortado backend received an invalid integer constant: " + text);
            }
            return value;
        }

        /** Extracts the raw low 64 bits of a constexpr primitive value. */
        static auto primitive_bits(antestatal_primitive const& value) -> std::uint64_t
        {
            if (value.value.size() > sizeof(std::uint64_t))
            {
                throw rpnx::unimplemented();
            }
            std::uint64_t bits = 0;
            for (std::size_t index = 0; index < value.value.size(); ++index)
            {
                bits |= static_cast< std::uint64_t >(std::to_integer< std::uint8_t >(value.value.at(index))) << (index * 8);
            }
            return bits;
        }

        /** Emits a constexpr primitive as the boxed value stored in a managed global cell. */
        static void emit_boxed_antestatal_primitive(code_builder& code, cortado_compilable_unit const& input, type_symbol const& type, antestatal_primitive const& value)
        {
            switch (value_kind(input, type))
            {
            case jvm_value_kind::integer:
            {
                std::uint64_t const bits = primitive_bits(value);
                emit_int_constant(code, static_cast< std::uint32_t >(bits));
                code.invokestatic("java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;");
                return;
            }
            case jvm_value_kind::long_:
            {
                std::uint64_t const bits = primitive_bits(value);
                emit_long_constant(code, bits);
                code.invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;");
                return;
            }
            case jvm_value_kind::big_integer:
            {
                emit_big_integer_from_little_endian(code, value.value);
                type_symbol const numeric_type = unwrapped_type(type);
                std::uint32_t bit_width = 0;
                bool has_sign = false;
                if (numeric_type.type_is< int_type >())
                {
                    bit_width = static_cast< std::uint32_t >(numeric_type.as< int_type >().bits);
                    has_sign = numeric_type.as< int_type >().has_sign;
                }
                else
                {
                    std::map< type_symbol, enum_info >::const_iterator const enum_iter = input.enum_definitions.find(numeric_type);
                    if (enum_iter != input.enum_definitions.end())
                    {
                        bit_width = static_cast< std::uint32_t >(enum_iter->second.format.bit_width);
                        has_sign = enum_iter->second.format.encoding == enum_integer_encoding::signed_twos_complement_le;
                    }
                    else
                    {
                        bit_width = static_cast< std::uint32_t >(input.flagset_definitions.at(numeric_type).bits);
                    }
                }
                emit_big_integer_canonicalization(code, bit_width, has_sign);
                return;
            }
            case jvm_value_kind::float_:
            {
                std::uint64_t const bits = primitive_bits(value);
                emit_int_constant(code, static_cast< std::uint32_t >(bits));
                code.invokestatic("java/lang/Float", "intBitsToFloat", "(I)F").invokestatic("java/lang/Float", "valueOf", "(F)Ljava/lang/Float;");
                return;
            }
            case jvm_value_kind::double_:
            {
                std::uint64_t const bits = primitive_bits(value);
                emit_long_constant(code, bits);
                code.invokestatic("java/lang/Double", "longBitsToDouble", "(J)D").invokestatic("java/lang/Double", "valueOf", "(D)Ljava/lang/Double;");
                return;
            }
            case jvm_value_kind::reference:
            case jvm_value_kind::void_:
                throw rpnx::unimplemented();
            }
            throw compiler_bug("Unknown Quxlang Cortado backend JVM value kind");
        }

        /** Loads a typed JVM carrier from a deterministic local slot. */
        static void emit_load(code_builder& code, jvm_value_kind kind, local_variable_index slot)
        {
            switch (kind)
            {
            case jvm_value_kind::integer:
                code.iload(slot);
                return;
            case jvm_value_kind::long_:
                code.lload(slot);
                return;
            case jvm_value_kind::big_integer:
                code.aload(slot);
                return;
            case jvm_value_kind::float_:
                code.fload(slot);
                return;
            case jvm_value_kind::double_:
                code.dload(slot);
                return;
            case jvm_value_kind::reference:
                code.aload(slot);
                return;
            case jvm_value_kind::void_:
                break;
            }
            throw compiler_bug("Cannot load a void Quxlang Cortado backend local");
        }

        /** Stores a typed JVM carrier into a deterministic local slot. */
        static void emit_store(code_builder& code, jvm_value_kind kind, local_variable_index slot)
        {
            switch (kind)
            {
            case jvm_value_kind::integer:
                code.istore(slot);
                return;
            case jvm_value_kind::long_:
                code.lstore(slot);
                return;
            case jvm_value_kind::big_integer:
                code.astore(slot);
                return;
            case jvm_value_kind::float_:
                code.fstore(slot);
                return;
            case jvm_value_kind::double_:
                code.dstore(slot);
                return;
            case jvm_value_kind::reference:
                code.astore(slot);
                return;
            case jvm_value_kind::void_:
                break;
            }
            throw compiler_bug("Cannot store a void Quxlang Cortado backend local");
        }

        /** Emits the JVM default value for a carrier category. */
        static void emit_default_value(code_builder& code, jvm_value_kind kind)
        {
            switch (kind)
            {
            case jvm_value_kind::integer:
                code.append< opcode::iconst_0 >();
                return;
            case jvm_value_kind::long_:
                code.append< opcode::lconst_0 >();
                return;
            case jvm_value_kind::big_integer:
                code.getstatic("java/math/BigInteger", "ZERO", "Ljava/math/BigInteger;");
                return;
            case jvm_value_kind::float_:
                code.append< opcode::fconst_0 >();
                return;
            case jvm_value_kind::double_:
                code.append< opcode::dconst_0 >();
                return;
            case jvm_value_kind::reference:
                code.append< opcode::aconst_null >();
                return;
            case jvm_value_kind::void_:
                return;
            }
        }

        /** Emits a boxed default value for one JVM carrier kind. */
        static void emit_boxed_default_value(code_builder& code, jvm_value_kind kind)
        {
            emit_default_value(code, kind);
            switch (kind)
            {
            case jvm_value_kind::integer:
                code.invokestatic("java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;");
                return;
            case jvm_value_kind::long_:
                code.invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;");
                return;
            case jvm_value_kind::big_integer:
                return;
            case jvm_value_kind::float_:
                code.invokestatic("java/lang/Float", "valueOf", "(F)Ljava/lang/Float;");
                return;
            case jvm_value_kind::double_:
                code.invokestatic("java/lang/Double", "valueOf", "(D)Ljava/lang/Double;");
                return;
            case jvm_value_kind::reference:
                return;
            case jvm_value_kind::void_:
                break;
            }
            throw compiler_bug("Cannot box a void Quxlang Cortado backend default value");
        }

        /** Boxes a carrier value already present on the JVM operand stack. */
        static void emit_boxed_stack_value(code_builder& code, jvm_value_kind kind)
        {
            switch (kind)
            {
            case jvm_value_kind::integer:
                code.invokestatic("java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;");
                return;
            case jvm_value_kind::long_:
                code.invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;");
                return;
            case jvm_value_kind::big_integer:
                return;
            case jvm_value_kind::float_:
                code.invokestatic("java/lang/Float", "valueOf", "(F)Ljava/lang/Float;");
                return;
            case jvm_value_kind::double_:
                code.invokestatic("java/lang/Double", "valueOf", "(D)Ljava/lang/Double;");
                return;
            case jvm_value_kind::reference:
                return;
            case jvm_value_kind::void_:
                break;
            }
            throw compiler_bug("Cannot box a void Quxlang Cortado backend stack value");
        }

        /** Unboxes an Object value already present on the JVM operand stack. */
        static void emit_unboxed_stack_value(code_builder& code, jvm_value_kind kind)
        {
            switch (kind)
            {
            case jvm_value_kind::integer:
                code.checkcast("java/lang/Integer").invokevirtual("java/lang/Integer", "intValue", "()I");
                return;
            case jvm_value_kind::long_:
                code.checkcast("java/lang/Long").invokevirtual("java/lang/Long", "longValue", "()J");
                return;
            case jvm_value_kind::big_integer:
                code.checkcast("java/math/BigInteger");
                return;
            case jvm_value_kind::float_:
                code.checkcast("java/lang/Float").invokevirtual("java/lang/Float", "floatValue", "()F");
                return;
            case jvm_value_kind::double_:
                code.checkcast("java/lang/Double").invokevirtual("java/lang/Double", "doubleValue", "()D");
                return;
            case jvm_value_kind::reference:
                return;
            case jvm_value_kind::void_:
                break;
            }
            throw compiler_bug("Cannot unbox a void Quxlang Cortado backend stack value");
        }

        /** Emits a readonly Quxlang byte-span constant containing the supplied bytes. */
        static void emit_readonly_byte_span_object(code_builder& code, std::span< std::byte const > bytes, local_variable_index byte_owner_slot)
        {
            if (bytes.size() > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
            {
                throw lowering_compilation_error("Quxlang's Cortado backend cannot materialize a readonly byte constant that exceeds the JVM array-index limit");
            }
            code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
            emit_int_constant(code, static_cast< std::uint32_t >(bytes.size()));
            code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").astore(byte_owner_slot);
            for (std::size_t index = 0; index < bytes.size(); ++index)
            {
                code.aload(byte_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_int_constant(code, static_cast< std::uint32_t >(index));
                code.bipush(static_cast< std::int8_t >(std::to_integer< std::uint8_t >(bytes[index])));
                code.invokestatic("java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;").append< opcode::aastore >().aload(byte_owner_slot).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                emit_int_constant(code, static_cast< std::uint32_t >(index));
                code.append< opcode::iconst_1 >().append< opcode::bastore >();
            }

            code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
            emit_int_constant(code, 2);
            code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >().new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(byte_owner_slot).append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").append< opcode::aastore >().append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::iconst_0 >().append< opcode::iconst_1 >().append< opcode::bastore >().append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(byte_owner_slot);
            emit_long_constant(code, bytes.size());
            code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").append< opcode::aastore >().append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::iconst_1 >().append< opcode::iconst_1 >().append< opcode::bastore >();
        }

        /** Returns the semantic payload type used to materialize an antestatal value. */
        static auto normalized_antestatal_type(type_symbol type) -> type_symbol
        {
            type = unwrapped_type(std::move(type));
            while (type.type_is< attached_type_reference >())
            {
                type = type.as< attached_type_reference >().carrying_type;
            }
            if (std::optional< type_symbol > atomic_value = atomic_type_argument(type); atomic_value.has_value())
            {
                return normalized_antestatal_type(*atomic_value);
            }
            return type;
        }

        /** Returns the declaration-order alternatives of a reached semantic fusion definition. */
        static auto fusion_alternative_types(cortado_compilable_unit const& input, type_symbol const& type) -> std::vector< type_symbol >
        {
            std::map< type_symbol, union_info >::const_iterator union_definition = input.union_definitions.find(type);
            if (union_definition != input.union_definitions.end())
            {
                std::vector< type_symbol > result;
                result.reserve(union_definition->second.options.size());
                for (union_option_info const& option : union_definition->second.options)
                {
                    result.push_back(option.type);
                }
                return result;
            }
            std::map< type_symbol, variant_info >::const_iterator variant_definition = input.variant_definitions.find(type);
            if (variant_definition != input.variant_definitions.end())
            {
                return variant_definition->second.alternatives;
            }
            throw compiler_bug("Quxlang's Cortado backend is missing semantic fusion information for " + to_string(type));
        }

        /** Returns the common properties of a reached semantic fusion definition. */
        static auto fusion_properties_for_type(cortado_compilable_unit const& input, type_symbol const& type) -> fusion_properties const&
        {
            std::map< type_symbol, union_info >::const_iterator union_definition = input.union_definitions.find(type);
            if (union_definition != input.union_definitions.end())
            {
                return union_definition->second.properties;
            }
            std::map< type_symbol, variant_info >::const_iterator variant_definition = input.variant_definitions.find(type);
            if (variant_definition != input.variant_definitions.end())
            {
                return variant_definition->second.properties;
            }
            throw compiler_bug("Quxlang's Cortado backend is missing semantic fusion properties for " + to_string(type));
        }

        /** Returns whether a type has a reached semantic UNION or VARIANT definition. */
        static auto is_semantic_fusion_type(cortado_compilable_unit const& input, type_symbol const& type) -> bool
        {
            return input.union_definitions.contains(type) || input.variant_definitions.contains(type);
        }

        /** Emits the uninitialized managed shape of one recursively addressable antestatal value. */
        static void emit_antestatal_storage_shape(code_builder& code, cortado_compilable_unit const& input, type_symbol type, antestatal_value const& value)
        {
            type = normalized_antestatal_type(std::move(type));
            if (type.type_is< readonly_constant >())
            {
                code.append< opcode::aconst_null >();
                return;
            }
            std::map< type_symbol, std::vector< struct_field > >::const_iterator struct_definition = input.struct_definitions.find(type);
            if (struct_definition != input.struct_definitions.end())
            {
                if (!value.type_is< antestatal_struct >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a non-struct value for an antestatal struct");
                }
                antestatal_struct const& struct_value = value.get_as< antestatal_struct >();
                code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                emit_int_constant(code, static_cast< std::uint32_t >(struct_definition->second.size()));
                code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V");
                for (std::size_t index = 0; index < struct_definition->second.size(); ++index)
                {
                    struct_field const& field = struct_definition->second.at(index);
                    std::map< std::string, antestatal_value >::const_iterator const field_value = struct_value.fields.find(field.name);
                    if (field_value == struct_value.fields.end())
                    {
                        throw compiler_bug("Quxlang's Cortado backend received an antestatal struct without field " + field.name);
                    }
                    code.append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                    emit_int_constant(code, static_cast< std::uint32_t >(index));
                    emit_antestatal_storage_shape(code, input, field.type, field_value->second);
                    code.append< opcode::aastore >();
                }
                return;
            }

            if (type.type_is< array_type >())
            {
                if (!value.type_is< antestatal_array >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a non-array value for an antestatal array");
                }
                antestatal_array const& array_value = value.get_as< antestatal_array >();
                if (array_value.elements.size() > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend cannot materialize an antestatal array that exceeds the JVM array-index limit");
                }
                code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                emit_int_constant(code, static_cast< std::uint32_t >(array_value.elements.size()));
                code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V");
                for (std::size_t index = 0; index < array_value.elements.size(); ++index)
                {
                    code.append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                    emit_int_constant(code, static_cast< std::uint32_t >(index));
                    emit_antestatal_storage_shape(code, input, type.as< array_type >().element_type, array_value.elements.at(index));
                    code.append< opcode::aastore >();
                }
                return;
            }

            if (value.type_is< antestatal_fusion >())
            {
                std::vector< type_symbol > alternatives = fusion_alternative_types(input, type);
                antestatal_fusion const& fusion = value.get_as< antestatal_fusion >();
                code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                emit_int_constant(code, 2);
                code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V");
                if (fusion.state.type_is< antestatal_fusion_valueless >())
                {
                    return;
                }
                antestatal_fusion_active const& active = fusion.state.get_as< antestatal_fusion_active >();
                if (active.alternative >= alternatives.size())
                {
                    throw compiler_bug("Quxlang's Cortado backend received an antestatal fusion with an invalid alternative ordinal");
                }
                type_symbol const& alternative_type = alternatives.at(static_cast< std::size_t >(active.alternative));
                if (alternative_type.type_is< void_type >())
                {
                    if (active.payload.has_value())
                    {
                        throw compiler_bug("Quxlang's Cortado backend received a VOID antestatal fusion alternative with a payload");
                    }
                    return;
                }
                if (!active.payload.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a non-VOID antestatal fusion alternative without a payload");
                }

                code.append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().new_("quxlang/runtime/QuxlangObject").append< opcode::dup >().append< opcode::iconst_1 >().invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").append< opcode::aastore >();
                code.append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().append< opcode::aaload >().checkcast("quxlang/runtime/QuxlangObject").getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >();
                emit_antestatal_storage_shape(code, input, alternative_type, *active.payload);
                code.append< opcode::aastore >();
                return;
            }
            if (value.type_is< antestatal_interface >())
            {
                antestatal_interface const& interface_value = value.get_as< antestatal_interface >();
                if (interface_value.interface_type != type)
                {
                    throw compiler_bug("Quxlang's Cortado backend received an antestatal interface value with a mismatched semantic type");
                }
                code.new_("quxlang/runtime/QuxlangInterface").append< opcode::dup >();
                if (interface_value.is_default)
                {
                    code.append< opcode::iconst_1 >();
                }
                else
                {
                    code.append< opcode::iconst_0 >();
                }
                code.invokespecial("quxlang/runtime/QuxlangInterface", "<init>", "(Z)V");
                return;
            }
            code.append< opcode::aconst_null >();
        }

        /** Resolves a symbolic antestatal data address through semantic aggregate membership. */
        static auto resolve_antestatal_access(cortado_compilable_unit const& input, antestatal_access const& access) -> resolved_antestatal_access
        {
            if (access.type_is< antestatal_access_global >())
            {
                type_symbol const& symbol = access.get_as< antestatal_access_global >().symbol;
                std::map< type_symbol, type_symbol >::const_iterator const global = input.global_types.find(symbol);
                if (global == input.global_types.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend cannot resolve an antestatal data address outside the global closure: " + to_string(symbol));
                }
                return resolved_antestatal_access{
                    .root_symbol = symbol,
                    .target_type = normalized_antestatal_type(global->second),
                    .cell_path = {0},
                };
            }
            if (access.type_is< antestatal_access_field >())
            {
                antestatal_access_field const& field_access = access.get_as< antestatal_access_field >();
                resolved_antestatal_access result = resolve_antestatal_access(input, field_access.object);
                std::map< type_symbol, std::vector< struct_field > >::const_iterator const struct_definition = input.struct_definitions.find(result.target_type);
                if (struct_definition == input.struct_definitions.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend cannot resolve an antestatal field address without a semantic struct definition");
                }
                for (std::size_t index = 0; index < struct_definition->second.size(); ++index)
                {
                    struct_field const& field = struct_definition->second.at(index);
                    if (field.name == field_access.field_name)
                    {
                        result.target_type = normalized_antestatal_type(field.type);
                        result.cell_path.push_back(index);
                        return result;
                    }
                }
                throw compiler_bug("Quxlang's Cortado backend received an antestatal address for an unknown struct field: " + field_access.field_name);
            }
            if (access.type_is< antestatal_access_array_element >())
            {
                antestatal_access_array_element const& element_access = access.get_as< antestatal_access_array_element >();
                resolved_antestatal_access result = resolve_antestatal_access(input, element_access.array);
                if (!result.target_type.type_is< array_type >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received an antestatal array-element address for a non-array");
                }
                result.target_type = normalized_antestatal_type(result.target_type.as< array_type >().element_type);
                result.cell_path.push_back(element_access.index);
                return result;
            }
            if (access.type_is< antestatal_access_fusion_payload >())
            {
                antestatal_access_fusion_payload const& payload_access = access.get_as< antestatal_access_fusion_payload >();
                resolved_antestatal_access result = resolve_antestatal_access(input, payload_access.fusion);
                std::vector< type_symbol > alternatives = fusion_alternative_types(input, result.target_type);
                if (payload_access.alternative >= alternatives.size())
                {
                    throw compiler_bug("Quxlang's Cortado backend received an antestatal address with an invalid fusion alternative ordinal");
                }
                result.target_type = normalized_antestatal_type(alternatives.at(static_cast< std::size_t >(payload_access.alternative)));
                result.cell_path.push_back(1);
                result.cell_path.push_back(0);
                return result;
            }
            throw compiler_bug("Quxlang's Cortado backend attempted to resolve a null antestatal data address");
        }

        /** Emits the managed object containing the last cell of one resolved antestatal address. */
        static void emit_antestatal_cell_owner(code_builder& code, resolved_antestatal_access const& access)
        {
            if (access.cell_path.empty())
            {
                throw compiler_bug("Quxlang's Cortado backend received an empty antestatal cell path");
            }
            code.getstatic("quxlang/runtime/GeneratedGlobals", global_field_name(access.root_symbol), "Lquxlang/runtime/QuxlangObject;");
            for (std::size_t index = 0; index + 1 < access.cell_path.size(); ++index)
            {
                if (access.cell_path.at(index) > static_cast< std::uint64_t >(std::numeric_limits< std::int32_t >::max()))
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend cannot address an antestatal subobject beyond the JVM array-index limit");
                }
                code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_int_constant(code, static_cast< std::uint32_t >(access.cell_path.at(index)));
                code.append< opcode::aaload >().checkcast("quxlang/runtime/QuxlangObject");
            }
        }

        /** Emits a JVM reference to one resolved antestatal data cell. */
        static void emit_antestatal_data_reference(code_builder& code, resolved_antestatal_access const& access)
        {
            std::uint64_t cell_index = access.cell_path.back();
            code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >();
            emit_antestatal_cell_owner(code, access);
            emit_long_constant(code, cell_index);
            code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V");
        }

        /** Emits one null, data, or procedure pointer stored in an antestatal object. */
        static void emit_antestatal_pointer(code_builder& code, cortado_compilable_unit const& input, std::map< type_symbol, routine_jvm_info > const& routine_infos, type_symbol const& type, antestatal_ptrref const& pointer)
        {
            if (pointer.target.type_is< antestatal_nullptr >())
            {
                code.append< opcode::aconst_null >();
                return;
            }

            type_symbol normalized_type = normalized_antestatal_type(type);
            if (normalized_type.type_is< ptrref_type >() && normalized_type.as< ptrref_type >().target.type_is< procedure_type >())
            {
                if (!pointer.target.type_is< antestatal_access_global >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a non-global antestatal procedure address");
                }
                type_symbol const& procedure = pointer.target.get_as< antestatal_access_global >().symbol;
                if (!routine_infos.contains(procedure))
                {
                    throw compiler_bug("Quxlang's Cortado backend antestatal procedure pointer is outside the routine closure: " + to_string(procedure));
                }
                std::string adapter_name = callable_adapter_class_name(procedure);
                code.new_(adapter_name).append< opcode::dup >().invokespecial(adapter_name, "<init>", "()V");
                return;
            }

            emit_antestatal_data_reference(code, resolve_antestatal_access(input, pointer.target));
        }

        /** Emits the owner and array index operands for one antestatal object cell. */
        static void emit_antestatal_cell_array_access(code_builder& code, type_symbol const& root_symbol, std::vector< std::uint64_t > const& cell_path, antestatal_cell_array_kind array_kind)
        {
            resolved_antestatal_access access{
                .root_symbol = root_symbol,
                .target_type = void_type{},
                .cell_path = cell_path,
            };
            emit_antestatal_cell_owner(code, access);
            switch (array_kind)
            {
            case antestatal_cell_array_kind::values:
                code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                break;
            case antestatal_cell_array_kind::initialized:
                code.getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                break;
            }
            if (cell_path.back() > static_cast< std::uint64_t >(std::numeric_limits< std::int32_t >::max()))
            {
                throw lowering_compilation_error("Quxlang's Cortado backend cannot fill an antestatal subobject beyond the JVM array-index limit");
            }
            emit_int_constant(code, static_cast< std::uint32_t >(cell_path.back()));
        }

        /** Marks one antestatal cell initialized after its complete value has been filled. */
        static void emit_antestatal_cell_initialized(code_builder& code, type_symbol const& root_symbol, std::vector< std::uint64_t > const& cell_path)
        {
            emit_antestatal_cell_array_access(code, root_symbol, cell_path, antestatal_cell_array_kind::initialized);
            code.append< opcode::iconst_1 >().append< opcode::bastore >();
        }

        /** Fills one recursively materialized antestatal value into its allocated managed cell. */
        static void emit_antestatal_value_fill(code_builder& code, cortado_compilable_unit const& input, std::map< type_symbol, routine_jvm_info > const& routine_infos, type_symbol const& root_symbol, std::vector< std::uint64_t >& cell_path, type_symbol type, antestatal_value const& value)
        {
            type = normalized_antestatal_type(std::move(type));
            if (type.type_is< readonly_constant >())
            {
                emit_antestatal_cell_array_access(code, root_symbol, cell_path, antestatal_cell_array_kind::values);
                if (!value.type_is< antestatal_primitive >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a non-primitive readonly constant antestatal value");
                }
                if (type.as< readonly_constant >().kind != constant_kind::string)
                {
                    throw rpnx::unimplemented();
                }
                std::vector< std::byte > const& bytes = value.get_as< antestatal_primitive >().value;
                emit_readonly_byte_span_object(code, bytes, {0});
                code.append< opcode::aastore >();
                emit_antestatal_cell_initialized(code, root_symbol, cell_path);
                return;
            }
            std::map< type_symbol, std::vector< struct_field > >::const_iterator struct_definition = input.struct_definitions.find(type);
            if (struct_definition != input.struct_definitions.end())
            {
                if (!value.type_is< antestatal_struct >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a non-struct value while filling an antestatal struct");
                }
                antestatal_struct const& struct_value = value.get_as< antestatal_struct >();
                for (std::size_t index = 0; index < struct_definition->second.size(); ++index)
                {
                    struct_field const& field = struct_definition->second.at(index);
                    std::map< std::string, antestatal_value >::const_iterator const field_value = struct_value.fields.find(field.name);
                    if (field_value == struct_value.fields.end())
                    {
                        throw compiler_bug("Quxlang's Cortado backend received an antestatal struct without field " + field.name);
                    }
                    cell_path.push_back(index);
                    emit_antestatal_value_fill(code, input, routine_infos, root_symbol, cell_path, field.type, field_value->second);
                    cell_path.pop_back();
                }
                emit_antestatal_cell_initialized(code, root_symbol, cell_path);
                return;
            }

            if (type.type_is< array_type >())
            {
                if (!value.type_is< antestatal_array >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a non-array value while filling an antestatal array");
                }
                antestatal_array const& array_value = value.get_as< antestatal_array >();
                for (std::size_t index = 0; index < array_value.elements.size(); ++index)
                {
                    cell_path.push_back(index);
                    emit_antestatal_value_fill(code, input, routine_infos, root_symbol, cell_path, type.as< array_type >().element_type, array_value.elements.at(index));
                    cell_path.pop_back();
                }
                emit_antestatal_cell_initialized(code, root_symbol, cell_path);
                return;
            }

            if (value.type_is< antestatal_primitive >())
            {
                emit_antestatal_cell_array_access(code, root_symbol, cell_path, antestatal_cell_array_kind::values);
                emit_boxed_antestatal_primitive(code, input, type, value.get_as< antestatal_primitive >());
                code.append< opcode::aastore >();
            }
            else if (value.type_is< antestatal_type_index >())
            {
                if (!type.type_is< type_index_type >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received an antestatal TYPE_INDEX value for a different semantic type");
                }
                type_symbol const& indexed_type = value.get_as< antestatal_type_index >().indexed_type;
                std::map< type_symbol, std::uint64_t >::const_iterator const ordinal = input.type_index_ordinals.find(indexed_type);
                if (ordinal == input.type_index_ordinals.end())
                {
                    throw compiler_bug("Missing Cortado TYPE_INDEX ordinal for " + to_string(indexed_type));
                }
                emit_antestatal_cell_array_access(code, root_symbol, cell_path, antestatal_cell_array_kind::values);
                emit_long_constant(code, ordinal->second);
                emit_boxed_stack_value(code, jvm_value_kind::long_);
                code.append< opcode::aastore >();
            }
            else if (value.type_is< antestatal_ptrref >())
            {
                emit_antestatal_cell_array_access(code, root_symbol, cell_path, antestatal_cell_array_kind::values);
                emit_antestatal_pointer(code, input, routine_infos, type, value.get_as< antestatal_ptrref >());
                code.append< opcode::aastore >();
            }
            else if (value.type_is< antestatal_fusion >())
            {
                std::vector< type_symbol > alternatives = fusion_alternative_types(input, type);
                antestatal_fusion const& fusion = value.get_as< antestatal_fusion >();
                std::uint64_t alternative = alternatives.size();
                std::optional< antestatal_value > const* payload = nullptr;
                if (fusion.state.type_is< antestatal_fusion_active >())
                {
                    antestatal_fusion_active const& active = fusion.state.get_as< antestatal_fusion_active >();
                    if (active.alternative >= alternatives.size())
                    {
                        throw compiler_bug("Quxlang's Cortado backend received an antestatal fusion with an invalid alternative ordinal");
                    }
                    alternative = active.alternative;
                    payload = &active.payload;
                }

                cell_path.push_back(0);
                emit_antestatal_cell_array_access(code, root_symbol, cell_path, antestatal_cell_array_kind::values);
                emit_long_constant(code, alternative);
                emit_boxed_stack_value(code, jvm_value_kind::long_);
                code.append< opcode::aastore >();
                emit_antestatal_cell_initialized(code, root_symbol, cell_path);
                cell_path.pop_back();

                if (alternative < alternatives.size())
                {
                    type_symbol const& alternative_type = alternatives.at(static_cast< std::size_t >(alternative));
                    if (alternative_type.type_is< void_type >())
                    {
                        if (payload != nullptr && payload->has_value())
                        {
                            throw compiler_bug("Quxlang's Cortado backend received a VOID antestatal fusion alternative with a payload");
                        }
                    }
                    else
                    {
                        if (payload == nullptr || !payload->has_value())
                        {
                            throw compiler_bug("Quxlang's Cortado backend received a non-VOID antestatal fusion alternative without a payload");
                        }
                        cell_path.push_back(1);
                        cell_path.push_back(0);
                        emit_antestatal_value_fill(code, input, routine_infos, root_symbol, cell_path, alternative_type, payload->value());
                        cell_path.pop_back();
                        cell_path.pop_back();
                        cell_path.push_back(1);
                        emit_antestatal_cell_initialized(code, root_symbol, cell_path);
                        cell_path.pop_back();
                    }
                }
            }
            else if (value.type_is< antestatal_interface >())
            {
                antestatal_interface const& interface_value = value.get_as< antestatal_interface >();
                if (interface_value.interface_type != type)
                {
                    throw compiler_bug("Quxlang's Cortado backend received an antestatal interface value with a mismatched semantic type while filling storage");
                }
                for (std::pair< interface_slot_key const, type_symbol > const& implementation : interface_value.functions)
                {
                    if (!routine_infos.contains(implementation.second))
                    {
                        throw compiler_bug("Quxlang's Cortado backend antestatal interface implementation is outside the aggregated closure: " + to_string(implementation.second));
                    }
                    emit_antestatal_cell_array_access(code, root_symbol, cell_path, antestatal_cell_array_kind::values);
                    code.append< opcode::aaload >().checkcast("quxlang/runtime/QuxlangInterface").getfield("quxlang/runtime/QuxlangInterface", "functions", "Ljava/util/HashMap;").ldc_string(to_string(implementation.first));
                    std::string const adapter_name = callable_adapter_class_name(implementation.second);
                    code.new_(adapter_name).append< opcode::dup >().invokespecial(adapter_name, "<init>", "()V");
                    code.invokevirtual("java/util/HashMap", "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;").append< opcode::pop >();
                }
            }
            else
            {
                throw compiler_bug("Quxlang's Cortado backend received an unsupported antestatal value and semantic type combination");
            }
            emit_antestatal_cell_initialized(code, root_symbol, cell_path);
        }

        /** Emits the JVM return instruction corresponding to a carrier category. */
        static void emit_return(code_builder& code, jvm_value_kind kind)
        {
            switch (kind)
            {
            case jvm_value_kind::void_:
                code.append< opcode::return_ >();
                return;
            case jvm_value_kind::integer:
                code.append< opcode::ireturn >();
                return;
            case jvm_value_kind::long_:
                code.append< opcode::lreturn >();
                return;
            case jvm_value_kind::big_integer:
                code.append< opcode::areturn >();
                return;
            case jvm_value_kind::float_:
                code.append< opcode::freturn >();
                return;
            case jvm_value_kind::double_:
                code.append< opcode::dreturn >();
                return;
            case jvm_value_kind::reference:
                code.append< opcode::areturn >();
                return;
            }
        }

        /** Emits construction and throwing of a generated runtime failure. */
        static void emit_runtime_exception(code_builder& code, std::string message)
        {
            code.new_("java/lang/RuntimeException").append< opcode::dup >().ldc_string(std::move(message)).invokespecial("java/lang/RuntimeException", "<init>", "(Ljava/lang/String;)V").append< opcode::athrow >();
        }

        /** Validates and serializes one generated Java classfile. */
        static auto validated_class_bytes(rpnx::cortado::class_file const& class_file) -> std::vector< std::byte >
        {
            rpnx::cortado::validation_report const report = rpnx::cortado::validate_class_file(class_file);
            if (!report.valid())
            {
                throw compiler_bug("Quxlang's Cortado backend generated an invalid classfile: " + report.diagnostics.front().message);
            }
            return rpnx::cortado::serialize_class_file(class_file);
        }

        /** Returns whether an assembly failure reports a hard JVM classfile encoding limit. */
        static auto assembly_failure_is_classfile_limit(std::string_view message) -> bool
        {
            return message.starts_with("constant pool exceeds the JVM u2 slot limit") || message.starts_with("code-builder label-owner identifier space was exhausted") || message.starts_with("invokeinterface argument count exceeds u1") || message.starts_with("assembled Code body exceeds 65535 bytes") || message.starts_with("tableswitch target displacement exceeds s4") || message.starts_with("computed max_locals exceeds u2") || message.starts_with("computed max_stack exceeds u2") || message.starts_with("StackMapTable offset_delta exceeds u2");
        }

        /** Lowers one VMIR routine into a Java 17 class with a static invoke method. */
        class routine_emitter
        {
            /** Defers a struct-field store until VMIR reports that initialization completed. */
            struct pending_struct_initializer
            {
                vmir2::local_index target;
                std::map< std::string, vmir2::local_index > fields;
            };

          public:
            /** Creates a per-routine emitter over one immutable compilation packet. */
            routine_emitter(cortado_compilable_unit const& input, type_symbol const& symbol, vmir2::functanoid_routine3 const& routine, std::map< type_symbol, routine_jvm_info > const& routine_infos) : m_input(input), m_symbol(symbol), m_routine(routine), m_routine_infos(routine_infos)
            {
            }

            /** Emits and validates the static JVM method body for this routine. */
            auto emit(std::string const& class_name) -> rpnx::cortado::class_file
            {
                if (m_routine.blocks.empty())
                {
                    throw compiler_bug("Quxlang's Cortado backend received a routine without VMIR blocks: " + to_string(m_symbol));
                }

                assign_local_slots();
                for (std::size_t i = 0; i < m_routine.blocks.size(); ++i)
                {
                    m_block_labels.push_back(m_code.new_label());
                }
                std::vector< bool > const reachable_blocks = runtime_reachable_blocks();

                initialize_parameter_locals_from_frame();
                initialize_non_parameter_locals();
                for (std::size_t block_index = 0; block_index < m_routine.blocks.size(); ++block_index)
                {
                    if (!reachable_blocks.at(block_index))
                    {
                        continue;
                    }
                    m_code.bind(m_block_labels.at(block_index));
                    vmir2::executable_block const& block = m_routine.blocks.at(block_index);
                    vmir2::state_map current_state = block.entry_state;
                    if (block_index == 0 && current_state.empty())
                    {
                        vmir2::codegen_state_engine entry_engine(current_state, m_routine.local_types, m_routine.parameters);
                        entry_engine.apply_entry();
                    }
                    vmir2::codegen_state_engine state_engine(current_state, m_routine.local_types, m_routine.parameters);
                    for (vmir2::vm_instruction const& instruction : block.instructions)
                    {
                        vmir2::state_map const previous_state = current_state;
                        emit_instruction(instruction, current_state);
                        state_engine.apply(instruction);
                        if (instruction.type_is< vmir2::defer_nontrivial_dtor >())
                        {
                            vmir2::defer_nontrivial_dtor const& deferred = instruction.as< vmir2::defer_nontrivial_dtor >();
                            current_state.at(deferred.on_value).nontrivial_dtor = vmir2::dtor_spec{
                                .func = deferred.func,
                                .args = deferred.args,
                            };
                        }
                        emit_completed_array_elements(previous_state, current_state);
                        emit_completed_struct_fields(previous_state, current_state);
                    }
                    if (!block.terminator.has_value())
                    {
                        throw compiler_bug("Quxlang's Cortado backend received an unterminated VMIR block in " + to_string(m_symbol));
                    }
                    emit_terminator(*block.terminator, current_state);
                }

                rpnx::cortado::class_file_builder builder(class_name, "java/lang/Object", {0, 61});
                builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;
                jvm_class_hierarchy hierarchy;
                static_cast< void >(builder.add_method("invoke", m_routine_infos.at(m_symbol).descriptor, rpnx::cortado::method_access_flags::is_public | rpnx::cortado::method_access_flags::is_static, m_code, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
                return builder.build();
            }

          private:
            /** Returns the VMIR blocks reachable when RUNTIME selects its native execution arm. */
            auto runtime_reachable_blocks() const -> std::vector< bool >
            {
                std::vector< bool > reachable(m_routine.blocks.size(), false);
                std::vector< std::size_t > pending{0};
                while (!pending.empty())
                {
                    std::size_t const block_index = pending.back();
                    pending.pop_back();
                    if (block_index >= m_routine.blocks.size())
                    {
                        throw compiler_bug("Quxlang's Cortado backend VMIR terminator targets an invalid block");
                    }
                    if (reachable.at(block_index))
                    {
                        continue;
                    }
                    reachable.at(block_index) = true;
                    std::optional< vmir2::vm_terminator > const& terminator = m_routine.blocks.at(block_index).terminator;
                    if (!terminator.has_value())
                    {
                        continue;
                    }
                    rpnx::apply_visitor< void >(*terminator,
                                                [&](auto& selected) -> void
                                                {
                                                    using terminator_type = std::decay_t< decltype(selected) >;
                                                    if constexpr (std::is_same_v< terminator_type, vmir2::jump >)
                                                    {
                                                        pending.push_back(block_slot(selected.target));
                                                    }
                                                    else if constexpr (std::is_same_v< terminator_type, vmir2::branch >)
                                                    {
                                                        pending.push_back(block_slot(selected.target_true));
                                                        pending.push_back(block_slot(selected.target_false));
                                                    }
                                                    else if constexpr (std::is_same_v< terminator_type, vmir2::tablebranch >)
                                                    {
                                                        pending.push_back(block_slot(selected.default_target));
                                                        for (vmir2::block_index const target : selected.targets)
                                                        {
                                                            pending.push_back(block_slot(target));
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< terminator_type, vmir2::runtime_constexpr >)
                                                    {
                                                        pending.push_back(block_slot(selected.target_native));
                                                    }
                                                    else if constexpr (std::is_same_v< terminator_type, vmir2::initguard_try_acquire >)
                                                    {
                                                        pending.push_back(block_slot(selected.target_acquired));
                                                        pending.push_back(block_slot(selected.target_already_initialized));
                                                    }
                                                });
                }
                return reachable;
            }

            /** Returns the JVM carrier category assigned to a VMIR local. */
            auto kind_of(vmir2::local_index index) const -> jvm_value_kind
            {
                return value_kind(m_input, m_routine.local_types.at(local_slot(index)).type);
            }

            /** Returns whether a VMIR local carries a raw JVM GC pointer. */
            auto local_is_gc_pointer(vmir2::local_index index) const -> bool
            {
                type_symbol const type = unwrapped_type(m_routine.local_types.at(local_slot(index)).type);
                return type.type_is< ptrref_type >() && type.get_as< ptrref_type >().ptr_class == pointer_class::gc;
            }

            /** Returns the deterministic JVM local slot assigned to a VMIR local. */
            auto jvm_slot(vmir2::local_index index) const -> local_variable_index
            {
                std::optional< local_variable_index > const& result = m_local_slots.at(local_slot(index));
                if (!result.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend attempted to use a void VMIR local");
                }
                return *result;
            }

            /** Returns whether a VMIR local denotes named value storage rather than a transient value. */
            auto local_has_value_storage(vmir2::local_index index) const -> bool
            {
                type_symbol const& type = m_routine.local_types.at(local_slot(index)).type;
                return type.type_is< nvalue_slot >() || type.type_is< dvalue_slot >();
            }

            /** Follows transparent VMIR reference aliases to their stored target local. */
            auto resolved_reference(vmir2::local_index index) const -> vmir2::local_index
            {
                std::set< vmir2::local_index > seen;
                std::optional< vmir2::local_index > current = index;
                while (m_reference_aliases.at(local_slot(*current)).has_value())
                {
                    if (!seen.insert(*current).second)
                    {
                        throw compiler_bug("Cyclic Quxlang Cortado backend reference alias");
                    }
                    current = m_reference_aliases.at(local_slot(*current));
                }
                return *current;
            }

            /** Assigns JVM local storage to one direct or argument-frame parameter. */
            void assign_parameter(vmir2::routine_parameter const& parameter, std::uint16_t& next_slot)
            {
                jvm_value_kind const kind = kind_of(parameter.local_index);
                m_local_slots.at(local_slot(parameter.local_index)) = local_variable_index{next_slot};
                m_parameter_locals.insert(parameter.local_index);
                next_slot = static_cast< std::uint16_t >(next_slot + value_slot_count(kind));
            }

            /** Assigns deterministic JVM slots and scratch storage for the complete routine. */
            void assign_local_slots()
            {
                m_local_slots.resize(m_routine.local_types.size());
                m_reference_aliases.resize(m_routine.local_types.size());
                m_storage_initializers.resize(m_routine.local_types.size());
                m_storage_deinitializers.resize(m_routine.local_types.size());
                m_array_element_reference_slots.resize(m_routine.local_types.size());
                m_scalar_reference_owner_slots.resize(m_routine.local_types.size());
                std::uint16_t next_slot = m_routine_infos.at(m_symbol).argument_frame_class_name.has_value() ? 1 : 0;
                for (vmir2::routine_parameter const& parameter : m_routine.parameters.positional)
                {
                    assign_parameter(parameter, next_slot);
                }
                for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : m_routine.parameters.named)
                {
                    if (parameter.first != "RETURN")
                    {
                        assign_parameter(parameter.second, next_slot);
                    }
                }
                for (std::size_t i = 0; i < m_routine.local_types.size(); ++i)
                {
                    if (m_local_slots.at(i).has_value())
                    {
                        continue;
                    }
                    jvm_value_kind const kind = value_kind(m_input, m_routine.local_types.at(i).type);
                    if (kind == jvm_value_kind::void_)
                    {
                        continue;
                    }
                    std::uint16_t const slots = value_slot_count(kind);
                    if (next_slot > std::numeric_limits< std::uint16_t >::max() - slots)
                    {
                        throw lowering_compilation_error("Quxlang's Cortado backend cannot emit a routine that exceeds the JVM local-variable limit: " + to_string(m_symbol));
                    }
                    m_local_slots.at(i) = local_variable_index{next_slot};
                    next_slot = static_cast< std::uint16_t >(next_slot + slots);
                }
                for (vmir2::executable_block const& block : m_routine.blocks)
                {
                    for (vmir2::vm_instruction const& instruction : block.instructions)
                    {
                        rpnx::apply_visitor< void >(instruction,
                                                    [&](auto&& selected) -> void
                                                    {
                                                        using instruction_type = std::remove_cvref_t< decltype(selected) >;
                                                        if constexpr (std::is_same_v< instruction_type, vmir2::array_init_element >)
                                                        {
                                                            std::optional< local_variable_index >& reference_slot = m_array_element_reference_slots.at(local_slot(selected.target));
                                                            if (reference_slot.has_value())
                                                            {
                                                                return;
                                                            }
                                                            if (next_slot == std::numeric_limits< std::uint16_t >::max())
                                                            {
                                                                throw lowering_compilation_error("Quxlang's Cortado backend has no JVM local slot available for array initialization: " + to_string(m_symbol));
                                                            }
                                                            reference_slot = local_variable_index{next_slot};
                                                            ++next_slot;
                                                        }
                                                        else if constexpr (std::is_same_v< instruction_type, vmir2::swap > || std::is_same_v< instruction_type, vmir2::fusion_swap_boxed_state >)
                                                        {
                                                            m_contains_swap_instruction = true;
                                                        }
                                                    });
                    }
                }
                collect_reference_aliases();
                for (std::size_t i = 0; i < m_reference_aliases.size(); ++i)
                {
                    if (!m_reference_aliases.at(i).has_value())
                    {
                        continue;
                    }
                    vmir2::local_index const target = resolved_reference(vmir2::local_index(i));
                    jvm_value_kind const target_kind = kind_of(target);
                    type_symbol const target_type = unwrapped_type(m_routine.local_types.at(local_slot(target)).type);
                    bool const is_aggregate = m_input.struct_definitions.contains(target_type) || target_type.type_is< array_type >() || target_type.type_is< storage >() || is_semantic_fusion_type(m_input, target_type);
                    if (is_aggregate || target_kind == jvm_value_kind::void_)
                    {
                        continue;
                    }
                    std::optional< local_variable_index >& owner_slot = m_scalar_reference_owner_slots.at(local_slot(target));
                    if (owner_slot.has_value())
                    {
                        continue;
                    }
                    if (next_slot == std::numeric_limits< std::uint16_t >::max())
                    {
                        throw lowering_compilation_error("Quxlang's Cortado backend has no JVM local slot available for scalar reference storage: " + to_string(m_symbol));
                    }
                    owner_slot = local_variable_index{next_slot};
                    ++next_slot;
                }
                if (next_slot == std::numeric_limits< std::uint16_t >::max())
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend has no JVM local slot available for managed-reference lowering: " + to_string(m_symbol));
                }
                m_reference_owner_slot = local_variable_index{next_slot};
                if (m_contains_swap_instruction)
                {
                    if (next_slot > std::numeric_limits< std::uint16_t >::max() - 3)
                    {
                        throw lowering_compilation_error("Quxlang's Cortado backend has no JVM local slots available for SWAP lowering: " + to_string(m_symbol));
                    }
                    m_swap_a_value_slot = local_variable_index{static_cast< std::uint16_t >(next_slot + 1)};
                    m_swap_b_value_slot = local_variable_index{static_cast< std::uint16_t >(next_slot + 2)};
                }
            }

            /** Loads oversized-signature parameters from their generated argument frame. */
            void initialize_parameter_locals_from_frame()
            {
                std::optional< std::string > const& frame_class_name = m_routine_infos.at(m_symbol).argument_frame_class_name;
                if (!frame_class_name.has_value())
                {
                    return;
                }

                std::size_t parameter_index = 0;
                auto initialize_parameter = [&](vmir2::routine_parameter const& parameter) -> void
                {
                    jvm_value_kind const kind = kind_of(parameter.local_index);
                    m_code.aload({0}).getfield(*frame_class_name, "p" + std::to_string(parameter_index), descriptor_for_kind(kind));
                    emit_store(m_code, kind, jvm_slot(parameter.local_index));
                    ++parameter_index;
                };
                for (vmir2::routine_parameter const& parameter : m_routine.parameters.positional)
                {
                    initialize_parameter(parameter);
                }
                for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : m_routine.parameters.named)
                {
                    if (parameter.first != "RETURN")
                    {
                        initialize_parameter(parameter.second);
                    }
                }
            }

            /** Collects transparent local aliases and storage initialization relationships from VMIR. */
            void collect_reference_aliases()
            {
                bool changed = true;
                while (changed)
                {
                    changed = false;
                    for (vmir2::executable_block const& block : m_routine.blocks)
                    {
                        for (vmir2::vm_instruction const& instruction : block.instructions)
                        {
                            rpnx::apply_visitor< void >(instruction,
                                                        [&](auto& selected) -> void
                                                        {
                                                            using instruction_type = std::decay_t< decltype(selected) >;
                                                            if constexpr (std::is_same_v< instruction_type, vmir2::make_reference >)
                                                            {
                                                                std::optional< vmir2::local_index >& alias = m_reference_aliases.at(local_slot(selected.reference_index));
                                                                if (alias != selected.value_index)
                                                                {
                                                                    alias = selected.value_index;
                                                                    changed = true;
                                                                }
                                                            }
                                                            else if constexpr (std::is_same_v< instruction_type, vmir2::copy_reference >)
                                                            {
                                                                if (local_has_value_storage(selected.to_index))
                                                                {
                                                                    return;
                                                                }
                                                                std::optional< vmir2::local_index > const resolved = m_reference_aliases.at(local_slot(selected.from_index));
                                                                if (resolved.has_value() && m_reference_aliases.at(local_slot(selected.to_index)) != resolved)
                                                                {
                                                                    m_reference_aliases.at(local_slot(selected.to_index)) = resolved;
                                                                    changed = true;
                                                                }
                                                            }
                                                            else if constexpr (std::is_same_v< instruction_type, vmir2::cast_ptrref >)
                                                            {
                                                                if (local_has_value_storage(selected.target_index))
                                                                {
                                                                    return;
                                                                }
                                                                std::optional< vmir2::local_index > const resolved = m_reference_aliases.at(local_slot(selected.source_index));
                                                                if (resolved.has_value() && m_reference_aliases.at(local_slot(selected.target_index)) != resolved)
                                                                {
                                                                    m_reference_aliases.at(local_slot(selected.target_index)) = resolved;
                                                                    changed = true;
                                                                }
                                                            }
                                                            else if constexpr (std::is_same_v< instruction_type, vmir2::storage_init_start >)
                                                            {
                                                                vmir2::local_index const storage_local = resolved_reference(selected.on_storage);
                                                                m_storage_initializers.at(local_slot(storage_local)) = selected.target_value;
                                                                m_storage_initializer_targets.insert(selected.target_value);
                                                            }
                                                            else if constexpr (std::is_same_v< instruction_type, vmir2::storage_deinit_start >)
                                                            {
                                                                m_storage_deinitializers.at(local_slot(selected.target_value)) = selected.on_storage;
                                                            }
                                                            else if constexpr (std::is_same_v< instruction_type, vmir2::dereference_pointer >)
                                                            {
                                                                type_symbol const pointer_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.from_pointer)).type);
                                                                if (!pointer_type.type_is< ptrref_type >() || !pointer_type.as< ptrref_type >().target.type_is< procedure_type >())
                                                                {
                                                                    return;
                                                                }
                                                                vmir2::local_index const resolved = resolved_reference(selected.from_pointer);
                                                                if (m_reference_aliases.at(local_slot(selected.to_reference)) != resolved)
                                                                {
                                                                    m_reference_aliases.at(local_slot(selected.to_reference)) = resolved;
                                                                    changed = true;
                                                                }
                                                            }
                                                        });
                        }
                    }
                }
            }

            /** Allocates an uninitialized managed object for one Quxlang aggregate or array type. */
            void emit_new_composite_object(type_symbol type)
            {
                type = unwrapped_type(std::move(type));
                std::map< type_symbol, std::vector< struct_field > >::const_iterator const aggregate = m_input.struct_definitions.find(type);
                if (aggregate != m_input.struct_definitions.end())
                {
                    m_code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                    emit_int_constant(m_code, static_cast< std::uint32_t >(aggregate->second.size()));
                    m_code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V");
                    return;
                }
                if (is_semantic_fusion_type(m_input, type))
                {
                    m_code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                    emit_int_constant(m_code, 2);
                    m_code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V");
                    return;
                }
                if (!type.type_is< array_type >())
                {
                    throw compiler_bug("Quxlang's Cortado backend cannot allocate managed composite storage for " + to_string(type));
                }

                array_type const& array = type.as< array_type >();
                if (!array.element_count.type_is< expression_numeric_literal >())
                {
                    throw compiler_bug("Quxlang's Cortado backend received an array without a resolved element count");
                }
                std::uint64_t const count = parsers::str_to_int< std::uint64_t >(array.element_count.as< expression_numeric_literal >().value);
                if (count > static_cast< std::uint64_t >(std::numeric_limits< std::int32_t >::max()))
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend cannot emit an array whose element count exceeds the JVM array-index limit");
                }
                m_code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                emit_int_constant(m_code, static_cast< std::uint32_t >(count));
                m_code.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V");
            }

            /** Initializes JVM locals, allocating managed aggregate storage at Quxlang storage initialization. */
            void initialize_non_parameter_locals()
            {
                for (std::size_t i = 0; i < m_routine.local_types.size(); ++i)
                {
                    vmir2::local_index const index(i);
                    if (m_parameter_locals.contains(index) || !m_local_slots.at(i).has_value())
                    {
                        continue;
                    }
                    jvm_value_kind const kind = value_kind(m_input, m_routine.local_types.at(i).type);
                    type_symbol const type = unwrapped_type(m_routine.local_types.at(i).type);
                    std::map< type_symbol, std::vector< struct_field > >::const_iterator const aggregate = m_input.struct_definitions.find(type);
                    if (kind == jvm_value_kind::reference && m_storage_initializer_targets.contains(index))
                    {
                        m_code.append< opcode::aconst_null >();
                    }
                    else if (kind == jvm_value_kind::reference && (aggregate != m_input.struct_definitions.end() || type.type_is< array_type >() || is_semantic_fusion_type(m_input, type)))
                    {
                        emit_new_composite_object(type);
                    }
                    else
                    {
                        emit_default_value(m_code, kind);
                    }
                    emit_store(m_code, kind, *m_local_slots.at(i));
                }
                for (std::optional< local_variable_index > const& reference_slot : m_array_element_reference_slots)
                {
                    if (!reference_slot.has_value())
                    {
                        continue;
                    }
                    m_code.append< opcode::aconst_null >().astore(*reference_slot);
                }
                for (std::optional< local_variable_index > const& owner_slot : m_scalar_reference_owner_slots)
                {
                    if (!owner_slot.has_value())
                    {
                        continue;
                    }
                    m_code.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >().append< opcode::iconst_1 >().invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").astore(*owner_slot);
                }
            }

            /** Copies a VMIR carrier between its assigned JVM local slots. */
            void copy_local(vmir2::local_index from, vmir2::local_index to)
            {
                jvm_value_kind const from_kind = kind_of(from);
                jvm_value_kind const to_kind = kind_of(to);
                if (from_kind != to_kind)
                {
                    throw compiler_bug("Quxlang's Cortado backend received incompatible JVM local kinds in " + to_string(m_symbol));
                }
                emit_load(m_code, from_kind, jvm_slot(from));
                emit_store(m_code, to_kind, jvm_slot(to));
            }

            /** Returns whether a pointer addresses a declared TYPED_STORAGE value. */
            auto pointer_targets_storage(vmir2::local_index pointer) const -> bool
            {
                type_symbol const type = unwrapped_type(m_routine.local_types.at(local_slot(pointer)).type);
                return type.type_is< ptrref_type >() && type.as< ptrref_type >().target.type_is< storage >();
            }

            /** Returns the semantic value type addressed by a VMIR reference local. */
            auto referenced_value_type(vmir2::local_index reference) const -> type_symbol
            {
                type_symbol type = unwrapped_type(m_routine.local_types.at(local_slot(reference)).type);
                if (!type.type_is< ptrref_type >())
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating operation received a non-reference target");
                }
                return unwrapped_type(type.as< ptrref_type >().target);
            }

            /** Returns a semantic struct field's stable managed-storage index. */
            auto aggregate_field_index(vmir2::local_index base, std::string const& field_name) const -> std::size_t
            {
                type_symbol type = unwrapped_type(m_routine.local_types.at(local_slot(base)).type);
                if (type.type_is< ptrref_type >())
                {
                    type = unwrapped_type(type.as< ptrref_type >().target);
                }
                std::map< type_symbol, std::vector< struct_field > >::const_iterator const aggregate = m_input.struct_definitions.find(type);
                if (aggregate == m_input.struct_definitions.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend field access is missing the semantic struct definition for " + to_string(type));
                }
                for (std::size_t index = 0; index < aggregate->second.size(); ++index)
                {
                    if (aggregate->second.at(index).name == field_name)
                    {
                        return index;
                    }
                }
                throw compiler_bug("Quxlang's Cortado backend VMIR names an unknown aggregate field: " + field_name);
            }

            /** Returns the resolved semantic type of a named aggregate field. */
            auto aggregate_field_type(vmir2::local_index base, std::string const& field_name) const -> type_symbol
            {
                type_symbol type = unwrapped_type(m_routine.local_types.at(local_slot(base)).type);
                if (type.type_is< ptrref_type >())
                {
                    type = unwrapped_type(type.as< ptrref_type >().target);
                }
                std::map< type_symbol, std::vector< struct_field > >::const_iterator const aggregate = m_input.struct_definitions.find(type);
                if (aggregate == m_input.struct_definitions.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend field access is missing the semantic struct definition for " + to_string(type));
                }
                for (struct_field const& field : aggregate->second)
                {
                    if (field.name == field_name)
                    {
                        return unwrapped_type(field.type);
                    }
                }
                throw compiler_bug("Quxlang's Cortado backend VMIR names an unknown aggregate field: " + field_name);
            }

            /** Loads the managed object represented by a composite local or a reference to one. */
            void emit_composite_object(vmir2::local_index base)
            {
                type_symbol const type = unwrapped_type(m_routine.local_types.at(local_slot(base)).type);
                if (type.type_is< ptrref_type >())
                {
                    if (m_reference_aliases.at(local_slot(base)).has_value())
                    {
                        vmir2::local_index const target = resolved_reference(base);
                        type_symbol const target_type = unwrapped_type(m_routine.local_types.at(local_slot(target)).type);
                        bool const target_is_composite = m_input.struct_definitions.contains(target_type) || target_type.type_is< array_type >() || target_type.type_is< storage >() || is_semantic_fusion_type(m_input, target_type);
                        if (!target_is_composite)
                        {
                            throw compiler_bug("Quxlang's Cortado backend resolved a composite reference to a scalar local");
                        }
                        m_code.aload(jvm_slot(target)).checkcast("quxlang/runtime/QuxlangObject");
                        return;
                    }
                    emit_managed_reference_owner(base);
                    label const stored_object = m_code.new_label();
                    label const object_ready = m_code.new_label();
                    m_code.aload(jvm_slot(base)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J");
                    emit_long_constant(m_code, std::numeric_limits< std::uint64_t >::max());
                    m_code.append< opcode::lcmp >().branch< opcode::ifne >(stored_object).aload(m_reference_owner_slot).branch< opcode::goto_ >(object_ready).bind(stored_object).aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                    emit_managed_reference_index(base);
                    m_code.append< opcode::aaload >().bind(object_ready);
                }
                else
                {
                    m_code.aload(jvm_slot(base));
                }
                m_code.checkcast("quxlang/runtime/QuxlangObject");
            }

            /** Returns the semantic UNION or VARIANT type represented by a direct value or reference local. */
            auto fusion_type_of(vmir2::local_index local) const -> type_symbol
            {
                type_symbol type = unwrapped_type(m_routine.local_types.at(local_slot(local)).type);
                if (type.type_is< ptrref_type >())
                {
                    type = unwrapped_type(type.as< ptrref_type >().target);
                }
                if (!is_semantic_fusion_type(m_input, type))
                {
                    throw compiler_bug("Quxlang's Cortado backend received a fusion instruction for a non-fusion local");
                }
                return type;
            }

            /** Loads the layout-independent declaration-order discriminator of a managed fusion object. */
            void emit_fusion_tag(vmir2::local_index subject)
            {
                emit_composite_object(subject);
                m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >().append< opcode::aaload >().checkcast("java/lang/Long").invokevirtual("java/lang/Long", "longValue", "()J");
            }

            /** Stores one declaration-order discriminator and marks the managed fusion tag initialized. */
            void emit_fusion_tag_store(vmir2::local_index target, std::uint64_t alternative)
            {
                emit_composite_object(target);
                m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >();
                emit_long_constant(m_code, alternative);
                cortado_jar_emitter_impl::emit_boxed_stack_value(m_code, jvm_value_kind::long_);
                m_code.append< opcode::aastore >();
                emit_composite_object(target);
                m_code.getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::iconst_0 >().append< opcode::iconst_1 >().append< opcode::bastore >();
            }

            /** Loads a fusion's payload-storage object, allocating embedded inline storage when requested. */
            void emit_fusion_payload_storage(vmir2::local_index subject, bool allocate_if_absent)
            {
                type_symbol type = fusion_type_of(subject);
                if (allocate_if_absent && !fusion_properties_for_type(m_input, type).is_inline)
                {
                    throw compiler_bug("Quxlang's Cortado backend cannot allocate embedded payload storage for a boxed fusion");
                }

                emit_composite_object(subject);
                m_code.astore(m_reference_owner_slot).aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().append< opcode::aaload >();
                if (allocate_if_absent)
                {
                    label const storage_ready = m_code.new_label();
                    m_code.append< opcode::dup >().branch< opcode::ifnonnull >(storage_ready).append< opcode::pop >().aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().new_("quxlang/runtime/QuxlangObject").append< opcode::dup >().append< opcode::iconst_1 >().invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").append< opcode::aastore >().aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().append< opcode::aaload >().bind(storage_ready);
                }
                m_code.checkcast("quxlang/runtime/QuxlangObject");
            }

            /** Exchanges the opaque payload storage and discriminator of two boxed fusion values. */
            void emit_fusion_swap_boxed_state(vmir2::fusion_swap_boxed_state const& instruction)
            {
                type_symbol a_type = fusion_type_of(instruction.a);
                type_symbol b_type = fusion_type_of(instruction.b);
                if (a_type != b_type || fusion_properties_for_type(m_input, a_type).is_inline)
                {
                    throw compiler_bug("Quxlang's Cortado backend received FUSION_SWAP_BOXED_STATE for incompatible fusion types");
                }
                if (!m_swap_a_value_slot.has_value() || !m_swap_b_value_slot.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend boxed fusion swap has no assigned JVM scratch slots");
                }

                emit_composite_object(instruction.a);
                m_code.astore(*m_swap_a_value_slot);
                emit_composite_object(instruction.b);
                m_code.astore(m_reference_owner_slot);

                m_code.aload(*m_swap_a_value_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >().append< opcode::aaload >().astore(*m_swap_b_value_slot);
                m_code.aload(*m_swap_a_value_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >().aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >().append< opcode::aaload >().append< opcode::aastore >();
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >().aload(*m_swap_b_value_slot).append< opcode::aastore >();

                m_code.aload(*m_swap_a_value_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().append< opcode::aaload >().astore(*m_swap_b_value_slot);
                m_code.aload(*m_swap_a_value_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().append< opcode::aaload >().append< opcode::aastore >();
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().aload(*m_swap_b_value_slot).append< opcode::aastore >();
            }

            /** Loads the managed allocation containing one TYPED_STORAGE cell. */
            void emit_storage_owner(vmir2::local_index storage_reference)
            {
                type_symbol const type = unwrapped_type(m_routine.local_types.at(local_slot(storage_reference)).type);
                if (type.type_is< ptrref_type >())
                {
                    emit_managed_reference_owner(storage_reference);
                    m_code.aload(m_reference_owner_slot);
                    return;
                }
                if (!type.type_is< storage >())
                {
                    throw compiler_bug("Quxlang's Cortado backend storage operation received a non-storage local");
                }
                m_code.aload(jvm_slot(storage_reference)).checkcast("quxlang/runtime/QuxlangObject");
            }

            /** Loads the managed allocation identified by a direct or recovered single-storage pointer. */
            void emit_single_storage_allocation(vmir2::local_index storage_pointer)
            {
                label const direct_allocation = m_code.new_label();
                label const allocation_ready = m_code.new_label();
                m_code.aload(jvm_slot(storage_pointer)).instanceof_("quxlang/runtime/QuxlangReference").branch< opcode::ifeq >(direct_allocation);
                m_code.aload(jvm_slot(storage_pointer)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").branch< opcode::goto_ >(allocation_ready);
                m_code.bind(direct_allocation).aload(jvm_slot(storage_pointer)).checkcast("quxlang/runtime/QuxlangObject").bind(allocation_ready);
            }

            /** Loads the checked managed-allocation index containing one TYPED_STORAGE cell. */
            void emit_storage_index(vmir2::local_index storage_reference)
            {
                type_symbol const type = unwrapped_type(m_routine.local_types.at(local_slot(storage_reference)).type);
                if (!type.type_is< ptrref_type >())
                {
                    if (!type.type_is< storage >())
                    {
                        throw compiler_bug("Quxlang's Cortado backend storage operation received a non-storage local");
                    }
                    m_code.append< opcode::iconst_0 >();
                    return;
                }

                label const allocated_element = m_code.new_label();
                label const complete = m_code.new_label();
                m_code.aload(jvm_slot(storage_reference)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J");
                emit_long_constant(m_code, std::numeric_limits< std::uint64_t >::max());
                m_code.append< opcode::lcmp >().branch< opcode::ifne >(allocated_element).append< opcode::iconst_0 >().branch< opcode::goto_ >(complete);
                m_code.bind(allocated_element);
                emit_managed_reference_index(storage_reference);
                m_code.bind(complete);
            }

            /** Copies a direct scalar JVM local into its managed reference owner. */
            void emit_scalar_reference_owner_value(vmir2::local_index target)
            {
                std::optional< local_variable_index > const& owner_slot = m_scalar_reference_owner_slots.at(local_slot(target));
                if (!owner_slot.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend scalar reference has no managed storage owner");
                }
                m_code.aload(*owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >();
                emit_boxed_value(target);
                m_code.append< opcode::aastore >().aload(*owner_slot).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::iconst_0 >().append< opcode::iconst_1 >().append< opcode::bastore >();
            }

            /** Emits one call argument, materializing local reference aliases when required. */
            void emit_call_argument(vmir2::local_index argument)
            {
                if (!m_reference_aliases.at(local_slot(argument)).has_value())
                {
                    emit_load(m_code, kind_of(argument), jvm_slot(argument));
                    return;
                }

                vmir2::local_index const target = resolved_reference(argument);
                type_symbol const target_type = unwrapped_type(m_routine.local_types.at(local_slot(target)).type);
                bool const is_aggregate = m_input.struct_definitions.contains(target_type) || target_type.type_is< array_type >() || target_type.type_is< storage >() || is_semantic_fusion_type(m_input, target_type);
                if (is_aggregate)
                {
                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(jvm_slot(target)).checkcast("quxlang/runtime/QuxlangObject");
                    emit_long_constant(m_code, std::numeric_limits< std::uint64_t >::max());
                    m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V");
                    return;
                }

                emit_scalar_reference_owner_value(target);
                std::optional< local_variable_index > const& owner_slot = m_scalar_reference_owner_slots.at(local_slot(target));
                m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(*owner_slot).append< opcode::lconst_0 >();
                m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V");
            }

            /** Emits one call argument according to the callee's declared parameter type. */
            void emit_call_argument(vmir2::local_index argument, type_symbol const& parameter_type)
            {
                type_symbol const& argument_type = m_routine.local_types.at(local_slot(argument)).type;
                if (parameter_type.type_is< dvalue_slot >() && is_ref(argument_type))
                {
                    type_symbol const target_type = unwrapped_type(parameter_type);
                    bool const is_composite = m_input.struct_definitions.contains(target_type) || target_type.type_is< array_type >() || target_type.type_is< storage >() || is_semantic_fusion_type(m_input, target_type);
                    if (!is_composite)
                    {
                        throw compiler_bug("Quxlang's Cortado backend cannot pass a reference to a scalar dvalue parameter");
                    }
                    emit_composite_object(argument);
                    return;
                }
                emit_call_argument(argument);
            }

            /** Copies a scalar reference argument from its managed owner back to its JVM local. */
            void emit_scalar_reference_result(vmir2::local_index target)
            {
                std::optional< local_variable_index > const& owner_slot = m_scalar_reference_owner_slots.at(local_slot(target));
                if (!owner_slot.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend scalar reference has no managed storage owner");
                }
                label const not_initialized = m_code.new_label();
                label const finished = m_code.new_label();
                m_code.aload(*owner_slot).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::iconst_0 >().append< opcode::baload >().branch< opcode::ifeq >(not_initialized).aload(*owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >().append< opcode::aaload >();
                jvm_value_kind const kind = kind_of(target);
                emit_unboxed_value(kind);
                emit_store(m_code, kind, jvm_slot(target));
                canonicalize_integer(target);
                m_code.branch< opcode::goto_ >(finished).bind(not_initialized).bind(finished);
            }

            /** Refreshes direct scalar JVM locals from initialized managed reference owners. */
            void emit_scalar_reference_results()
            {
                for (std::size_t i = 0; i < m_scalar_reference_owner_slots.size(); ++i)
                {
                    if (!m_scalar_reference_owner_slots.at(i).has_value())
                    {
                        continue;
                    }
                    emit_scalar_reference_result(vmir2::local_index(i));
                }
            }

            /** Refreshes scalar locals that were passed by managed reference to one invocation. */
            void emit_invocation_scalar_reference_results(vmir2::invocation_args const& args)
            {
                std::set< vmir2::local_index > refreshed_targets;
                bool refresh_all_targets = false;
                auto refresh_argument = [&](vmir2::local_index argument) -> void
                {
                    bool const is_alias = m_reference_aliases.at(local_slot(argument)).has_value();
                    vmir2::local_index const target = is_alias ? resolved_reference(argument) : argument;
                    type_symbol const target_type = unwrapped_type(m_routine.local_types.at(local_slot(target)).type);
                    if (m_input.struct_definitions.contains(target_type) || target_type.type_is< array_type >() || target_type.type_is< storage >() || is_semantic_fusion_type(m_input, target_type))
                    {
                        refresh_all_targets = true;
                        return;
                    }
                    if (!is_alias)
                    {
                        return;
                    }
                    if (!m_scalar_reference_owner_slots.at(local_slot(target)).has_value() || !refreshed_targets.insert(target).second)
                    {
                        return;
                    }
                };
                for (vmir2::local_index const argument : args.positional)
                {
                    refresh_argument(argument);
                }
                for (std::pair< std::string const, vmir2::local_index > const& argument : args.named)
                {
                    if (argument.first != "RETURN")
                    {
                        refresh_argument(argument.second);
                    }
                }
                if (refresh_all_targets)
                {
                    emit_scalar_reference_results();
                    return;
                }
                for (vmir2::local_index const target : refreshed_targets)
                {
                    emit_scalar_reference_result(target);
                }
            }

            /** Resolves and validates the managed allocation owning a Quxlang reference. */
            void emit_managed_reference_owner(vmir2::local_index reference)
            {
                m_code.aload(jvm_slot(reference)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").astore(m_reference_owner_slot);
                if (m_input.options.mode == backend_cortado_mode::address_sanitizer)
                {
                    label const valid = m_code.new_label();
                    m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "deallocated", "Z").branch< opcode::ifeq >(valid);
                    emit_runtime_exception(m_code, "Quxlang use after deallocation");
                    m_code.bind(valid);
                }
            }

            /** Loads and validates the owner from a managed reference in a JVM local slot. */
            void emit_managed_reference_owner_from_jvm_slot(local_variable_index reference_slot)
            {
                m_code.aload(reference_slot).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").astore(m_reference_owner_slot);
                if (m_input.options.mode == backend_cortado_mode::address_sanitizer)
                {
                    label const valid = m_code.new_label();
                    m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "deallocated", "Z").branch< opcode::ifeq >(valid);
                    emit_runtime_exception(m_code, "Quxlang use after deallocation");
                    m_code.bind(valid);
                }
            }

            /** Loads and range-checks a managed reference's logical JVM array index. */
            void emit_managed_reference_index(vmir2::local_index reference)
            {
                m_code.aload(jvm_slot(reference)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").invokestatic("java/lang/Math", "toIntExact", "(J)I");
            }

            /** Emits the checked JVM array index from a managed reference in a JVM local slot. */
            void emit_managed_reference_index_from_jvm_slot(local_variable_index reference_slot)
            {
                m_code.aload(reference_slot).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").invokestatic("java/lang/Math", "toIntExact", "(J)I");
            }

            /** Boxes a VMIR local for storage in a managed JVM object cell. */
            void emit_boxed_value(vmir2::local_index value)
            {
                jvm_value_kind const kind = kind_of(value);
                emit_load(m_code, kind, jvm_slot(value));
                switch (kind)
                {
                case jvm_value_kind::integer:
                    m_code.invokestatic("java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;");
                    return;
                case jvm_value_kind::long_:
                    m_code.invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;");
                    return;
                case jvm_value_kind::big_integer:
                    return;
                case jvm_value_kind::float_:
                    m_code.invokestatic("java/lang/Float", "valueOf", "(F)Ljava/lang/Float;");
                    return;
                case jvm_value_kind::double_:
                    m_code.invokestatic("java/lang/Double", "valueOf", "(D)Ljava/lang/Double;");
                    return;
                case jvm_value_kind::reference:
                    return;
                case jvm_value_kind::void_:
                    break;
                }
                throw compiler_bug("Quxlang's Cortado backend cannot box a void value");
            }

            /** Unboxes an Object operand to the requested JVM carrier category. */
            void emit_unboxed_value(jvm_value_kind kind)
            {
                switch (kind)
                {
                case jvm_value_kind::integer:
                    m_code.checkcast("java/lang/Integer").invokevirtual("java/lang/Integer", "intValue", "()I");
                    return;
                case jvm_value_kind::long_:
                    m_code.checkcast("java/lang/Long").invokevirtual("java/lang/Long", "longValue", "()J");
                    return;
                case jvm_value_kind::big_integer:
                    m_code.checkcast("java/math/BigInteger");
                    return;
                case jvm_value_kind::float_:
                    m_code.checkcast("java/lang/Float").invokevirtual("java/lang/Float", "floatValue", "()F");
                    return;
                case jvm_value_kind::double_:
                    m_code.checkcast("java/lang/Double").invokevirtual("java/lang/Double", "doubleValue", "()D");
                    return;
                case jvm_value_kind::reference:
                    return;
                case jvm_value_kind::void_:
                    break;
                }
                throw compiler_bug("Quxlang's Cortado backend cannot unbox a void value");
            }

            /** Boxes a JVM operand-stack value of the selected carrier kind. */
            void emit_boxed_stack_value(jvm_value_kind kind)
            {
                switch (kind)
                {
                case jvm_value_kind::integer:
                    m_code.invokestatic("java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;");
                    return;
                case jvm_value_kind::long_:
                    m_code.invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;");
                    return;
                case jvm_value_kind::big_integer:
                    return;
                case jvm_value_kind::float_:
                    m_code.invokestatic("java/lang/Float", "valueOf", "(F)Ljava/lang/Float;");
                    return;
                case jvm_value_kind::double_:
                    m_code.invokestatic("java/lang/Double", "valueOf", "(D)Ljava/lang/Double;");
                    return;
                case jvm_value_kind::reference:
                    return;
                case jvm_value_kind::void_:
                    break;
                }
                throw compiler_bug("Quxlang's Cortado backend cannot box a void operand-stack value");
            }

            /** Loads and unboxes a value through a managed reference. */
            void emit_unboxed_managed_reference_value(vmir2::local_index reference, jvm_value_kind kind)
            {
                emit_managed_reference_owner(reference);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(reference);
                m_code.append< opcode::aaload >();
                emit_unboxed_value(kind);
            }

            /** Stores a VMIR value through a managed Quxlang reference. */
            void store_managed_reference(vmir2::local_index reference, vmir2::local_index value)
            {
                emit_managed_reference_owner(reference);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(reference);
                emit_boxed_value(value);
                m_code.append< opcode::aastore >().aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                emit_managed_reference_index(reference);
                m_code.append< opcode::iconst_1 >().append< opcode::bastore >();
            }

            /** Loads the boxed value addressed by a managed reference into a JVM local slot. */
            void load_boxed_managed_reference_value(vmir2::local_index reference, local_variable_index destination)
            {
                emit_managed_reference_owner(reference);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(reference);
                m_code.append< opcode::aaload >().astore(destination);
            }

            /** Stores a boxed JVM local value through a managed reference. */
            void store_boxed_managed_reference_value(vmir2::local_index reference, local_variable_index source)
            {
                emit_managed_reference_owner(reference);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(reference);
                m_code.aload(source).append< opcode::aastore >();
            }

            /** Exchanges the initialized values addressed by two Quxlang managed references. */
            void emit_swap(vmir2::swap const& instruction)
            {
                if (!m_swap_a_value_slot.has_value() || !m_swap_b_value_slot.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend SWAP has no assigned JVM scratch slots");
                }
                load_boxed_managed_reference_value(instruction.a, *m_swap_a_value_slot);
                load_boxed_managed_reference_value(instruction.b, *m_swap_b_value_slot);
                store_boxed_managed_reference_value(instruction.a, *m_swap_b_value_slot);
                store_boxed_managed_reference_value(instruction.b, *m_swap_a_value_slot);
            }

            /** Returns the fixed semantic byte width used by GET_BYTE and SET_BYTE. */
            auto semantic_value_byte_count(type_symbol const& type) const -> std::uint64_t
            {
                jvm_value_kind const kind = value_kind(m_input, type);
                if (kind == jvm_value_kind::float_)
                {
                    return 4;
                }
                if (kind == jvm_value_kind::double_)
                {
                    return 8;
                }
                if (kind == jvm_value_kind::integer || kind == jvm_value_kind::long_ || kind == jvm_value_kind::big_integer)
                {
                    return (integer_bit_width_for_type(type) + 7) / 8;
                }
                throw rpnx::unimplemented();
            }

            /** Extracts one byte from the canonical little-endian representation of a numeric value. */
            void emit_get_value_byte(vmir2::get_value_byte const& instruction)
            {
                type_symbol const value_type = referenced_value_type(instruction.source_reference);
                if (instruction.offset >= semantic_value_byte_count(value_type))
                {
                    throw compiler_bug("Quxlang's Cortado backend GET_BYTE offset is outside the numeric value");
                }
                jvm_value_kind const kind = value_kind(m_input, value_type);
                emit_unboxed_managed_reference_value(instruction.source_reference, kind);
                if (kind == jvm_value_kind::float_)
                {
                    m_code.invokestatic("java/lang/Float", "floatToRawIntBits", "(F)I");
                }
                else if (kind == jvm_value_kind::double_)
                {
                    m_code.invokestatic("java/lang/Double", "doubleToRawLongBits", "(D)J");
                }

                std::uint32_t const shift = static_cast< std::uint32_t >(instruction.offset * 8);
                if (kind == jvm_value_kind::integer || kind == jvm_value_kind::float_)
                {
                    if (shift != 0)
                    {
                        emit_int_constant(m_code, shift);
                        m_code.append< opcode::iushr >();
                    }
                }
                else if (kind == jvm_value_kind::long_ || kind == jvm_value_kind::double_)
                {
                    if (shift != 0)
                    {
                        emit_int_constant(m_code, shift);
                        m_code.append< opcode::lushr >();
                    }
                    m_code.append< opcode::l2i >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    if (shift != 0)
                    {
                        emit_int_constant(m_code, shift);
                        m_code.invokevirtual("java/math/BigInteger", "shiftRight", "(I)Ljava/math/BigInteger;");
                    }
                    m_code.invokevirtual("java/math/BigInteger", "intValue", "()I");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend GET_BYTE accepted an unsupported JVM value kind");
                }
                emit_int_constant(m_code, 0xff);
                m_code.append< opcode::iand >().istore(jvm_slot(instruction.result));
            }

            /** Replaces one byte in the canonical little-endian representation of a numeric value. */
            void emit_set_value_byte(vmir2::set_value_byte const& instruction)
            {
                type_symbol const value_type = referenced_value_type(instruction.target_reference);
                if (instruction.offset >= semantic_value_byte_count(value_type))
                {
                    throw compiler_bug("Quxlang's Cortado backend SET_BYTE offset is outside the numeric value");
                }
                jvm_value_kind const kind = value_kind(m_input, value_type);
                emit_managed_reference_owner(instruction.target_reference);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(instruction.target_reference);
                emit_unboxed_managed_reference_value(instruction.target_reference, kind);
                if (kind == jvm_value_kind::float_)
                {
                    m_code.invokestatic("java/lang/Float", "floatToRawIntBits", "(F)I");
                }
                else if (kind == jvm_value_kind::double_)
                {
                    m_code.invokestatic("java/lang/Double", "doubleToRawLongBits", "(D)J");
                }

                std::uint32_t const shift = static_cast< std::uint32_t >(instruction.offset * 8);
                if (kind == jvm_value_kind::integer || kind == jvm_value_kind::float_)
                {
                    emit_int_constant(m_code, ~(std::uint32_t(0xff) << shift));
                    m_code.append< opcode::iand >().iload(jvm_slot(instruction.value));
                    emit_int_constant(m_code, 0xff);
                    m_code.append< opcode::iand >();
                    if (shift != 0)
                    {
                        emit_int_constant(m_code, shift);
                        m_code.append< opcode::ishl >();
                    }
                    m_code.append< opcode::ior >();
                    if (kind == jvm_value_kind::float_)
                    {
                        m_code.invokestatic("java/lang/Float", "intBitsToFloat", "(I)F");
                    }
                    else
                    {
                        emit_integer_canonicalization(value_type, kind);
                    }
                }
                else if (kind == jvm_value_kind::long_ || kind == jvm_value_kind::double_)
                {
                    emit_long_constant(m_code, ~(std::uint64_t(0xff) << shift));
                    m_code.append< opcode::land >().iload(jvm_slot(instruction.value)).append< opcode::i2l >();
                    emit_long_constant(m_code, 0xff);
                    m_code.append< opcode::land >();
                    if (shift != 0)
                    {
                        emit_int_constant(m_code, shift);
                        m_code.append< opcode::lshl >();
                    }
                    m_code.append< opcode::lor >();
                    if (kind == jvm_value_kind::double_)
                    {
                        m_code.invokestatic("java/lang/Double", "longBitsToDouble", "(J)D");
                    }
                    else
                    {
                        emit_integer_canonicalization(value_type, kind);
                    }
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    emit_long_constant(m_code, 0xff);
                    m_code.invokestatic("java/math/BigInteger", "valueOf", "(J)Ljava/math/BigInteger;");
                    if (shift != 0)
                    {
                        emit_int_constant(m_code, shift);
                        m_code.invokevirtual("java/math/BigInteger", "shiftLeft", "(I)Ljava/math/BigInteger;");
                    }
                    m_code.invokevirtual("java/math/BigInteger", "not", "()Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "and", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                    m_code.iload(jvm_slot(instruction.value));
                    emit_int_constant(m_code, 0xff);
                    m_code.append< opcode::iand >().append< opcode::i2l >().invokestatic("java/math/BigInteger", "valueOf", "(J)Ljava/math/BigInteger;");
                    if (shift != 0)
                    {
                        emit_int_constant(m_code, shift);
                        m_code.invokevirtual("java/math/BigInteger", "shiftLeft", "(I)Ljava/math/BigInteger;");
                    }
                    m_code.invokevirtual("java/math/BigInteger", "or", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                    emit_integer_canonicalization(value_type, kind);
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend SET_BYTE accepted an unsupported JVM value kind");
                }
                emit_boxed_stack_value(kind);
                m_code.append< opcode::aastore >();
            }

            /** Stores a VMIR value through a managed reference held in a JVM local slot. */
            void store_managed_reference_from_jvm_slot(local_variable_index reference_slot, vmir2::local_index value)
            {
                emit_managed_reference_owner_from_jvm_slot(reference_slot);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index_from_jvm_slot(reference_slot);
                emit_boxed_value(value);
                m_code.append< opcode::aastore >().aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                emit_managed_reference_index_from_jvm_slot(reference_slot);
                m_code.append< opcode::iconst_1 >().append< opcode::bastore >();
            }

            /** Publishes array element values and advances their initializer when they transition to alive. */
            void emit_completed_array_elements(vmir2::state_map const& previous_state, vmir2::state_map const& current_state)
            {
                for (std::pair< vmir2::local_index const, vmir2::slot_state > const& entry : current_state)
                {
                    vmir2::local_index const target = entry.first;
                    vmir2::slot_state const& current = entry.second;
                    if (!current.array_delegate_of_initializer.has_value() || !current.alive())
                    {
                        continue;
                    }
                    vmir2::state_map::const_iterator const previous = previous_state.find(target);
                    if (previous == previous_state.end() || previous->second.alive())
                    {
                        continue;
                    }
                    std::optional< local_variable_index > const& reference_slot = m_array_element_reference_slots.at(local_slot(target));
                    if (!reference_slot.has_value())
                    {
                        throw compiler_bug("Quxlang's Cortado backend array delegate has no managed-reference JVM local slot");
                    }
                    store_managed_reference_from_jvm_slot(*reference_slot, target);
                    vmir2::local_index const initializer = *current.array_delegate_of_initializer;
                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(jvm_slot(initializer)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").aload(jvm_slot(initializer)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").template append< opcode::lconst_1 >().template append< opcode::ladd >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(initializer));
                }
            }

            /** Publishes one initialized semantic field into managed aggregate storage. */
            void store_aggregate_field(vmir2::local_index aggregate, std::size_t field_index, vmir2::local_index value)
            {
                m_code.aload(jvm_slot(aggregate)).checkcast("quxlang/runtime/QuxlangObject").getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_int_constant(m_code, static_cast< std::uint32_t >(field_index));
                emit_boxed_value(value);
                m_code.append< opcode::aastore >().aload(jvm_slot(aggregate)).checkcast("quxlang/runtime/QuxlangObject").getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                emit_int_constant(m_code, static_cast< std::uint32_t >(field_index));
                m_code.append< opcode::iconst_1 >().append< opcode::bastore >();
            }

            /** Publishes aggregate fields when their VMIR initializer locals transition to alive. */
            void emit_completed_struct_fields(vmir2::state_map const& previous_state, vmir2::state_map const& current_state)
            {
                for (pending_struct_initializer& initializer : m_pending_struct_initializers)
                {
                    std::map< std::string, vmir2::local_index >::iterator field = initializer.fields.begin();
                    while (field != initializer.fields.end())
                    {
                        vmir2::state_map::const_iterator const current = current_state.find(field->second);
                        if (current == current_state.end() || !current->second.alive())
                        {
                            ++field;
                            continue;
                        }
                        vmir2::state_map::const_iterator const previous = previous_state.find(field->second);
                        if (previous != previous_state.end() && previous->second.alive())
                        {
                            ++field;
                            continue;
                        }
                        store_aggregate_field(initializer.target, aggregate_field_index(initializer.target, field->first), field->second);
                        field = initializer.fields.erase(field);
                    }
                }
            }

            /** Loads an initialized VMIR value through a managed Quxlang reference. */
            void load_managed_reference(vmir2::local_index reference, vmir2::local_index value)
            {
                emit_managed_reference_owner(reference);
                label const initialized = m_code.new_label();
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                emit_managed_reference_index(reference);
                m_code.append< opcode::baload >().branch< opcode::ifne >(initialized);
                emit_runtime_exception(m_code, "Quxlang access to uninitialized object storage");
                m_code.bind(initialized).aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(reference);
                m_code.append< opcode::aaload >();
                jvm_value_kind const kind = kind_of(value);
                emit_unboxed_value(kind);
                emit_store(m_code, kind, jvm_slot(value));
                canonicalize_integer(value);
            }

            /** Canonicalizes the integer value currently on the JVM operand stack for a VMIR local type. */
            void emit_integer_canonicalization(type_symbol type, jvm_value_kind kind)
            {
                type = unwrapped_type(std::move(type));
                std::uint32_t bit_width = 0;
                bool has_sign = false;
                if (type.type_is< int_type >())
                {
                    int_type const format = type.as< int_type >();
                    bit_width = static_cast< std::uint32_t >(format.bits);
                    has_sign = format.has_sign;
                }
                else if (type.type_is< byte_type >())
                {
                    bit_width = 8;
                }
                else if (type.type_is< bool_type >())
                {
                    bit_width = 1;
                }
                else if (type.type_is< size_type >())
                {
                    bit_width = 64;
                }
                else if (type.type_is< builtin_symbol >() && type.as< builtin_symbol >().name == "ORDER")
                {
                    bit_width = 8;
                    has_sign = true;
                }
                else
                {
                    std::map< type_symbol, enum_info >::const_iterator const enum_iter = m_input.enum_definitions.find(type);
                    if (enum_iter != m_input.enum_definitions.end())
                    {
                        bit_width = static_cast< std::uint32_t >(enum_iter->second.format.bit_width);
                        has_sign = enum_iter->second.format.encoding == enum_integer_encoding::signed_twos_complement_le;
                    }
                    else
                    {
                        std::map< type_symbol, flagset_info >::const_iterator const flagset_iter = m_input.flagset_definitions.find(type);
                        if (flagset_iter == m_input.flagset_definitions.end())
                        {
                            return;
                        }
                        bit_width = static_cast< std::uint32_t >(flagset_iter->second.bits);
                    }
                }
                if (bit_width == 32 || bit_width == 64)
                {
                    return;
                }
                if (kind == jvm_value_kind::big_integer)
                {
                    emit_big_integer_canonicalization(m_code, bit_width, has_sign);
                }
                else if (kind == jvm_value_kind::integer)
                {
                    if (has_sign)
                    {
                        m_code.bipush(static_cast< std::int8_t >(32 - bit_width)).append< opcode::ishl >();
                        m_code.bipush(static_cast< std::int8_t >(32 - bit_width)).append< opcode::ishr >();
                    }
                    else
                    {
                        emit_int_constant(m_code, (std::uint32_t(1) << bit_width) - 1U);
                        m_code.append< opcode::iand >();
                    }
                }
                else
                {
                    if (has_sign)
                    {
                        m_code.bipush(static_cast< std::int8_t >(64 - bit_width)).append< opcode::lshl >();
                        m_code.bipush(static_cast< std::int8_t >(64 - bit_width)).append< opcode::lshr >();
                    }
                    else
                    {
                        emit_long_constant(m_code, (std::uint64_t(1) << bit_width) - 1U);
                        m_code.append< opcode::land >();
                    }
                }
            }

            /** Canonicalizes the integer value currently on the JVM operand stack for a VMIR local type. */
            void emit_integer_canonicalization(vmir2::local_index result)
            {
                emit_integer_canonicalization(m_routine.local_types.at(local_slot(result)).type, kind_of(result));
            }

            /** Canonicalizes the integer value stored in a VMIR local. */
            void canonicalize_integer(vmir2::local_index result)
            {
                jvm_value_kind const kind = kind_of(result);
                if (kind != jvm_value_kind::integer && kind != jvm_value_kind::long_ && kind != jvm_value_kind::big_integer)
                {
                    return;
                }
                emit_load(m_code, kind, jvm_slot(result));
                emit_integer_canonicalization(result);
                emit_store(m_code, kind, jvm_slot(result));
            }

            /** Returns whether a VMIR integer-represented local uses unsigned semantics. */
            auto integer_is_unsigned(vmir2::local_index index) const -> bool
            {
                type_symbol const type = unwrapped_type(m_routine.local_types.at(local_slot(index)).type);
                if (type.type_is< int_type >())
                {
                    return !type.as< int_type >().has_sign;
                }
                if (type.type_is< size_type >() || type.type_is< byte_type >() || type.type_is< bool_type >())
                {
                    return true;
                }
                std::map< type_symbol, enum_info >::const_iterator const enum_iter = m_input.enum_definitions.find(type);
                if (enum_iter != m_input.enum_definitions.end())
                {
                    return enum_iter->second.format.encoding == enum_integer_encoding::unsigned_le;
                }
                return m_input.flagset_definitions.contains(type);
            }

            /** Emits an integer local as a JVM long without losing unsigned 32-bit values. */
            void emit_integer_as_long(vmir2::local_index index)
            {
                jvm_value_kind const kind = kind_of(index);
                if (kind == jvm_value_kind::long_)
                {
                    m_code.lload(jvm_slot(index));
                    return;
                }
                if (kind != jvm_value_kind::integer)
                {
                    throw compiler_bug("Quxlang's Cortado backend managed-reference indexing received a non-integer offset");
                }
                m_code.iload(jvm_slot(index)).append< opcode::i2l >();
                if (integer_is_unsigned(index))
                {
                    emit_long_constant(m_code, std::numeric_limits< std::uint32_t >::max());
                    m_code.append< opcode::land >();
                }
            }

            /** Emits an integer shift amount as the JVM int required by shift opcodes. */
            void emit_shift_amount_as_int(vmir2::local_index amount)
            {
                jvm_value_kind const kind = kind_of(amount);
                if (kind == jvm_value_kind::integer)
                {
                    m_code.iload(jvm_slot(amount));
                    return;
                }
                if (kind == jvm_value_kind::long_)
                {
                    m_code.lload(jvm_slot(amount)).append< opcode::l2i >();
                    return;
                }
                if (kind == jvm_value_kind::big_integer)
                {
                    m_code.aload(jvm_slot(amount)).invokevirtual("java/math/BigInteger", "intValueExact", "()I");
                    return;
                }
                throw compiler_bug("Quxlang's Cortado backend bitwise shift received a non-integer amount");
            }

            /** Returns the logical Quxlang bit width represented by an integer type. */
            auto integer_bit_width_for_type(type_symbol type) const -> std::uint32_t
            {
                type = unwrapped_type(std::move(type));
                if (type.type_is< int_type >())
                {
                    std::size_t const bits = type.as< int_type >().bits;
                    if (bits > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
                    {
                        throw lowering_compilation_error("Quxlang's Cortado backend cannot lower an integer width that exceeds java.math.BigInteger's index limit");
                    }
                    return static_cast< std::uint32_t >(bits);
                }
                if (type.type_is< byte_type >())
                {
                    return 8;
                }
                if (type.type_is< bool_type >())
                {
                    return 1;
                }
                if (type.type_is< size_type >())
                {
                    return 64;
                }
                std::map< type_symbol, enum_info >::const_iterator const enum_iter = m_input.enum_definitions.find(type);
                if (enum_iter != m_input.enum_definitions.end())
                {
                    std::size_t const bits = enum_iter->second.format.bit_width;
                    if (bits > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
                    {
                        throw lowering_compilation_error("Quxlang's Cortado backend cannot lower an enum width that exceeds java.math.BigInteger's index limit");
                    }
                    return static_cast< std::uint32_t >(bits);
                }
                std::map< type_symbol, flagset_info >::const_iterator const flagset_iter = m_input.flagset_definitions.find(type);
                if (flagset_iter != m_input.flagset_definitions.end())
                {
                    std::size_t const bits = flagset_iter->second.bits;
                    if (bits > static_cast< std::size_t >(std::numeric_limits< std::int32_t >::max()))
                    {
                        throw lowering_compilation_error("Quxlang's Cortado backend cannot lower a flagset width that exceeds java.math.BigInteger's index limit");
                    }
                    return static_cast< std::uint32_t >(bits);
                }
                throw compiler_bug("Quxlang's Cortado backend bitwise operation received a non-integer value");
            }

            /** Returns the logical Quxlang bit width represented by an integer JVM local. */
            auto integer_bit_width(vmir2::local_index index) const -> std::uint32_t
            {
                return integer_bit_width_for_type(m_routine.local_types.at(local_slot(index)).type);
            }

            /** Selects the BigInteger method corresponding to a primitive JVM integer opcode. */
            template < opcode IntegerOperation >
            static constexpr auto big_integer_binary_method_name() -> char const*
            {
                if constexpr (IntegerOperation == opcode::iadd)
                {
                    return "add";
                }
                else if constexpr (IntegerOperation == opcode::isub)
                {
                    return "subtract";
                }
                else if constexpr (IntegerOperation == opcode::imul)
                {
                    return "multiply";
                }
                else if constexpr (IntegerOperation == opcode::iand)
                {
                    return "and";
                }
                else if constexpr (IntegerOperation == opcode::ior)
                {
                    return "or";
                }
                else if constexpr (IntegerOperation == opcode::ixor)
                {
                    return "xor";
                }
                else
                {
                    static_assert(IntegerOperation != IntegerOperation, "Unsupported BigInteger binary operation");
                }
            }

            /** Converts an integer local to an exact mathematical BigInteger value. */
            void emit_integer_as_big_integer(vmir2::local_index index)
            {
                jvm_value_kind const kind = kind_of(index);
                if (kind == jvm_value_kind::big_integer)
                {
                    m_code.aload(jvm_slot(index));
                    return;
                }
                if (kind == jvm_value_kind::integer)
                {
                    m_code.iload(jvm_slot(index)).append< opcode::i2l >();
                    if (integer_is_unsigned(index))
                    {
                        emit_long_constant(m_code, std::numeric_limits< std::uint32_t >::max());
                        m_code.append< opcode::land >();
                    }
                    m_code.invokestatic("java/math/BigInteger", "valueOf", "(J)Ljava/math/BigInteger;");
                    return;
                }
                if (kind == jvm_value_kind::long_)
                {
                    if (integer_is_unsigned(index))
                    {
                        m_code.new_("java/math/BigInteger").append< opcode::dup >().lload(jvm_slot(index)).invokestatic("java/lang/Long", "toUnsignedString", "(J)Ljava/lang/String;").invokespecial("java/math/BigInteger", "<init>", "(Ljava/lang/String;)V");
                    }
                    else
                    {
                        m_code.lload(jvm_slot(index)).invokestatic("java/math/BigInteger", "valueOf", "(J)Ljava/math/BigInteger;");
                    }
                    return;
                }
                throw compiler_bug("Quxlang's Cortado backend cannot convert a non-integer carrier to BigInteger");
            }

            /** Restricts a BigInteger operand-stack value to a fixed-width unsigned bit pattern. */
            void emit_unsigned_big_integer_pattern(std::uint32_t bit_width)
            {
                m_code.getstatic("java/math/BigInteger", "ONE", "Ljava/math/BigInteger;");
                emit_int_constant(m_code, bit_width);
                m_code.invokevirtual("java/math/BigInteger", "shiftLeft", "(I)Ljava/math/BigInteger;").getstatic("java/math/BigInteger", "ONE", "Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "subtract", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "and", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
            }

            /** Inverts the integer value currently on the JVM operand stack. */
            void emit_integer_inverse(jvm_value_kind kind)
            {
                if (kind == jvm_value_kind::integer)
                {
                    m_code.append< opcode::iconst_m1 >().append< opcode::ixor >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    emit_long_constant(m_code, std::numeric_limits< std::uint64_t >::max());
                    m_code.append< opcode::lxor >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", "not", "()Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend bitwise inverse received a non-integer value");
                }
            }

            /** Emits a carrier-selected integer binary operation into a result local. */
            template < opcode IntegerOperation, opcode LongOperation >
            void emit_integer_binary(vmir2::local_index a, vmir2::local_index b, vmir2::local_index result)
            {
                jvm_value_kind const kind = kind_of(result);
                emit_load(m_code, kind, jvm_slot(a));
                emit_load(m_code, kind, jvm_slot(b));
                if (kind == jvm_value_kind::integer)
                {
                    m_code.append< IntegerOperation >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.append< LongOperation >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", big_integer_binary_method_name< IntegerOperation >(), "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend integer operation received a non-integer value");
                }
                emit_store(m_code, kind, jvm_slot(result));
                canonicalize_integer(result);
            }

            /** Emits a nonatomic mutating integer operation on a direct JVM local alias. */
            template < opcode IntegerOperation, opcode LongOperation >
            void emit_local_mutating_integer_binary(vmir2::local_index target_reference, vmir2::local_index value, std::optional< vmir2::local_index > old_value, bool invert = false)
            {
                vmir2::local_index const target = resolved_reference(target_reference);
                if (old_value.has_value())
                {
                    copy_local(target, *old_value);
                }
                jvm_value_kind const kind = kind_of(target);
                emit_load(m_code, kind, jvm_slot(target));
                emit_load(m_code, kind, jvm_slot(value));
                if (kind == jvm_value_kind::integer)
                {
                    m_code.append< IntegerOperation >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.append< LongOperation >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", big_integer_binary_method_name< IntegerOperation >(), "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating integer operation received a non-integer value");
                }
                if (invert)
                {
                    emit_integer_inverse(kind);
                }
                emit_integer_canonicalization(m_routine.local_types.at(local_slot(target)).type, kind);
                emit_store(m_code, kind, jvm_slot(target));
            }

            /** Emits a nonatomic mutating integer division on a direct JVM local alias. */
            void emit_local_mutating_integer_division(vmir2::local_index target_reference, vmir2::local_index value, std::optional< vmir2::local_index > old_value, bool remainder)
            {
                vmir2::local_index const target = resolved_reference(target_reference);
                if (old_value.has_value())
                {
                    copy_local(target, *old_value);
                }
                jvm_value_kind const kind = kind_of(target);
                emit_load(m_code, kind, jvm_slot(target));
                emit_load(m_code, kind, jvm_slot(value));
                if (kind == jvm_value_kind::integer)
                {
                    if (integer_is_unsigned(target))
                    {
                        m_code.invokestatic("java/lang/Integer", remainder ? "remainderUnsigned" : "divideUnsigned", "(II)I");
                    }
                    else if (remainder)
                    {
                        m_code.append< opcode::irem >();
                    }
                    else
                    {
                        m_code.append< opcode::idiv >();
                    }
                }
                else if (kind == jvm_value_kind::long_)
                {
                    if (integer_is_unsigned(target))
                    {
                        m_code.invokestatic("java/lang/Long", remainder ? "remainderUnsigned" : "divideUnsigned", "(JJ)J");
                    }
                    else if (remainder)
                    {
                        m_code.append< opcode::lrem >();
                    }
                    else
                    {
                        m_code.append< opcode::ldiv >();
                    }
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", remainder ? "remainder" : "divide", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating integer division received a non-integer value");
                }
                emit_integer_canonicalization(m_routine.local_types.at(local_slot(target)).type, kind);
                emit_store(m_code, kind, jvm_slot(target));
            }

            /** Emits a nonatomic mutating floating-point operation on a direct JVM local alias. */
            template < opcode FloatOperation, opcode DoubleOperation >
            void emit_local_mutating_float_binary(vmir2::local_index target_reference, vmir2::local_index value)
            {
                vmir2::local_index const target = resolved_reference(target_reference);
                jvm_value_kind const kind = kind_of(target);
                emit_load(m_code, kind, jvm_slot(target));
                emit_load(m_code, kind, jvm_slot(value));
                if (kind == jvm_value_kind::float_)
                {
                    m_code.append< FloatOperation >();
                }
                else if (kind == jvm_value_kind::double_)
                {
                    m_code.append< DoubleOperation >();
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating floating-point operation received a non-floating value");
                }
                emit_store(m_code, kind, jvm_slot(target));
            }

            /** Emits a nonatomic mutating implication on a direct JVM local alias. */
            void emit_local_mutating_bitwise_implication(vmir2::local_index target_reference, vmir2::local_index value, std::optional< vmir2::local_index > old_value, bool inverse_left)
            {
                vmir2::local_index const target = resolved_reference(target_reference);
                if (old_value.has_value())
                {
                    copy_local(target, *old_value);
                }
                jvm_value_kind const kind = kind_of(target);
                emit_load(m_code, kind, jvm_slot(target));
                if (inverse_left)
                {
                    emit_integer_inverse(kind);
                }
                emit_load(m_code, kind, jvm_slot(value));
                if (!inverse_left)
                {
                    emit_integer_inverse(kind);
                }
                if (kind == jvm_value_kind::integer)
                {
                    m_code.append< opcode::ior >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.append< opcode::lor >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", "or", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating bitwise implication received a non-integer value");
                }
                emit_integer_canonicalization(m_routine.local_types.at(local_slot(target)).type, kind);
                emit_store(m_code, kind, jvm_slot(target));
            }

            /** Emits a nonatomic mutating integer operation through managed storage. */
            template < opcode IntegerOperation, opcode LongOperation >
            void emit_mutating_integer_binary(vmir2::local_index target, vmir2::local_index value, std::optional< vmir2::local_index > old_value, bool invert = false)
            {
                if (old_value.has_value())
                {
                    load_managed_reference(target, *old_value);
                }
                jvm_value_kind const kind = value_kind(m_input, referenced_value_type(target));
                emit_managed_reference_owner(target);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(target);
                emit_unboxed_managed_reference_value(target, kind);
                emit_load(m_code, kind, jvm_slot(value));
                if (kind == jvm_value_kind::integer)
                {
                    m_code.append< IntegerOperation >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.append< LongOperation >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", big_integer_binary_method_name< IntegerOperation >(), "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating integer operation received a non-integer value");
                }
                if (invert)
                {
                    emit_integer_inverse(kind);
                }
                emit_integer_canonicalization(referenced_value_type(target), kind);
                emit_boxed_stack_value(kind);
                m_code.append< opcode::aastore >();
            }

            /** Emits a nonatomic mutating integer division or remainder operation. */
            void emit_mutating_integer_division(vmir2::local_index target, vmir2::local_index value, std::optional< vmir2::local_index > old_value, bool remainder)
            {
                if (old_value.has_value())
                {
                    load_managed_reference(target, *old_value);
                }
                jvm_value_kind const kind = value_kind(m_input, referenced_value_type(target));
                emit_managed_reference_owner(target);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(target);
                emit_unboxed_managed_reference_value(target, kind);
                emit_load(m_code, kind, jvm_slot(value));
                if (kind == jvm_value_kind::integer)
                {
                    if (integer_is_unsigned(value))
                    {
                        m_code.invokestatic("java/lang/Integer", remainder ? "remainderUnsigned" : "divideUnsigned", "(II)I");
                    }
                    else if (remainder)
                    {
                        m_code.append< opcode::irem >();
                    }
                    else
                    {
                        m_code.append< opcode::idiv >();
                    }
                }
                else if (kind == jvm_value_kind::long_)
                {
                    if (integer_is_unsigned(value))
                    {
                        m_code.invokestatic("java/lang/Long", remainder ? "remainderUnsigned" : "divideUnsigned", "(JJ)J");
                    }
                    else if (remainder)
                    {
                        m_code.append< opcode::lrem >();
                    }
                    else
                    {
                        m_code.append< opcode::ldiv >();
                    }
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", remainder ? "remainder" : "divide", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating integer division received a non-integer value");
                }
                emit_integer_canonicalization(referenced_value_type(target), kind);
                emit_boxed_stack_value(kind);
                m_code.append< opcode::aastore >();
            }

            /** Emits a nonatomic mutating floating-point operation through managed storage. */
            template < opcode FloatOperation, opcode DoubleOperation >
            void emit_mutating_float_binary(vmir2::local_index target, vmir2::local_index value)
            {
                jvm_value_kind const kind = value_kind(m_input, referenced_value_type(target));
                emit_managed_reference_owner(target);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(target);
                emit_unboxed_managed_reference_value(target, kind);
                emit_load(m_code, kind, jvm_slot(value));
                if (kind == jvm_value_kind::float_)
                {
                    m_code.append< FloatOperation >();
                }
                else if (kind == jvm_value_kind::double_)
                {
                    m_code.append< DoubleOperation >();
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating floating-point operation received a non-floating value");
                }
                emit_boxed_stack_value(kind);
                m_code.append< opcode::aastore >();
            }

            /** Emits a nonatomic mutating logical shift through managed storage. */
            void emit_mutating_bitwise_shift(vmir2::local_index target, vmir2::local_index amount, bool upward)
            {
                type_symbol const value_type = referenced_value_type(target);
                jvm_value_kind const kind = value_kind(m_input, value_type);
                emit_managed_reference_owner(target);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(target);
                emit_unboxed_managed_reference_value(target, kind);
                if (kind == jvm_value_kind::integer)
                {
                    emit_shift_amount_as_int(amount);
                    if (upward)
                    {
                        m_code.append< opcode::ishl >();
                    }
                    else
                    {
                        m_code.append< opcode::iushr >();
                    }
                }
                else if (kind == jvm_value_kind::long_)
                {
                    emit_shift_amount_as_int(amount);
                    if (upward)
                    {
                        m_code.append< opcode::lshl >();
                    }
                    else
                    {
                        m_code.append< opcode::lushr >();
                    }
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    if (!upward)
                    {
                        emit_unsigned_big_integer_pattern(integer_bit_width_for_type(value_type));
                    }
                    emit_shift_amount_as_int(amount);
                    m_code.invokevirtual("java/math/BigInteger", upward ? "shiftLeft" : "shiftRight", "(I)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating shift received a non-integer value");
                }
                emit_integer_canonicalization(value_type, kind);
                emit_boxed_stack_value(kind);
                m_code.append< opcode::aastore >();
            }

            /** Emits a nonatomic mutating bitwise implication through managed storage. */
            void emit_mutating_bitwise_implication(vmir2::local_index target, vmir2::local_index value, std::optional< vmir2::local_index > old_value, bool inverse_left)
            {
                if (old_value.has_value())
                {
                    load_managed_reference(target, *old_value);
                }
                type_symbol const value_type = referenced_value_type(target);
                jvm_value_kind const kind = value_kind(m_input, value_type);
                emit_managed_reference_owner(target);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(target);
                emit_unboxed_managed_reference_value(target, kind);
                if (inverse_left)
                {
                    emit_integer_inverse(kind);
                }
                emit_load(m_code, kind, jvm_slot(value));
                if (!inverse_left)
                {
                    emit_integer_inverse(kind);
                }
                if (kind == jvm_value_kind::integer)
                {
                    m_code.append< opcode::ior >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.append< opcode::lor >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", "or", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating bitwise implication received a non-integer value");
                }
                emit_integer_canonicalization(value_type, kind);
                emit_boxed_stack_value(kind);
                m_code.append< opcode::aastore >();
            }

            /** Emits a nonatomic width-aware rotate through managed storage. */
            void emit_mutating_bitwise_rotate(vmir2::local_index target, vmir2::local_index amount, bool upward)
            {
                type_symbol const value_type = referenced_value_type(target);
                jvm_value_kind const kind = value_kind(m_input, value_type);
                std::uint32_t const bits = integer_bit_width_for_type(value_type);
                emit_managed_reference_owner(target);
                m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                emit_managed_reference_index(target);
                if (kind == jvm_value_kind::integer && bits == 32)
                {
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_shift_amount_as_int(amount);
                    m_code.invokestatic("java/lang/Integer", upward ? "rotateLeft" : "rotateRight", "(II)I");
                }
                else if (kind == jvm_value_kind::long_ && bits == 64)
                {
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_shift_amount_as_int(amount);
                    m_code.invokestatic("java/lang/Long", upward ? "rotateLeft" : "rotateRight", "(JI)J");
                }
                else if (kind == jvm_value_kind::integer)
                {
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >();
                    if (upward)
                    {
                        m_code.append< opcode::ishl >();
                    }
                    else
                    {
                        m_code.append< opcode::iushr >();
                    }
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_int_constant(m_code, bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().append< opcode::isub >();
                    if (upward)
                    {
                        m_code.append< opcode::iushr >();
                    }
                    else
                    {
                        m_code.append< opcode::ishl >();
                    }
                    m_code.append< opcode::ior >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >();
                    if (upward)
                    {
                        m_code.append< opcode::lshl >();
                    }
                    else
                    {
                        m_code.append< opcode::lushr >();
                    }
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_int_constant(m_code, bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().append< opcode::isub >();
                    if (upward)
                    {
                        m_code.append< opcode::lushr >();
                    }
                    else
                    {
                        m_code.append< opcode::lshl >();
                    }
                    m_code.append< opcode::lor >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_unsigned_big_integer_pattern(bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().invokevirtual("java/math/BigInteger", upward ? "shiftLeft" : "shiftRight", "(I)Ljava/math/BigInteger;");
                    emit_unboxed_managed_reference_value(target, kind);
                    emit_unsigned_big_integer_pattern(bits);
                    emit_int_constant(m_code, bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().append< opcode::isub >().invokevirtual("java/math/BigInteger", upward ? "shiftRight" : "shiftLeft", "(I)Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "or", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend mutating rotate received a non-integer value");
                }
                emit_integer_canonicalization(value_type, kind);
                emit_boxed_stack_value(kind);
                m_code.append< opcode::aastore >();
            }

            /** Emits an integer binary operation followed by logical bitwise inversion. */
            template < opcode IntegerOperation, opcode LongOperation >
            void emit_inverted_integer_binary(vmir2::local_index a, vmir2::local_index b, vmir2::local_index result)
            {
                jvm_value_kind const kind = kind_of(result);
                emit_load(m_code, kind, jvm_slot(a));
                emit_load(m_code, kind, jvm_slot(b));
                if (kind == jvm_value_kind::integer)
                {
                    m_code.append< IntegerOperation >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.append< LongOperation >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", big_integer_binary_method_name< IntegerOperation >(), "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend bitwise operation received a non-integer value");
                }
                emit_integer_inverse(kind);
                emit_store(m_code, kind, jvm_slot(result));
                canonicalize_integer(result);
            }

            /** Emits a logical shift for an integer-represented Quxlang value. */
            void emit_bitwise_shift(vmir2::local_index value, vmir2::local_index amount, vmir2::local_index result, bool upward)
            {
                jvm_value_kind const kind = kind_of(result);
                emit_load(m_code, kind, jvm_slot(value));
                if (kind == jvm_value_kind::integer)
                {
                    emit_shift_amount_as_int(amount);
                    if (upward)
                    {
                        m_code.append< opcode::ishl >();
                    }
                    else
                    {
                        m_code.append< opcode::iushr >();
                    }
                }
                else if (kind == jvm_value_kind::long_)
                {
                    emit_shift_amount_as_int(amount);
                    if (upward)
                    {
                        m_code.append< opcode::lshl >();
                    }
                    else
                    {
                        m_code.append< opcode::lushr >();
                    }
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    if (!upward)
                    {
                        emit_unsigned_big_integer_pattern(integer_bit_width(value));
                    }
                    emit_shift_amount_as_int(amount);
                    m_code.invokevirtual("java/math/BigInteger", upward ? "shiftLeft" : "shiftRight", "(I)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend bitwise shift received a non-integer value");
                }
                emit_store(m_code, kind, jvm_slot(result));
                canonicalize_integer(result);
            }

            /** Emits a width-aware rotate for an integer-represented Quxlang value. */
            void emit_bitwise_rotate(vmir2::local_index value, vmir2::local_index amount, vmir2::local_index result, bool upward)
            {
                jvm_value_kind const kind = kind_of(result);
                std::uint32_t const bits = integer_bit_width(result);
                if (kind == jvm_value_kind::integer && bits == 32)
                {
                    m_code.iload(jvm_slot(value));
                    emit_shift_amount_as_int(amount);
                    m_code.invokestatic("java/lang/Integer", upward ? "rotateLeft" : "rotateRight", "(II)I");
                }
                else if (kind == jvm_value_kind::long_ && bits == 64)
                {
                    m_code.lload(jvm_slot(value));
                    emit_shift_amount_as_int(amount);
                    m_code.invokestatic("java/lang/Long", upward ? "rotateLeft" : "rotateRight", "(JI)J");
                }
                else if (kind == jvm_value_kind::integer)
                {
                    m_code.iload(jvm_slot(value));
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >();
                    if (upward)
                    {
                        m_code.append< opcode::ishl >();
                    }
                    else
                    {
                        m_code.append< opcode::iushr >();
                    }
                    m_code.iload(jvm_slot(value));
                    emit_int_constant(m_code, bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().append< opcode::isub >();
                    if (upward)
                    {
                        m_code.append< opcode::iushr >();
                    }
                    else
                    {
                        m_code.append< opcode::ishl >();
                    }
                    m_code.append< opcode::ior >();
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.lload(jvm_slot(value));
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >();
                    if (upward)
                    {
                        m_code.append< opcode::lshl >();
                    }
                    else
                    {
                        m_code.append< opcode::lushr >();
                    }
                    m_code.lload(jvm_slot(value));
                    emit_int_constant(m_code, bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().append< opcode::isub >();
                    if (upward)
                    {
                        m_code.append< opcode::lushr >();
                    }
                    else
                    {
                        m_code.append< opcode::lshl >();
                    }
                    m_code.append< opcode::lor >();
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.aload(jvm_slot(value));
                    emit_unsigned_big_integer_pattern(bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().invokevirtual("java/math/BigInteger", upward ? "shiftLeft" : "shiftRight", "(I)Ljava/math/BigInteger;");
                    m_code.aload(jvm_slot(value));
                    emit_unsigned_big_integer_pattern(bits);
                    emit_int_constant(m_code, bits);
                    emit_shift_amount_as_int(amount);
                    emit_int_constant(m_code, bits);
                    m_code.append< opcode::irem >().append< opcode::isub >().invokevirtual("java/math/BigInteger", upward ? "shiftRight" : "shiftLeft", "(I)Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", "or", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend bitwise rotate received a non-integer value");
                }
                emit_store(m_code, kind, jvm_slot(result));
                canonicalize_integer(result);
            }

            /** Emits the JVM carrier conversion for one integer-represented VMIR local. */
            void emit_integer_carrier_conversion(vmir2::local_index from, vmir2::local_index to)
            {
                jvm_value_kind const source_kind = kind_of(from);
                jvm_value_kind const target_kind = kind_of(to);
                if (source_kind == target_kind)
                {
                    emit_load(m_code, source_kind, jvm_slot(from));
                    return;
                }
                if (source_kind == jvm_value_kind::integer && target_kind == jvm_value_kind::long_)
                {
                    m_code.iload(jvm_slot(from)).append< opcode::i2l >();
                    if (integer_is_unsigned(from) && integer_bit_width(from) == 32)
                    {
                        emit_long_constant(m_code, std::numeric_limits< std::uint32_t >::max());
                        m_code.append< opcode::land >();
                    }
                    return;
                }
                if (source_kind == jvm_value_kind::long_ && target_kind == jvm_value_kind::integer)
                {
                    m_code.lload(jvm_slot(from)).append< opcode::l2i >();
                    return;
                }
                if (target_kind == jvm_value_kind::big_integer)
                {
                    emit_integer_as_big_integer(from);
                    return;
                }
                if (source_kind == jvm_value_kind::big_integer && target_kind == jvm_value_kind::integer)
                {
                    m_code.aload(jvm_slot(from)).invokevirtual("java/math/BigInteger", "intValue", "()I");
                    return;
                }
                if (source_kind == jvm_value_kind::big_integer && target_kind == jvm_value_kind::long_)
                {
                    m_code.aload(jvm_slot(from)).invokevirtual("java/math/BigInteger", "longValue", "()J");
                    return;
                }
                throw compiler_bug("Quxlang's Cortado backend ICONV received a non-integer source or destination");
            }

            /** Verifies that a checked integer conversion preserves its source value. */
            void emit_checked_integer_conversion_validation(vmir2::local_index from, vmir2::local_index to)
            {
                label const conversion_valid = m_code.new_label();
                emit_integer_as_big_integer(from);
                emit_integer_as_big_integer(to);
                m_code.invokevirtual("java/math/BigInteger", "compareTo", "(Ljava/math/BigInteger;)I").branch< opcode::ifeq >(conversion_valid);
                emit_runtime_exception(m_code, "Quxlang checked integer conversion failed");
                m_code.bind(conversion_valid);
            }

            /** Emits a carrier-selected floating-point binary operation. */
            template < opcode FloatOperation, opcode DoubleOperation >
            void emit_float_binary(vmir2::local_index a, vmir2::local_index b, vmir2::local_index result)
            {
                jvm_value_kind const kind = kind_of(result);
                emit_load(m_code, kind, jvm_slot(a));
                emit_load(m_code, kind, jvm_slot(b));
                if (kind == jvm_value_kind::float_)
                {
                    m_code.append< FloatOperation >();
                }
                else if (kind == jvm_value_kind::double_)
                {
                    m_code.append< DoubleOperation >();
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend floating-point operation received a non-floating value");
                }
                emit_store(m_code, kind, jvm_slot(result));
            }

            /** Canonicalizes a floating-point NaN while preserving every non-NaN value exactly. */
            void emit_canonicalize_float(vmir2::canonicalize_float const& instruction)
            {
                jvm_value_kind const source_kind = kind_of(instruction.source);
                if (source_kind != kind_of(instruction.result))
                {
                    throw compiler_bug("Quxlang's Cortado backend FCANON received mismatched source and result types");
                }

                emit_load(m_code, source_kind, jvm_slot(instruction.source));
                if (source_kind == jvm_value_kind::float_)
                {
                    m_code.invokestatic("java/lang/Float", "floatToIntBits", "(F)I");
                    m_code.invokestatic("java/lang/Float", "intBitsToFloat", "(I)F");
                }
                else if (source_kind == jvm_value_kind::double_)
                {
                    m_code.invokestatic("java/lang/Double", "doubleToLongBits", "(D)J");
                    m_code.invokestatic("java/lang/Double", "longBitsToDouble", "(J)D");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend FCANON received a non-floating value");
                }
                emit_store(m_code, source_kind, jvm_slot(instruction.result));
            }

            /** Emits signed or unsigned integer division or remainder. */
            void emit_integer_division(vmir2::local_index a, vmir2::local_index b, vmir2::local_index result, bool remainder)
            {
                jvm_value_kind const kind = kind_of(result);
                emit_load(m_code, kind, jvm_slot(a));
                emit_load(m_code, kind, jvm_slot(b));
                if (kind == jvm_value_kind::integer)
                {
                    if (integer_is_unsigned(result))
                    {
                        m_code.invokestatic("java/lang/Integer", remainder ? "remainderUnsigned" : "divideUnsigned", "(II)I");
                    }
                    else if (remainder)
                    {
                        m_code.append< opcode::irem >();
                    }
                    else
                    {
                        m_code.append< opcode::idiv >();
                    }
                }
                else if (kind == jvm_value_kind::long_)
                {
                    if (integer_is_unsigned(result))
                    {
                        m_code.invokestatic("java/lang/Long", remainder ? "remainderUnsigned" : "divideUnsigned", "(JJ)J");
                    }
                    else if (remainder)
                    {
                        m_code.append< opcode::lrem >();
                    }
                    else
                    {
                        m_code.append< opcode::ldiv >();
                    }
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", remainder ? "remainder" : "divide", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                }
                else
                {
                    throw compiler_bug("Quxlang's Cortado backend integer division received a non-integer value");
                }
                emit_store(m_code, kind, jvm_slot(result));
                canonicalize_integer(result);
            }

            /** Emits an integer-represented comparison and stores its BOOL result. */
            void emit_comparison_boolean(vmir2::cmp_bool const& instruction)
            {
                label const relation_true = m_code.new_label();
                label const relation_done = m_code.new_label();
                m_code.iload(jvm_slot(instruction.ordering));
                switch (instruction.relation)
                {
                case vmir2::comparison_relation::equal:
                    m_code.branch< opcode::ifeq >(relation_true);
                    break;
                case vmir2::comparison_relation::not_equal:
                    m_code.branch< opcode::ifne >(relation_true);
                    break;
                case vmir2::comparison_relation::less:
                    m_code.branch< opcode::iflt >(relation_true);
                    break;
                case vmir2::comparison_relation::less_equal:
                    m_code.branch< opcode::ifle >(relation_true);
                    break;
                case vmir2::comparison_relation::greater:
                    m_code.branch< opcode::ifgt >(relation_true);
                    break;
                case vmir2::comparison_relation::greater_equal:
                    m_code.branch< opcode::ifge >(relation_true);
                    break;
                }
                m_code.append< opcode::iconst_0 >().branch< opcode::goto_ >(relation_done).bind(relation_true).append< opcode::iconst_1 >().bind(relation_done).istore(jvm_slot(instruction.result));
            }

            /** Converts one supported JVM carrier to a Quxlang BOOL result. */
            void emit_boolean_from_value(vmir2::local_index from, vmir2::local_index to, bool invert)
            {
                label const value_true = m_code.new_label();
                label const value_done = m_code.new_label();
                jvm_value_kind const kind = kind_of(from);
                emit_load(m_code, kind, jvm_slot(from));
                if (kind == jvm_value_kind::integer)
                {
                    invert ? m_code.branch< opcode::ifeq >(value_true) : m_code.branch< opcode::ifne >(value_true);
                }
                else if (kind == jvm_value_kind::long_)
                {
                    m_code.append< opcode::lconst_0 >().append< opcode::lcmp >();
                    invert ? m_code.branch< opcode::ifeq >(value_true) : m_code.branch< opcode::ifne >(value_true);
                }
                else if (kind == jvm_value_kind::big_integer)
                {
                    m_code.invokevirtual("java/math/BigInteger", "signum", "()I");
                    invert ? m_code.branch< opcode::ifeq >(value_true) : m_code.branch< opcode::ifne >(value_true);
                }
                else if (kind == jvm_value_kind::float_)
                {
                    m_code.append< opcode::fconst_0 >().append< opcode::fcmpl >();
                    invert ? m_code.branch< opcode::ifeq >(value_true) : m_code.branch< opcode::ifne >(value_true);
                }
                else if (kind == jvm_value_kind::double_)
                {
                    m_code.append< opcode::dconst_0 >().append< opcode::dcmpl >();
                    invert ? m_code.branch< opcode::ifeq >(value_true) : m_code.branch< opcode::ifne >(value_true);
                }
                else if (kind == jvm_value_kind::reference)
                {
                    invert ? m_code.branch< opcode::ifnull >(value_true) : m_code.branch< opcode::ifnonnull >(value_true);
                }
                else
                {
                    throw rpnx::unimplemented();
                }

                m_code.append< opcode::iconst_0 >().branch< opcode::goto_ >(value_done).bind(value_true).append< opcode::iconst_1 >().bind(value_done).istore(jvm_slot(to));
            }

            /** Compares managed references by owner identity and logical index. */
            void emit_pointer_comparison(vmir2::local_index a, vmir2::local_index b, vmir2::local_index result)
            {
                m_code.aload(jvm_slot(a)).aload(jvm_slot(b)).invokestatic("quxlang/runtime/QuxlangReference", "compare", "(Ljava/lang/Object;Ljava/lang/Object;)I").istore(jvm_slot(result));
            }

            /** Emits one validated JVM method or field binding. */
            void emit_external_invoke(vmir2::invoke const& instruction, resolved_jvm_external_callable const& callable)
            {
                std::size_t positional_index = 0;
                auto argument_local = [&](jvm_external_parameter const& parameter) -> vmir2::local_index
                {
                    if (parameter.api_name.has_value())
                    {
                        std::map< std::string, vmir2::local_index >::const_iterator const argument = instruction.args.named.find(*parameter.api_name);
                        if (argument == instruction.args.named.end())
                        {
                            throw compiler_bug("Quxlang's Cortado backend JVM external call is missing named argument " + *parameter.api_name);
                        }
                        return argument->second;
                    }
                    if (positional_index >= instruction.args.positional.size())
                    {
                        throw compiler_bug("Quxlang's Cortado backend JVM external call is missing a positional argument");
                    }
                    return instruction.args.positional.at(positional_index++);
                };

                auto emit_external_argument = [&](jvm_external_parameter const& parameter) -> void
                {
                    vmir2::local_index const argument = argument_local(parameter);
                    emit_load(m_code, kind_of(argument), jvm_slot(argument));
                    if (parameter.type.type_is< ptrref_type >() && parameter.type.get_as< ptrref_type >().ptr_class == pointer_class::gc)
                    {
                        ptrref_type const& pointer = parameter.type.get_as< ptrref_type >();
                        std::map< type_symbol, jvm_external_type_info >::const_iterator const external = m_input.external_types.find(pointer.target);
                        if (external == m_input.external_types.end())
                        {
                            throw compiler_bug("Quxlang's Cortado backend JVM external argument type is absent from the compilation packet");
                        }
                        m_code.checkcast(external->second.internal_name);
                    }
                };

                jvm_external_parameter const* receiver = nullptr;
                for (jvm_external_parameter const& parameter : callable.parameters)
                {
                    if (parameter.api_name == "THIS")
                    {
                        receiver = &parameter;
                        break;
                    }
                }

                if (receiver != nullptr)
                {
                    emit_external_argument(*receiver);
                }

                if (callable.call_kind == jvm_call_kind::constructor)
                {
                    m_code.new_(callable.owner_internal_name).append< opcode::dup >();
                }

                for (jvm_external_parameter const& parameter : callable.parameters)
                {
                    if (parameter.api_name != "THIS")
                    {
                        emit_external_argument(parameter);
                    }
                }

                switch (callable.call_kind)
                {
                case jvm_call_kind::static_method:
                    m_code.invokestatic(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                case jvm_call_kind::virtual_method:
                    m_code.invokevirtual(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                case jvm_call_kind::interface_method:
                    m_code.invokeinterface(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                case jvm_call_kind::constructor:
                    m_code.invokespecial(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                case jvm_call_kind::get_static:
                    m_code.getstatic(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                case jvm_call_kind::put_static:
                    m_code.putstatic(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                case jvm_call_kind::get_field:
                    m_code.getfield(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                case jvm_call_kind::put_field:
                    m_code.putfield(callable.owner_internal_name, callable.member_name, callable.descriptor);
                    break;
                }

                if (callable.return_type.has_value())
                {
                    std::map< std::string, vmir2::local_index >::const_iterator const result = instruction.args.named.find("RETURN");
                    if (result == instruction.args.named.end())
                    {
                        throw compiler_bug("Quxlang's Cortado backend JVM external call returning a value has no RETURN slot");
                    }
                    emit_store(m_code, kind_of(result->second), jvm_slot(result->second));
                    if (m_scalar_reference_owner_slots.at(local_slot(result->second)).has_value())
                    {
                        emit_scalar_reference_owner_value(result->second);
                    }
                }

                if (positional_index != instruction.args.positional.size())
                {
                    throw compiler_bug("Quxlang's Cortado backend JVM external call has extra positional arguments");
                }
            }

            /** Returns the concrete procedure signature carried by an indirect-call local. */
            auto indirect_signature(vmir2::local_index callable_index) const -> procedure_type
            {
                type_symbol callable_type = remove_ref(m_routine.local_types.at(local_slot(callable_index)).type);
                if (callable_type.type_is< procedure_type >())
                {
                    return callable_type.get_as< procedure_type >();
                }
                if (callable_type.type_is< ptrref_type >())
                {
                    ptrref_type const& pointer = callable_type.get_as< ptrref_type >();
                    if (pointer.ptr_class == pointer_class::instance && pointer.target.type_is< procedure_type >())
                    {
                        return pointer.target.get_as< procedure_type >();
                    }
                }
                throw compiler_bug("Quxlang's Cortado backend INVOKE_INDIRECT local does not carry a procedure type");
            }

            /** Invokes a QuxlangCallable already on the JVM operand stack. */
            void emit_callable_invocation(vmir2::invocation_args const& args, invotype const& parameter_types)
            {
                if (args.positional.size() != parameter_types.positional.size())
                {
                    throw compiler_bug("Quxlang's Cortado backend callable invocation has the wrong positional argument count");
                }
                std::size_t const argument_count = parameter_types.positional.size() + parameter_types.named.size();
                emit_int_constant(m_code, static_cast< std::uint32_t >(argument_count));
                m_code.anewarray("java/lang/Object");

                std::size_t array_index = 0;
                auto emit_array_argument = [&](vmir2::local_index argument) -> void
                {
                    m_code.append< opcode::dup >();
                    emit_int_constant(m_code, static_cast< std::uint32_t >(array_index));
                    emit_call_argument(argument);
                    cortado_jar_emitter_impl::emit_boxed_stack_value(m_code, kind_of(argument));
                    m_code.append< opcode::aastore >();
                    ++array_index;
                };
                for (vmir2::local_index const argument : args.positional)
                {
                    emit_array_argument(argument);
                }
                for (std::pair< std::string const, type_symbol > const& parameter : parameter_types.named)
                {
                    std::map< std::string, vmir2::local_index >::const_iterator const argument = args.named.find(parameter.first);
                    if (argument == args.named.end())
                    {
                        throw compiler_bug("Quxlang's Cortado backend callable invocation is missing named argument " + parameter.first);
                    }
                    emit_array_argument(argument->second);
                }
                m_code.invokevirtual("quxlang/runtime/QuxlangCallable", "invoke", "([Ljava/lang/Object;)Ljava/lang/Object;");

                std::map< std::string, vmir2::local_index >::const_iterator const result = args.named.find("RETURN");
                if (result == args.named.end())
                {
                    m_code.append< opcode::pop >();
                }
                else
                {
                    jvm_value_kind const result_kind = kind_of(result->second);
                    emit_unboxed_stack_value(m_code, result_kind);
                    emit_store(m_code, result_kind, jvm_slot(result->second));
                    if (m_scalar_reference_owner_slots.at(local_slot(result->second)).has_value())
                    {
                        emit_scalar_reference_owner_value(result->second);
                    }
                }

                emit_invocation_scalar_reference_results(args);
            }

            /** Emits a generic JVM procedure-value invocation through QuxlangCallable. */
            void emit_indirect_invoke(vmir2::invoke_indirect const& instruction)
            {
                procedure_type const signature = indirect_signature(instruction.what_index);
                type_symbol const& callable_type = m_routine.local_types.at(local_slot(instruction.what_index)).type;
                if (m_reference_aliases.at(local_slot(instruction.what_index)).has_value())
                {
                    vmir2::local_index const callable = resolved_reference(instruction.what_index);
                    m_code.aload(jvm_slot(callable));
                }
                else if (is_ref(callable_type))
                {
                    load_boxed_managed_reference_value(instruction.what_index, m_reference_owner_slot);
                    m_code.aload(m_reference_owner_slot);
                }
                else
                {
                    m_code.aload(jvm_slot(instruction.what_index));
                }
                m_code.checkcast("quxlang/runtime/QuxlangCallable");
                emit_callable_invocation(instruction.args, signature.signature.params);
            }

            /** Materializes a deterministic adapter for one direct Quxlang procedure. */
            void emit_procedure_value(vmir2::get_procedure_ptr const& instruction)
            {
                if (instruction.calling_convention != "DEFAULT")
                {
                    throw rpnx::unimplemented();
                }
                if (!m_input.routines.contains(instruction.routine) || !m_routine_infos.contains(instruction.routine))
                {
                    throw compiler_bug("Quxlang's Cortado backend procedure-value target is outside the aggregated closure: " + to_string(instruction.routine));
                }
                std::string const adapter_name = callable_adapter_class_name(instruction.routine);
                m_code.new_(adapter_name).append< opcode::dup >().invokespecial(adapter_name, "<init>", "()V").astore(jvm_slot(instruction.pointer_index));
            }

            /** Loads one Quxlang interface handle onto the JVM operand stack. */
            void emit_interface_value(vmir2::local_index interface_value)
            {
                type_symbol const& interface_type = m_routine.local_types.at(local_slot(interface_value)).type;
                if (m_reference_aliases.at(local_slot(interface_value)).has_value())
                {
                    vmir2::local_index const target = resolved_reference(interface_value);
                    m_code.aload(jvm_slot(target));
                }
                else if (is_ref(interface_type))
                {
                    load_boxed_managed_reference_value(interface_value, m_reference_owner_slot);
                    m_code.aload(m_reference_owner_slot);
                }
                else
                {
                    m_code.aload(jvm_slot(interface_value));
                }
                m_code.checkcast("quxlang/runtime/QuxlangInterface");
            }

            /** Materializes a JVM interface handle and its overload-safe callable table. */
            void emit_interface_init(vmir2::interface_init const& instruction)
            {
                if (kind_of(instruction.target) != jvm_value_kind::reference)
                {
                    throw compiler_bug("Quxlang's Cortado backend interface handle does not use a JVM reference carrier");
                }
                m_code.new_("quxlang/runtime/QuxlangInterface").append< opcode::dup >();
                if (instruction.is_default)
                {
                    m_code.append< opcode::iconst_1 >();
                }
                else
                {
                    m_code.append< opcode::iconst_0 >();
                }
                m_code.invokespecial("quxlang/runtime/QuxlangInterface", "<init>", "(Z)V").astore(jvm_slot(instruction.target));

                for (std::pair< interface_slot_key const, type_symbol > const& implementation : instruction.functions)
                {
                    if (!m_input.routines.contains(implementation.second) || !m_routine_infos.contains(implementation.second))
                    {
                        throw compiler_bug("Quxlang's Cortado backend interface implementation is outside the aggregated closure: " + to_string(implementation.second));
                    }
                    std::string const adapter_name = callable_adapter_class_name(implementation.second);
                    m_code.aload(jvm_slot(instruction.target)).getfield("quxlang/runtime/QuxlangInterface", "functions", "Ljava/util/HashMap;").ldc_string(to_string(implementation.first));
                    m_code.new_(adapter_name).append< opcode::dup >().invokespecial(adapter_name, "<init>", "()V");
                    m_code.invokevirtual("java/util/HashMap", "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;").append< opcode::pop >();
                }
            }

            /** Dispatches one interface call through its concrete slot or default body. */
            void emit_interface_invoke(vmir2::interface_invoke const& instruction)
            {
                label const implementation_dispatch = m_code.new_label();
                label const complete = m_code.new_label();
                emit_interface_value(instruction.interface_value);
                m_code.getfield("quxlang/runtime/QuxlangInterface", "isDefault", "Z").branch< opcode::ifeq >(implementation_dispatch);

                if (instruction.default_function.has_value())
                {
                    vmir2::invoke default_invocation{
                        .what = *instruction.default_function,
                        .args = instruction.args,
                    };
                    default_invocation.args.named["THIS"] = instruction.interface_value;
                    emit_direct_invoke(default_invocation);
                    m_code.branch< opcode::goto_ >(complete);
                }
                else
                {
                    emit_runtime_exception(m_code, "Quxlang interface default value invoked without a default implementation");
                }

                m_code.bind(implementation_dispatch);
                emit_interface_value(instruction.interface_value);
                m_code.getfield("quxlang/runtime/QuxlangInterface", "functions", "Ljava/util/HashMap;").ldc_string(to_string(instruction.slot));
                m_code.invokevirtual("java/util/HashMap", "get", "(Ljava/lang/Object;)Ljava/lang/Object;").append< opcode::dup >();
                label const implementation_found = m_code.new_label();
                m_code.branch< opcode::ifnonnull >(implementation_found).append< opcode::pop >();
                emit_runtime_exception(m_code, "Quxlang interface value invoked without a matching implementation slot");
                m_code.bind(implementation_found).checkcast("quxlang/runtime/QuxlangCallable");
                emit_callable_invocation(instruction.args, instruction.slot.concrete_params);
                m_code.bind(complete);
            }

            /** Reads whether a Quxlang interface handle denotes its default value. */
            void emit_interface_is_default(vmir2::interface_is_default const& instruction)
            {
                if (kind_of(instruction.result) != jvm_value_kind::integer)
                {
                    throw compiler_bug("Quxlang's Cortado backend interface default query does not produce a JVM integer carrier");
                }
                emit_interface_value(instruction.interface_value);
                m_code.getfield("quxlang/runtime/QuxlangInterface", "isDefault", "Z").istore(jvm_slot(instruction.result));
            }

            /** Emits one runtime-created Quxlang STRING_CONSTANT as a JVM call argument. */
            void emit_runtime_string_argument(std::string_view value)
            {
                std::span< std::byte const > const bytes(reinterpret_cast< std::byte const* >(value.data()), value.size());
                emit_readonly_byte_span_object(m_code, bytes, m_reference_owner_slot);
            }

            /** Emits source file index, line, and column values for one runtime diagnostic call. */
            auto runtime_source_coordinates(std::optional< source_location > const& location) const -> std::tuple< std::uint64_t, std::uint64_t, std::uint64_t >
            {
                if (!location.has_value() || !m_input.source_index.has_value())
                {
                    return {0, 0, 0};
                }
                vmir2::source_index const& source_index = m_input.source_index->get();
                std::map< std::uint64_t, vmir2::indexed_source_file >::const_iterator const file = source_index.files.find(location->file_id);
                if (file == source_index.files.end())
                {
                    return {0, 0, 0};
                }
                vmir2::source_position const position = file->second.position(location->begin_index);
                return {
                    location->file_id,
                    static_cast< std::uint64_t >(position.line),
                    static_cast< std::uint64_t >(position.column),
                };
            }

            /** Emits a call to MODULE(RUNTIME)::ASSERT_FAIL and prevents false-path continuation. */
            void emit_runtime_assertion_failure(vmir2::assert_instr const& instruction)
            {
                std::map< vmir_runtime_dependency, type_symbol >::const_iterator const resolved = m_input.resolved_runtime_procedures.find(vmir_runtime_dependency::assert_fail);
                if (resolved == m_input.resolved_runtime_procedures.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend assertion has no resolved runtime procedure");
                }
                std::map< type_symbol, routine_jvm_info >::const_iterator const info = m_routine_infos.find(resolved->second);
                std::map< type_symbol, vmir2::functanoid_routine3 >::const_iterator const routine = m_input.routines.find(resolved->second);
                if (info == m_routine_infos.end() || routine == m_input.routines.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend ASSERT_FAIL runtime procedure is outside the routine closure");
                }
                if (info->second.argument_frame_class_name.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend ASSERT_FAIL unexpectedly requires an argument frame");
                }
                auto [file, line, column] = runtime_source_coordinates(instruction.location);
                for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine->second.parameters.named)
                {
                    if (parameter.first == "RETURN")
                    {
                        continue;
                    }
                    if (parameter.first == "column")
                    {
                        emit_long_constant(m_code, column);
                    }
                    else if (parameter.first == "expr")
                    {
                        emit_runtime_string_argument(instruction.expr_text);
                    }
                    else if (parameter.first == "file")
                    {
                        emit_long_constant(m_code, file);
                    }
                    else if (parameter.first == "line")
                    {
                        emit_long_constant(m_code, line);
                    }
                    else if (parameter.first == "tag")
                    {
                        m_code.append< opcode::aconst_null >();
                    }
                    else
                    {
                        throw compiler_bug("Unexpected Quxlang Cortado backend ASSERT_FAIL runtime parameter: " + parameter.first);
                    }
                }
                m_code.invokestatic(info->second.class_name, "invoke", info->second.descriptor);
                label const stopped = m_code.new_label();
                m_code.bind(stopped).branch< opcode::goto_ >(stopped);
            }

            /** Emits a call to MODULE(RUNTIME)::PANIC and prevents terminator fallthrough. */
            void emit_runtime_panic(vmir2::panic const& instruction)
            {
                std::map< vmir_runtime_dependency, type_symbol >::const_iterator const resolved = m_input.resolved_runtime_procedures.find(vmir_runtime_dependency::panic);
                if (resolved == m_input.resolved_runtime_procedures.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend panic has no resolved runtime procedure");
                }
                std::map< type_symbol, routine_jvm_info >::const_iterator const info = m_routine_infos.find(resolved->second);
                std::map< type_symbol, vmir2::functanoid_routine3 >::const_iterator const routine = m_input.routines.find(resolved->second);
                if (info == m_routine_infos.end() || routine == m_input.routines.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend PANIC runtime procedure is outside the routine closure");
                }
                if (info->second.argument_frame_class_name.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend PANIC unexpectedly requires an argument frame");
                }
                auto [file, line, column] = runtime_source_coordinates(instruction.location);
                for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine->second.parameters.named)
                {
                    if (parameter.first == "RETURN")
                    {
                        continue;
                    }
                    if (parameter.first == "column")
                    {
                        emit_long_constant(m_code, column);
                    }
                    else if (parameter.first == "file")
                    {
                        emit_long_constant(m_code, file);
                    }
                    else if (parameter.first == "line")
                    {
                        emit_long_constant(m_code, line);
                    }
                    else if (parameter.first == "message")
                    {
                        emit_runtime_string_argument(instruction.message);
                    }
                    else
                    {
                        throw compiler_bug("Unexpected Quxlang Cortado backend PANIC runtime parameter: " + parameter.first);
                    }
                }
                m_code.invokestatic(info->second.class_name, "invoke", info->second.descriptor);
                label const stopped = m_code.new_label();
                m_code.bind(stopped).branch< opcode::goto_ >(stopped);
            }

            /** Materializes the managed reference passed to a Quxlang initialization-guard procedure. */
            void emit_runtime_initguard_reference(type_symbol const& global)
            {
                m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().getstatic("quxlang/runtime/GeneratedGlobals", global_initialization_field_name(global), "Lquxlang/runtime/QuxlangObject;").append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V");
            }

            /** Calls one resolved Quxlang initialization-guard runtime procedure. */
            void emit_runtime_initguard_call(vmir_runtime_dependency dependency, vmir2::local_index guard)
            {
                std::map< vmir_runtime_dependency, type_symbol >::const_iterator const resolved = m_input.resolved_runtime_procedures.find(dependency);
                if (resolved == m_input.resolved_runtime_procedures.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend initialization guard has no resolved runtime procedure");
                }
                std::map< type_symbol, routine_jvm_info >::const_iterator const info = m_routine_infos.find(resolved->second);
                if (info == m_routine_infos.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend initialization-guard runtime procedure is outside the routine closure");
                }
                if (info->second.argument_frame_class_name.has_value())
                {
                    throw compiler_bug("Quxlang's Cortado backend initialization-guard runtime procedure unexpectedly requires an argument frame");
                }
                emit_call_argument(guard);
                m_code.invokestatic(info->second.class_name, "invoke", info->second.descriptor);
            }

            /** Emits a direct static invocation, including argument-frame and reference synchronization. */
            void emit_direct_invoke(vmir2::invoke const& instruction)
            {
                std::map< type_symbol, resolved_jvm_external_callable >::const_iterator const external = m_input.external_callables.find(instruction.what);
                if (external != m_input.external_callables.end())
                {
                    emit_external_invoke(instruction, external->second);
                    return;
                }
                std::map< type_symbol, routine_jvm_info >::const_iterator const info_iter = m_routine_infos.find(instruction.what);
                std::map< type_symbol, vmir2::functanoid_routine3 >::const_iterator const routine_iter = m_input.routines.find(instruction.what);
                if (info_iter == m_routine_infos.end() || routine_iter == m_input.routines.end())
                {
                    throw compiler_bug("Quxlang's Cortado backend call target is outside the aggregated closure: " + to_string(instruction.what));
                }
                vmir2::functanoid_routine3 const& callee = routine_iter->second;
                if (instruction.args.positional.size() != callee.parameters.positional.size())
                {
                    throw compiler_bug("Quxlang's Cortado backend positional call argument count mismatch");
                }
                if (info_iter->second.argument_frame_class_name.has_value())
                {
                    std::string const& frame_class_name = *info_iter->second.argument_frame_class_name;
                    m_code.new_(frame_class_name).append< opcode::dup >().invokespecial(frame_class_name, "<init>", "()V");
                    std::size_t parameter_index = 0;
                    auto store_argument = [&](vmir2::local_index argument, type_symbol const& parameter_type) -> void
                    {
                        jvm_value_kind const kind = value_kind(m_input, parameter_type);
                        m_code.append< opcode::dup >();
                        emit_call_argument(argument, parameter_type);
                        m_code.putfield(frame_class_name, "p" + std::to_string(parameter_index), descriptor_for_kind(kind));
                        ++parameter_index;
                    };
                    for (std::size_t argument_index = 0; argument_index < instruction.args.positional.size(); ++argument_index)
                    {
                        store_argument(instruction.args.positional.at(argument_index), callee.parameters.positional.at(argument_index).type);
                    }
                    for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : callee.parameters.named)
                    {
                        if (parameter.first == "RETURN")
                        {
                            continue;
                        }
                        std::map< std::string, vmir2::local_index >::const_iterator const argument = instruction.args.named.find(parameter.first);
                        if (argument == instruction.args.named.end())
                        {
                            throw compiler_bug("Quxlang's Cortado backend named call argument is missing: " + parameter.first);
                        }
                        store_argument(argument->second, parameter.second.type);
                    }
                    m_code.invokestatic(info_iter->second.class_name, "invoke", info_iter->second.descriptor);
                }
                else
                {
                    for (std::size_t argument_index = 0; argument_index < instruction.args.positional.size(); ++argument_index)
                    {
                        vmir2::local_index const argument = instruction.args.positional.at(argument_index);
                        emit_call_argument(argument, callee.parameters.positional.at(argument_index).type);
                    }
                    for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : callee.parameters.named)
                    {
                        if (parameter.first == "RETURN")
                        {
                            continue;
                        }
                        std::map< std::string, vmir2::local_index >::const_iterator const argument = instruction.args.named.find(parameter.first);
                        if (argument == instruction.args.named.end())
                        {
                            throw compiler_bug("Quxlang's Cortado backend named call argument is missing: " + parameter.first);
                        }
                        emit_call_argument(argument->second, parameter.second.type);
                    }
                    m_code.invokestatic(info_iter->second.class_name, "invoke", info_iter->second.descriptor);
                }
                std::map< std::string, vmir2::local_index >::const_iterator const result = instruction.args.named.find("RETURN");
                if (info_iter->second.return_kind != jvm_value_kind::void_)
                {
                    if (result == instruction.args.named.end())
                    {
                        throw compiler_bug("Quxlang's Cortado backend non-void call has no RETURN slot");
                    }
                    emit_store(m_code, info_iter->second.return_kind, jvm_slot(result->second));
                    if (m_scalar_reference_owner_slots.at(local_slot(result->second)).has_value())
                    {
                        emit_scalar_reference_owner_value(result->second);
                    }
                }
                emit_invocation_scalar_reference_results(instruction.args);
            }

            /** Returns the declared parameter type for a formal parameter local. */
            auto routine_parameter_type(vmir2::local_index slot) const -> std::optional< type_symbol >
            {
                for (vmir2::routine_parameter const& parameter : m_routine.parameters.positional)
                {
                    if (parameter.local_index == slot)
                    {
                        return parameter.type;
                    }
                }
                for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : m_routine.parameters.named)
                {
                    if (parameter.second.local_index == slot)
                    {
                        return parameter.second.type;
                    }
                }
                return std::nullopt;
            }

            /** Returns whether a VMIR slot is a non-owning view of another slot's lifetime. */
            static auto is_cleanup_alias(vmir2::slot_state const& state) -> bool
            {
                return state.delegate_of.has_value() || state.array_delegate_of_initializer.has_value() || state.destroy_delegate || state.is_projection;
            }

            /** Returns whether a completed struct delegate loses its owner on one control-flow edge. */
            static auto struct_delegate_needs_cleanup(vmir2::slot_state const& state, vmir2::state_map const& target_state) -> bool
            {
                if (!state.delegate_of.has_value() || !state.struct_delegate_selector.has_value() || state.stage != vmir2::slot_stage::full || !state.nontrivial_dtor.has_value())
                {
                    return false;
                }
                vmir2::state_map::const_iterator const owner = target_state.find(*state.delegate_of);
                return owner == target_state.end() || !owner->second.alive();
            }

            /** Invokes the destructor selected for one live VMIR slot. */
            void emit_slot_destructor_call(vmir2::local_index slot, vmir2::slot_state const& state)
            {
                type_symbol destructor;
                vmir2::invocation_args arguments;
                if (state.nontrivial_dtor.has_value())
                {
                    destructor = state.nontrivial_dtor->func;
                    arguments = state.nontrivial_dtor->args;
                    std::map< std::string, vmir2::local_index >::const_iterator const this_argument = arguments.named.find("THIS");
                    if (this_argument == arguments.named.end() || this_argument->second != slot)
                    {
                        throw compiler_bug("Quxlang's Cortado backend deferred destructor THIS argument does not match its target slot");
                    }
                }
                else
                {
                    type_symbol const& slot_type = m_routine.local_types.at(local_slot(slot)).type;
                    std::map< type_symbol, type_symbol >::const_iterator const selected = m_routine.non_trivial_dtors.find(slot_type);
                    if (selected == m_routine.non_trivial_dtors.end())
                    {
                        return;
                    }
                    destructor = selected->second;
                    arguments.named.emplace("THIS", slot);
                }

                emit_direct_invoke(vmir2::invoke{
                    .what = std::move(destructor),
                    .args = std::move(arguments),
                });
            }

            /** Emits lifetime cleanup for values that do not survive one control-flow transition. */
            void emit_transition_cleanup(vmir2::state_map const& current_state, vmir2::state_map const& target_state, bool normal_return)
            {
                for (vmir2::state_map::const_reverse_iterator entry = current_state.crbegin(); entry != current_state.crend(); ++entry)
                {
                    bool const survives = target_state.contains(entry->first) && target_state.at(entry->first).alive();
                    if (!survives && struct_delegate_needs_cleanup(entry->second, target_state))
                    {
                        emit_slot_destructor_call(entry->first, entry->second);
                    }
                }
                for (std::pair< vmir2::local_index const, vmir2::slot_state > const& entry : current_state)
                {
                    vmir2::local_index const slot = entry.first;
                    vmir2::slot_state const& state = entry.second;
                    bool const survives = target_state.contains(slot) && target_state.at(slot).alive();
                    if (survives || !state.alive() || is_cleanup_alias(state))
                    {
                        continue;
                    }

                    type_symbol const slot_type = unwrapped_type(m_routine.local_types.at(local_slot(slot)).type);
                    if (slot_type.type_is< initguard_lock_type >())
                    {
                        emit_runtime_initguard_call(vmir_runtime_dependency::initguard_abort, slot);
                        continue;
                    }
                    if (!state.dtor_enabled())
                    {
                        continue;
                    }
                    if (normal_return)
                    {
                        std::optional< type_symbol > const parameter_type = routine_parameter_type(slot);
                        if (parameter_type.has_value() && parameter_type->type_is< dvalue_slot >())
                        {
                            continue;
                        }
                    }
                    emit_slot_destructor_call(slot, state);
                }
            }

            /** Emits cleanup for one control-flow edge and transfers to its VMIR block. */
            void emit_cleanup_edge(vmir2::state_map const& current_state, vmir2::block_index target)
            {
                emit_transition_cleanup(current_state, m_routine.blocks.at(block_slot(target)).entry_state, false);
                m_code.branch< opcode::goto_ >(m_block_labels.at(block_slot(target)));
            }

            /** Emits cleanup required before a routine's normal return. */
            void emit_return_cleanup(vmir2::state_map const& current_state)
            {
                vmir2::state_map normal_exit;
                vmir2::codegen_state_engine state_engine(normal_exit, m_routine.local_types, m_routine.parameters);
                state_engine.apply_normal_exit();
                emit_transition_cleanup(current_state, normal_exit, true);
            }

            /** Lowers one supported VMIR instruction into JVM bytecode. */
            void emit_instruction(vmir2::vm_instruction const& instruction, vmir2::state_map const& current_state)
            {
                rpnx::apply_visitor< void >(instruction,
                                            [&](auto& selected) -> void
                                            {
                                                using instruction_type = std::decay_t< decltype(selected) >;
                                                if constexpr (std::is_same_v< instruction_type, vmir2::load_const_int >)
                                                {
                                                    jvm_value_kind const kind = kind_of(selected.target);
                                                    if (kind == jvm_value_kind::big_integer)
                                                    {
                                                        emit_big_integer_from_decimal(m_code, selected.value);
                                                    }
                                                    else
                                                    {
                                                        std::uint64_t const bits = parse_integer_bits(selected.value);
                                                        if (kind == jvm_value_kind::integer)
                                                        {
                                                            emit_int_constant(m_code, static_cast< std::uint32_t >(bits));
                                                        }
                                                        else
                                                        {
                                                            emit_long_constant(m_code, bits);
                                                        }
                                                    }
                                                    emit_store(m_code, kind, jvm_slot(selected.target));
                                                    canonicalize_integer(selected.target);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::load_const_bool >)
                                                {
                                                    if (selected.value)
                                                    {
                                                        m_code.append< opcode::iconst_1 >();
                                                    }
                                                    else
                                                    {
                                                        m_code.append< opcode::iconst_0 >();
                                                    }
                                                    m_code.istore(jvm_slot(selected.target));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::load_type_index >)
                                                {
                                                    std::map< type_symbol, std::uint64_t >::const_iterator const ordinal = m_input.type_index_ordinals.find(selected.indexed_type);
                                                    if (ordinal == m_input.type_index_ordinals.end())
                                                    {
                                                        throw compiler_bug("Missing Cortado TYPE_INDEX ordinal for " + to_string(selected.indexed_type));
                                                    }
                                                    emit_long_constant(m_code, ordinal->second);
                                                    m_code.lstore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::load_const_zero >)
                                                {
                                                    emit_default_value(m_code, kind_of(selected.target));
                                                    emit_store(m_code, kind_of(selected.target), jvm_slot(selected.target));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::load_const_value >)
                                                {
                                                    type_symbol const type = unwrapped_type(m_routine.local_types.at(local_slot(selected.target)).type);
                                                    if (!type.type_is< readonly_constant >())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend received load_const_value for a non-readonly-constant local");
                                                    }
                                                    emit_readonly_byte_span_object(m_code, selected.value, jvm_slot(selected.target));
                                                    m_code.astore(jvm_slot(selected.target));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::load_const_float >)
                                                {
                                                    jvm_value_kind const kind = kind_of(selected.target);
                                                    if (kind == jvm_value_kind::float_)
                                                    {
                                                        float const value = std::stof(selected.value);
                                                        emit_int_constant(m_code, std::bit_cast< std::uint32_t >(value));
                                                        m_code.invokestatic("java/lang/Float", "intBitsToFloat", "(I)F");
                                                    }
                                                    else
                                                    {
                                                        double const value = std::stod(selected.value);
                                                        emit_long_constant(m_code, std::bit_cast< std::uint64_t >(value));
                                                        m_code.invokestatic("java/lang/Double", "longBitsToDouble", "(J)D");
                                                    }
                                                    emit_store(m_code, kind, jvm_slot(selected.target));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::load_const_enum >)
                                                {
                                                    type_symbol const enum_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.target)).type);
                                                    enum_info const& info = m_input.enum_definitions.at(enum_type);
                                                    std::vector< std::byte > const& bytes = info.values.at(selected.case_name).value;
                                                    jvm_value_kind const kind = kind_of(selected.target);
                                                    if (kind == jvm_value_kind::big_integer)
                                                    {
                                                        emit_big_integer_from_little_endian(m_code, bytes);
                                                    }
                                                    else
                                                    {
                                                        std::uint64_t bits = 0;
                                                        for (std::size_t i = 0; i < bytes.size(); ++i)
                                                        {
                                                            bits |= static_cast< std::uint64_t >(std::to_integer< std::uint8_t >(bytes.at(i))) << (i * 8);
                                                        }
                                                        if (kind == jvm_value_kind::integer)
                                                        {
                                                            emit_int_constant(m_code, static_cast< std::uint32_t >(bits));
                                                        }
                                                        else
                                                        {
                                                            emit_long_constant(m_code, bits);
                                                        }
                                                    }
                                                    emit_store(m_code, kind, jvm_slot(selected.target));
                                                    canonicalize_integer(selected.target);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::enum_int_inrange >)
                                                {
                                                    std::map< type_symbol, enum_info >::const_iterator const info_iter = m_input.enum_definitions.find(selected.enum_type);
                                                    if (info_iter == m_input.enum_definitions.end())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend ENUM_INT_INRANGE references a type without semantic enum information: " + to_string(selected.enum_type));
                                                    }
                                                    label const matches = m_code.new_label();
                                                    label const complete = m_code.new_label();
                                                    jvm_value_kind const integer_kind = kind_of(selected.integer);
                                                    type_symbol const integer_type = m_routine.local_types.at(local_slot(selected.integer)).type;
                                                    for (std::pair< std::string const, enum_value_info > const& value : info_iter->second.values)
                                                    {
                                                        emit_load(m_code, integer_kind, jvm_slot(selected.integer));
                                                        if (integer_kind == jvm_value_kind::integer)
                                                        {
                                                            std::uint64_t bits = 0;
                                                            for (std::size_t index = 0; index < value.second.value.size(); ++index)
                                                            {
                                                                bits |= static_cast< std::uint64_t >(std::to_integer< std::uint8_t >(value.second.value.at(index))) << (index * 8);
                                                            }
                                                            emit_int_constant(m_code, static_cast< std::uint32_t >(bits));
                                                            emit_integer_canonicalization(integer_type, integer_kind);
                                                            m_code.branch< opcode::if_icmpeq >(matches);
                                                        }
                                                        else if (integer_kind == jvm_value_kind::long_)
                                                        {
                                                            std::uint64_t bits = 0;
                                                            for (std::size_t index = 0; index < value.second.value.size(); ++index)
                                                            {
                                                                bits |= static_cast< std::uint64_t >(std::to_integer< std::uint8_t >(value.second.value.at(index))) << (index * 8);
                                                            }
                                                            emit_long_constant(m_code, bits);
                                                            emit_integer_canonicalization(integer_type, integer_kind);
                                                            m_code.append< opcode::lcmp >().branch< opcode::ifeq >(matches);
                                                        }
                                                        else if (integer_kind == jvm_value_kind::big_integer)
                                                        {
                                                            emit_big_integer_from_little_endian(m_code, value.second.value);
                                                            emit_integer_canonicalization(integer_type, integer_kind);
                                                            m_code.invokevirtual("java/math/BigInteger", "compareTo", "(Ljava/math/BigInteger;)I").branch< opcode::ifeq >(matches);
                                                        }
                                                        else
                                                        {
                                                            throw compiler_bug("Quxlang's Cortado backend ENUM_INT_INRANGE received a non-integer source");
                                                        }
                                                    }
                                                    m_code.append< opcode::iconst_0 >().branch< opcode::goto_ >(complete).bind(matches).append< opcode::iconst_1 >().bind(complete).istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::enum_cast >)
                                                {
                                                    emit_integer_carrier_conversion(selected.integer, selected.result);
                                                    emit_store(m_code, kind_of(selected.result), jvm_slot(selected.result));
                                                    canonicalize_integer(selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::fusion_active_index >)
                                                {
                                                    type_symbol type = fusion_type_of(selected.subject);
                                                    static_cast< void >(fusion_alternative_types(m_input, type));
                                                    emit_fusion_tag(selected.subject);
                                                    jvm_value_kind kind = kind_of(selected.result);
                                                    if (kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.append< opcode::l2i >();
                                                    }
                                                    else if (kind != jvm_value_kind::long_)
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend fusion active index has a non-integer result");
                                                    }
                                                    emit_store(m_code, kind, jvm_slot(selected.result));
                                                    canonicalize_integer(selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::fusion_has_alternative >)
                                                {
                                                    type_symbol type = fusion_type_of(selected.subject);
                                                    std::vector< type_symbol > alternatives = fusion_alternative_types(m_input, type);
                                                    if (selected.alternative >= alternatives.size())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend fusion alternative ordinal is out of range");
                                                    }
                                                    label const matches = m_code.new_label();
                                                    label const complete = m_code.new_label();
                                                    emit_fusion_tag(selected.subject);
                                                    emit_long_constant(m_code, selected.alternative);
                                                    m_code.append< opcode::lcmp >().branch< opcode::ifeq >(matches).append< opcode::iconst_0 >().branch< opcode::goto_ >(complete).bind(matches).append< opcode::iconst_1 >().bind(complete).istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::fusion_is_valueless >)
                                                {
                                                    type_symbol type = fusion_type_of(selected.subject);
                                                    std::vector< type_symbol > alternatives = fusion_alternative_types(m_input, type);
                                                    label const valueless = m_code.new_label();
                                                    label const complete = m_code.new_label();
                                                    emit_fusion_tag(selected.subject);
                                                    emit_long_constant(m_code, alternatives.size());
                                                    m_code.append< opcode::lcmp >().branch< opcode::ifeq >(valueless).append< opcode::iconst_0 >().branch< opcode::goto_ >(complete).bind(valueless).append< opcode::iconst_1 >().bind(complete).istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::fusion_storage_ref >)
                                                {
                                                    type_symbol type = fusion_type_of(selected.subject);
                                                    std::vector< type_symbol > alternatives = fusion_alternative_types(m_input, type);
                                                    if (selected.alternative >= alternatives.size() || alternatives.at(static_cast< std::size_t >(selected.alternative)).type_is< void_type >())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend cannot project the requested fusion payload storage");
                                                    }
                                                    emit_fusion_payload_storage(selected.subject, fusion_properties_for_type(m_input, type).is_inline);
                                                    m_code.astore(m_reference_owner_slot).new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(m_reference_owner_slot).append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::fusion_set_active >)
                                                {
                                                    type_symbol type = fusion_type_of(selected.target);
                                                    std::vector< type_symbol > alternatives = fusion_alternative_types(m_input, type);
                                                    if (selected.alternative >= alternatives.size())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend fusion alternative ordinal is out of range");
                                                    }
                                                    fusion_properties const& properties = fusion_properties_for_type(m_input, type);
                                                    type_symbol const& alternative_type = alternatives.at(static_cast< std::size_t >(selected.alternative));
                                                    if (properties.is_inline)
                                                    {
                                                        if (selected.payload_storage.has_value())
                                                        {
                                                            throw compiler_bug("Quxlang's Cortado backend received external payload storage for an inline fusion");
                                                        }
                                                    }
                                                    else
                                                    {
                                                        emit_composite_object(selected.target);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >();
                                                        if (alternative_type.type_is< void_type >())
                                                        {
                                                            if (selected.payload_storage.has_value())
                                                            {
                                                                throw compiler_bug("Quxlang's Cortado backend received payload storage for a boxed VOID fusion alternative");
                                                            }
                                                            m_code.append< opcode::aconst_null >();
                                                        }
                                                        else
                                                        {
                                                            if (!selected.payload_storage.has_value())
                                                            {
                                                                throw compiler_bug("Quxlang's Cortado backend received a boxed fusion alternative without payload storage");
                                                            }
                                                            emit_storage_owner(*selected.payload_storage);
                                                        }
                                                        m_code.append< opcode::aastore >();
                                                    }
                                                    emit_fusion_tag_store(selected.target, selected.alternative);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::fusion_set_valueless >)
                                                {
                                                    type_symbol type = fusion_type_of(selected.target);
                                                    std::vector< type_symbol > alternatives = fusion_alternative_types(m_input, type);
                                                    fusion_properties const& properties = fusion_properties_for_type(m_input, type);
                                                    if (properties.never_valueless)
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend received FUSION_SET_VALUELESS for a NEVER_VALUELESS fusion");
                                                    }
                                                    emit_fusion_tag_store(selected.target, alternatives.size());
                                                    if (!properties.is_inline)
                                                    {
                                                        emit_composite_object(selected.target);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_1 >().append< opcode::aconst_null >().append< opcode::aastore >();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::fusion_swap_boxed_state >)
                                                {
                                                    emit_fusion_swap_boxed_state(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::storage_init >)
                                                {
                                                    type_symbol const storage_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.storage)).type);
                                                    if (!storage_type.type_is< storage >())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend STORAGE_INIT received a non-TYPED_STORAGE local on a layoutless target");
                                                    }
                                                    if (!m_input.storage_definitions.contains(storage_type))
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend STORAGE_INIT is missing its semantic TYPED_STORAGE definition");
                                                    }
                                                    std::string const class_name = typed_storage_class_name(storage_type);
                                                    m_code.new_(class_name).append< opcode::dup >().invokespecial(class_name, "<init>", "()V").astore(jvm_slot(selected.storage));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::make_reference >)
                                                {
                                                    emit_call_argument(selected.reference_index);
                                                    m_code.astore(jvm_slot(selected.reference_index));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::storage_init_start >)
                                                {
                                                    type_symbol const target_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.target_value)).type);
                                                    if (m_input.struct_definitions.contains(target_type) || target_type.type_is< array_type >() || is_semantic_fusion_type(m_input, target_type))
                                                    {
                                                        emit_new_composite_object(target_type);
                                                        m_code.astore(jvm_slot(selected.target_value));
                                                    }
                                                    emit_storage_owner(selected.on_storage);
                                                    m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                    emit_storage_index(selected.on_storage);
                                                    emit_boxed_value(selected.target_value);
                                                    m_code.append< opcode::aastore >();
                                                    emit_storage_owner(selected.on_storage);
                                                    m_code.getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                                                    emit_storage_index(selected.on_storage);
                                                    m_code.append< opcode::iconst_0 >().append< opcode::bastore >();
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::storage_deinit_start >)
                                                {
                                                    label const initialized = m_code.new_label();
                                                    emit_storage_owner(selected.on_storage);
                                                    m_code.getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                                                    emit_storage_index(selected.on_storage);
                                                    m_code.append< opcode::baload >().branch< opcode::ifne >(initialized);
                                                    emit_runtime_exception(m_code, "Quxlang destruction of uninitialized object storage");
                                                    m_code.bind(initialized);
                                                    emit_storage_owner(selected.on_storage);
                                                    m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                    emit_storage_index(selected.on_storage);
                                                    m_code.append< opcode::aaload >();
                                                    jvm_value_kind const target_kind = kind_of(selected.target_value);
                                                    emit_unboxed_value(target_kind);
                                                    emit_store(m_code, target_kind, jvm_slot(selected.target_value));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::canonicalize_float >)
                                                {
                                                    emit_canonicalize_float(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::newtype > || std::is_same_v< instruction_type, vmir2::struct_init_finish > || std::is_same_v< instruction_type, vmir2::end_lifetime > || std::is_same_v< instruction_type, vmir2::defer_nontrivial_dtor >)
                                                {
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::destroy >)
                                                {
                                                    emit_slot_destructor_call(selected.of, current_state.at(selected.of));
                                                    std::optional< vmir2::local_index > const& storage_reference = m_storage_deinitializers.at(local_slot(selected.of));
                                                    if (storage_reference.has_value())
                                                    {
                                                        emit_storage_owner(*storage_reference);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                        emit_storage_index(*storage_reference);
                                                        m_code.append< opcode::aconst_null >().append< opcode::aastore >();
                                                        emit_storage_owner(*storage_reference);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                                                        emit_storage_index(*storage_reference);
                                                        m_code.append< opcode::iconst_0 >().append< opcode::bastore >();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::struct_init_start >)
                                                {
                                                    std::map< std::string, vmir2::local_index > field_delegates;
                                                    type_symbol target_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.on_value)).type);
                                                    if (target_type.type_is< ptrref_type >())
                                                    {
                                                        target_type = unwrapped_type(target_type.as< ptrref_type >().target);
                                                    }
                                                    std::map< type_symbol, std::vector< struct_field > >::const_iterator const definition = m_input.struct_definitions.find(target_type);
                                                    if (definition == m_input.struct_definitions.end())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend is missing the semantic struct definition for STRUCT_INIT_START on " + to_string(target_type));
                                                    }
                                                    for (vmir2::struct_init_delegate const& delegate : selected.delegates)
                                                    {
                                                        if (!typeis< vmir2::struct_init_field_selector >(delegate.selector))
                                                        {
                                                            throw lowering_compilation_error("Quxlang's Cortado backend does not yet lower inheritance subobject construction");
                                                        }
                                                        std::size_t const field_ordinal = as< vmir2::struct_init_field_selector >(delegate.selector).field_ordinal;
                                                        if (field_ordinal >= definition->second.size())
                                                        {
                                                            throw compiler_bug("STRUCT_INIT_START names an unknown Cortado field ordinal");
                                                        }
                                                        field_delegates.emplace(definition->second.at(field_ordinal).name, delegate.value);
                                                    }
                                                    m_pending_struct_initializers.push_back(pending_struct_initializer{
                                                        .target = selected.on_value,
                                                        .fields = std::move(field_delegates),
                                                    });
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::access_field >)
                                                {
                                                    vmir2::local_index const aggregate = resolved_reference(selected.base_index);
                                                    std::size_t const field_index = aggregate_field_index(aggregate, selected.field_name);
                                                    type_symbol const field_type = aggregate_field_type(aggregate, selected.field_name);
                                                    type_symbol const access_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.store_index)).type);
                                                    if (field_type.type_is< ptrref_type >() && access_type == field_type)
                                                    {
                                                        emit_composite_object(aggregate);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                        emit_int_constant(m_code, static_cast< std::uint32_t >(field_index));
                                                        m_code.append< opcode::aaload >().checkcast("quxlang/runtime/QuxlangReference").astore(jvm_slot(selected.store_index));
                                                    }
                                                    else
                                                    {
                                                        m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >();
                                                        emit_composite_object(aggregate);
                                                        emit_long_constant(m_code, field_index);
                                                        m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.store_index));
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::access_array >)
                                                {
                                                    vmir2::local_index const array = resolved_reference(selected.base_index);
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >();
                                                    emit_composite_object(array);
                                                    emit_integer_as_long(selected.index_index);
                                                    m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.store_index));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::access_pointer >)
                                                {
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(jvm_slot(selected.base_index)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").aload(jvm_slot(selected.base_index)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J");
                                                    emit_integer_as_long(selected.index_index);
                                                    m_code.append< opcode::ladd >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.store_index));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::copy_reference >)
                                                {
                                                    if (!m_reference_aliases.at(local_slot(selected.to_index)).has_value())
                                                    {
                                                        emit_call_argument(selected.from_index);
                                                        emit_store(m_code, kind_of(selected.to_index), jvm_slot(selected.to_index));
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::cast_ptrref >)
                                                {
                                                    if (!m_reference_aliases.at(local_slot(selected.target_index)).has_value())
                                                    {
                                                        emit_call_argument(selected.source_index);
                                                        type_symbol const target_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.target_index)).type);
                                                        if (target_type.type_is< ptrref_type >() && target_type.get_as< ptrref_type >().ptr_class == pointer_class::gc)
                                                        {
                                                            ptrref_type const& pointer = target_type.get_as< ptrref_type >();
                                                            std::map< type_symbol, jvm_external_type_info >::const_iterator const external = m_input.external_types.find(pointer.target);
                                                            if (external == m_input.external_types.end())
                                                            {
                                                                throw compiler_bug("Quxlang's Cortado backend GC-pointer cast target is absent from the compilation packet");
                                                            }
                                                            m_code.checkcast(external->second.internal_name);
                                                        }
                                                        emit_store(m_code, kind_of(selected.target_index), jvm_slot(selected.target_index));
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::load_from_ref >)
                                                {
                                                    if (selected.access_mode != atomic_access_mode::nonatomic)
                                                    {
                                                        type_symbol const reference_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.from_reference)).type);
                                                        if (!reference_type.type_is< ptrref_type >() || reference_type.get_as< ptrref_type >().target != type_symbol(initguard_type{}) || kind_of(selected.to_value) != jvm_value_kind::long_ || m_reference_aliases.at(local_slot(selected.from_reference)).has_value())
                                                        {
                                                            throw rpnx::unimplemented();
                                                        }
                                                        emit_call_argument(selected.from_reference);
                                                        m_code.checkcast("quxlang/runtime/QuxlangReference").invokestatic("quxlang/runtime/JavaInterop", "atomicLoadLong", "(Lquxlang/runtime/QuxlangReference;)J").lstore(jvm_slot(selected.to_value));
                                                        return;
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.from_reference)).has_value())
                                                    {
                                                        copy_local(resolved_reference(selected.from_reference), selected.to_value);
                                                    }
                                                    else
                                                    {
                                                        load_managed_reference(selected.from_reference, selected.to_value);
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::store_to_ref >)
                                                {
                                                    if (selected.access_mode != atomic_access_mode::nonatomic)
                                                    {
                                                        type_symbol const reference_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.to_reference)).type);
                                                        if (!reference_type.type_is< ptrref_type >() || reference_type.get_as< ptrref_type >().target != type_symbol(initguard_type{}) || kind_of(selected.from_value) != jvm_value_kind::long_ || m_reference_aliases.at(local_slot(selected.to_reference)).has_value())
                                                        {
                                                            throw rpnx::unimplemented();
                                                        }
                                                        emit_call_argument(selected.to_reference);
                                                        m_code.checkcast("quxlang/runtime/QuxlangReference");
                                                        emit_call_argument(selected.from_value);
                                                        m_code.invokestatic("quxlang/runtime/JavaInterop", "atomicStoreLong", "(Lquxlang/runtime/QuxlangReference;J)V");
                                                        return;
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.to_reference)).has_value())
                                                    {
                                                        vmir2::local_index const target = resolved_reference(selected.to_reference);
                                                        copy_local(selected.from_value, target);
                                                        if (m_scalar_reference_owner_slots.at(local_slot(target)).has_value())
                                                        {
                                                            emit_scalar_reference_owner_value(target);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        store_managed_reference(selected.to_reference, selected.from_value);
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::dereference_pointer >)
                                                {
                                                    type_symbol const pointer_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.from_pointer)).type);
                                                    if (pointer_targets_storage(selected.from_pointer) && pointer_type.as< ptrref_type >().ptr_class == pointer_class::instance)
                                                    {
                                                        label const recovered_pointer = m_code.new_label();
                                                        label const reference_ready = m_code.new_label();
                                                        m_code.aload(jvm_slot(selected.from_pointer)).instanceof_("quxlang/runtime/QuxlangReference").template branch< opcode::ifne >(recovered_pointer);
                                                        m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(jvm_slot(selected.from_pointer)).checkcast("quxlang/runtime/QuxlangObject");
                                                        emit_long_constant(m_code, std::numeric_limits< std::uint64_t >::max());
                                                        m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").template branch< opcode::goto_ >(reference_ready);
                                                        m_code.bind(recovered_pointer).aload(jvm_slot(selected.from_pointer)).checkcast("quxlang/runtime/QuxlangReference").bind(reference_ready).astore(jvm_slot(selected.to_reference));
                                                    }
                                                    else
                                                    {
                                                        copy_local(selected.from_pointer, selected.to_reference);
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::storage_pun >)
                                                {
                                                    vmir2::local_index const storage_local = resolved_reference(selected.from_storage);
                                                    std::optional< vmir2::local_index > const initializer = m_storage_initializers.at(local_slot(storage_local));
                                                    if (initializer.has_value())
                                                    {
                                                        emit_storage_owner(selected.from_storage);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                        emit_storage_index(selected.from_storage);
                                                        emit_boxed_value(*initializer);
                                                        m_code.append< opcode::aastore >();

                                                        emit_storage_owner(selected.from_storage);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                                                        emit_storage_index(selected.from_storage);
                                                        m_code.append< opcode::iconst_1 >().append< opcode::bastore >();
                                                    }

                                                    type_symbol const stored_type = unwrapped_type(selected.as_type);
                                                    bool const stored_composite = m_input.struct_definitions.contains(stored_type) || stored_type.type_is< array_type >() || is_semantic_fusion_type(m_input, stored_type);
                                                    if (stored_composite)
                                                    {
                                                        emit_storage_owner(selected.from_storage);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                        emit_storage_index(selected.from_storage);
                                                        m_code.append< opcode::aaload >().checkcast("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                                                        emit_storage_owner(selected.from_storage);
                                                        m_code.putfield("quxlang/runtime/QuxlangObject", "underlyingStorageOwner", "Lquxlang/runtime/QuxlangObject;").append< opcode::dup >();
                                                        emit_storage_index(selected.from_storage);
                                                        m_code.append< opcode::i2l >().putfield("quxlang/runtime/QuxlangObject", "underlyingStorageIndex", "J").append< opcode::pop >();
                                                    }
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >();
                                                    if (stored_composite)
                                                    {
                                                        emit_storage_owner(selected.from_storage);
                                                        m_code.getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                        emit_storage_index(selected.from_storage);
                                                        m_code.append< opcode::aaload >().checkcast("quxlang/runtime/QuxlangObject");
                                                        emit_long_constant(m_code, std::numeric_limits< std::uint64_t >::max());
                                                    }
                                                    else
                                                    {
                                                        emit_storage_owner(selected.from_storage);
                                                        emit_storage_index(selected.from_storage);
                                                        m_code.append< opcode::i2l >();
                                                    }
                                                    m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.to_reference));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::get_underyling_storage >)
                                                {
                                                    if (!selected.storage_type.template type_is< storage >())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend GET_UNDERYLING_STORAGE received a non-storage type");
                                                    }
                                                    storage const& designated_storage = selected.storage_type.template as< storage >();
                                                    if (designated_storage.storable_types.size() != 1)
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend GET_UNDERYLING_STORAGE requires one exact storable type");
                                                    }
                                                    type_symbol const& stored_type = *designated_storage.storable_types.begin();
                                                    bool const stored_composite = m_input.struct_definitions.contains(stored_type) || stored_type.type_is< array_type >() || is_semantic_fusion_type(m_input, stored_type);
                                                    if (!stored_composite)
                                                    {
                                                        copy_local(selected.object_pointer, selected.storage_pointer);
                                                    }
                                                    else
                                                    {
                                                        emit_composite_object(selected.object_pointer);
                                                        m_code.astore(m_reference_owner_slot).new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "underlyingStorageOwner", "Lquxlang/runtime/QuxlangObject;").aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "underlyingStorageIndex", "J").invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.storage_pointer));
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::make_pointer_to >)
                                                {
                                                    copy_local(selected.of_index, selected.pointer_index);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::pointer_arith >)
                                                {
                                                    if (selected.multiplier != 1 && selected.multiplier != -1)
                                                    {
                                                        throw rpnx::unimplemented();
                                                    }
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(jvm_slot(selected.from)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").aload(jvm_slot(selected.from)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J");
                                                    emit_integer_as_long(selected.offset);
                                                    if (selected.multiplier == 1)
                                                    {
                                                        m_code.append< opcode::ladd >();
                                                    }
                                                    else
                                                    {
                                                        m_code.append< opcode::lsub >();
                                                    }
                                                    m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::pointer_diff >)
                                                {
                                                    label const matching_owner = m_code.new_label();
                                                    m_code.aload(jvm_slot(selected.from)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").aload(jvm_slot(selected.to)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").template branch< opcode::if_acmpeq >(matching_owner);
                                                    emit_runtime_exception(m_code, "Quxlang pointer difference requires references into the same allocation");
                                                    m_code.bind(matching_owner).aload(jvm_slot(selected.from)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").aload(jvm_slot(selected.to)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").template append< opcode::lsub >();
                                                    jvm_value_kind const result_kind = kind_of(selected.result);
                                                    if (result_kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.append< opcode::l2i >();
                                                    }
                                                    else if (result_kind != jvm_value_kind::long_)
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend pointer difference received a non-integer result local");
                                                    }
                                                    emit_store(m_code, result_kind, jvm_slot(selected.result));
                                                    canonicalize_integer(selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::array_init_start >)
                                                {
                                                    vmir2::local_index const array = resolved_reference(selected.on_value);
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >();
                                                    emit_composite_object(array);
                                                    m_code.append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.initializer));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::array_init_element >)
                                                {
                                                    std::optional< local_variable_index > const& element_reference_slot = m_array_element_reference_slots.at(local_slot(selected.target));
                                                    if (!element_reference_slot.has_value())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend ARRAY_INIT_ELEMENT has no managed-reference JVM local slot");
                                                    }
                                                    m_code.aload(jvm_slot(selected.initializer)).astore(*element_reference_slot);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::array_init_index >)
                                                {
                                                    m_code.aload(jvm_slot(selected.initializer)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J");
                                                    jvm_value_kind const result_kind = kind_of(selected.result);
                                                    if (result_kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.append< opcode::l2i >();
                                                    }
                                                    else if (result_kind != jvm_value_kind::long_)
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend array initializer received a non-integer index result local");
                                                    }
                                                    emit_store(m_code, result_kind, jvm_slot(selected.result));
                                                    canonicalize_integer(selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::array_init_more >)
                                                {
                                                    label const elements_remain = m_code.new_label();
                                                    label const comparison_done = m_code.new_label();
                                                    type_symbol const initializer_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.initializer)).type);
                                                    if (!initializer_type.type_is< array_initializer_type >())
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend ARRAY_INIT_MORE received a non-initializer local");
                                                    }
                                                    m_code.aload(jvm_slot(selected.initializer)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J");
                                                    emit_long_constant(m_code, initializer_type.as< array_initializer_type >().count);
                                                    m_code.append< opcode::lcmp >().branch< opcode::iflt >(elements_remain).append< opcode::iconst_0 >().branch< opcode::goto_ >(comparison_done).bind(elements_remain).append< opcode::iconst_1 >().bind(comparison_done).istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::array_init_finish >)
                                                {
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_int_add > || std::is_same_v< instruction_type, vmir2::mut_int_sub > || std::is_same_v< instruction_type, vmir2::mut_int_mul > || std::is_same_v< instruction_type, vmir2::mut_int_div > || std::is_same_v< instruction_type, vmir2::mut_int_mod >)
                                                {
                                                    if (selected.access_mode != atomic_access_mode::nonatomic)
                                                    {
                                                        throw rpnx::unimplemented();
                                                    }
                                                    bool const local_reference = m_reference_aliases.at(local_slot(selected.target)).has_value();
                                                    if constexpr (std::is_same_v< instruction_type, vmir2::mut_int_add >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::iadd, opcode::ladd >(selected.target, selected.value, selected.old_value);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::iadd, opcode::ladd >(selected.target, selected.value, selected.old_value);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_int_sub >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::isub, opcode::lsub >(selected.target, selected.value, selected.old_value);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::isub, opcode::lsub >(selected.target, selected.value, selected.old_value);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_int_mul >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::imul, opcode::lmul >(selected.target, selected.value, selected.old_value);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::imul, opcode::lmul >(selected.target, selected.value, selected.old_value);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_int_div >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_division(selected.target, selected.value, selected.old_value, false);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_division(selected.target, selected.value, selected.old_value, false);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_division(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_division(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                    }
                                                    if (local_reference)
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_float_add >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_local_mutating_float_binary< opcode::fadd, opcode::dadd >(selected.target, selected.value);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_float_binary< opcode::fadd, opcode::dadd >(selected.target, selected.value);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_float_sub >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_local_mutating_float_binary< opcode::fsub, opcode::dsub >(selected.target, selected.value);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_float_binary< opcode::fsub, opcode::dsub >(selected.target, selected.value);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_float_mul >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_local_mutating_float_binary< opcode::fmul, opcode::dmul >(selected.target, selected.value);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_float_binary< opcode::fmul, opcode::dmul >(selected.target, selected.value);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_float_div >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_local_mutating_float_binary< opcode::fdiv, opcode::ddiv >(selected.target, selected.value);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_float_binary< opcode::fdiv, opcode::ddiv >(selected.target, selected.value);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_and > || std::is_same_v< instruction_type, vmir2::mut_bitwise_or > || std::is_same_v< instruction_type, vmir2::mut_bitwise_xor > || std::is_same_v< instruction_type, vmir2::mut_bitwise_nand > || std::is_same_v< instruction_type, vmir2::mut_bitwise_nor > || std::is_same_v< instruction_type, vmir2::mut_bitwise_nxor > || std::is_same_v< instruction_type, vmir2::mut_bitwise_implies > || std::is_same_v< instruction_type, vmir2::mut_bitwise_implied >)
                                                {
                                                    if (selected.access_mode != atomic_access_mode::nonatomic)
                                                    {
                                                        throw rpnx::unimplemented();
                                                    }
                                                    bool const local_reference = m_reference_aliases.at(local_slot(selected.target)).has_value();
                                                    if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_and >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::iand, opcode::land >(selected.target, selected.value, selected.old_value);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::iand, opcode::land >(selected.target, selected.value, selected.old_value);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_or >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::ior, opcode::lor >(selected.target, selected.value, selected.old_value);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::ior, opcode::lor >(selected.target, selected.value, selected.old_value);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_xor >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::ixor, opcode::lxor >(selected.target, selected.value, selected.old_value);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::ixor, opcode::lxor >(selected.target, selected.value, selected.old_value);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_nand >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::iand, opcode::land >(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::iand, opcode::land >(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_nor >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::ior, opcode::lor >(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::ior, opcode::lor >(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_nxor >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_integer_binary< opcode::ixor, opcode::lxor >(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_integer_binary< opcode::ixor, opcode::lxor >(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_implies >)
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_bitwise_implication(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_bitwise_implication(selected.target, selected.value, selected.old_value, true);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        if (local_reference)
                                                        {
                                                            emit_local_mutating_bitwise_implication(selected.target, selected.value, selected.old_value, false);
                                                        }
                                                        else
                                                        {
                                                            emit_mutating_bitwise_implication(selected.target, selected.value, selected.old_value, false);
                                                        }
                                                    }
                                                    if (local_reference)
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_shift_up >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        vmir2::local_index const target = resolved_reference(selected.target);
                                                        emit_bitwise_shift(target, selected.amount, target, true);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_bitwise_shift(selected.target, selected.amount, true);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_shift_down >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        vmir2::local_index const target = resolved_reference(selected.target);
                                                        emit_bitwise_shift(target, selected.amount, target, false);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_bitwise_shift(selected.target, selected.amount, false);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_rotate_up >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        vmir2::local_index const target = resolved_reference(selected.target);
                                                        emit_bitwise_rotate(target, selected.amount, target, true);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_bitwise_rotate(selected.target, selected.amount, true);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::mut_bitwise_rotate_down >)
                                                {
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        vmir2::local_index const target = resolved_reference(selected.target);
                                                        emit_bitwise_rotate(target, selected.amount, target, false);
                                                    }
                                                    else
                                                    {
                                                        emit_mutating_bitwise_rotate(selected.target, selected.amount, false);
                                                    }
                                                    if (m_reference_aliases.at(local_slot(selected.target)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(resolved_reference(selected.target));
                                                    }
                                                    else
                                                    {
                                                        emit_scalar_reference_results();
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::increment > || std::is_same_v< instruction_type, vmir2::decrement >)
                                                {
                                                    bool constexpr incrementing = std::is_same_v< instruction_type, vmir2::increment >;
                                                    if (m_reference_aliases.at(local_slot(selected.value)).has_value())
                                                    {
                                                        vmir2::local_index const target = resolved_reference(selected.value);
                                                        copy_local(target, selected.result);
                                                        jvm_value_kind const result_kind = kind_of(selected.result);
                                                        if (result_kind == jvm_value_kind::integer)
                                                        {
                                                            m_code.iload(jvm_slot(selected.result)).template append< opcode::iconst_1 >();
                                                            if constexpr (incrementing)
                                                            {
                                                                m_code.append< opcode::iadd >();
                                                            }
                                                            else
                                                            {
                                                                m_code.append< opcode::isub >();
                                                            }
                                                            emit_integer_canonicalization(m_routine.local_types.at(local_slot(target)).type, result_kind);
                                                            m_code.istore(jvm_slot(target));
                                                        }
                                                        else if (result_kind == jvm_value_kind::long_)
                                                        {
                                                            m_code.lload(jvm_slot(selected.result)).template append< opcode::lconst_1 >();
                                                            if constexpr (incrementing)
                                                            {
                                                                m_code.append< opcode::ladd >();
                                                            }
                                                            else
                                                            {
                                                                m_code.append< opcode::lsub >();
                                                            }
                                                            emit_integer_canonicalization(m_routine.local_types.at(local_slot(target)).type, result_kind);
                                                            m_code.lstore(jvm_slot(target));
                                                        }
                                                        else if (result_kind == jvm_value_kind::big_integer)
                                                        {
                                                            m_code.aload(jvm_slot(selected.result)).getstatic("java/math/BigInteger", "ONE", "Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", incrementing ? "add" : "subtract", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                                                            emit_integer_canonicalization(m_routine.local_types.at(local_slot(target)).type, result_kind);
                                                            m_code.astore(jvm_slot(target));
                                                        }
                                                        else if (result_kind == jvm_value_kind::reference)
                                                        {
                                                            m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(jvm_slot(selected.result)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").aload(jvm_slot(selected.result)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").template append< opcode::lconst_1 >();
                                                            if constexpr (incrementing)
                                                            {
                                                                m_code.append< opcode::ladd >();
                                                            }
                                                            else
                                                            {
                                                                m_code.append< opcode::lsub >();
                                                            }
                                                            m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(target));
                                                        }
                                                        else
                                                        {
                                                            throw compiler_bug("Quxlang's Cortado backend increment or decrement received a non-integer, non-reference value");
                                                        }
                                                        if (m_scalar_reference_owner_slots.at(local_slot(target)).has_value())
                                                        {
                                                            emit_scalar_reference_owner_value(target);
                                                        }
                                                        return;
                                                    }
                                                    load_managed_reference(selected.value, selected.result);
                                                    emit_managed_reference_owner(selected.value);
                                                    m_code.aload(m_reference_owner_slot).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                                                    emit_managed_reference_index(selected.value);
                                                    jvm_value_kind const result_kind = kind_of(selected.result);
                                                    if (result_kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.iload(jvm_slot(selected.result)).template append< opcode::iconst_1 >();
                                                        if constexpr (incrementing)
                                                        {
                                                            m_code.append< opcode::iadd >();
                                                        }
                                                        else
                                                        {
                                                            m_code.append< opcode::isub >();
                                                        }
                                                        emit_integer_canonicalization(selected.result);
                                                        m_code.invokestatic("java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;");
                                                    }
                                                    else if (result_kind == jvm_value_kind::long_)
                                                    {
                                                        m_code.lload(jvm_slot(selected.result)).template append< opcode::lconst_1 >();
                                                        if constexpr (incrementing)
                                                        {
                                                            m_code.append< opcode::ladd >();
                                                        }
                                                        else
                                                        {
                                                            m_code.append< opcode::lsub >();
                                                        }
                                                        emit_integer_canonicalization(selected.result);
                                                        m_code.invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;");
                                                    }
                                                    else if (result_kind == jvm_value_kind::big_integer)
                                                    {
                                                        m_code.aload(jvm_slot(selected.result)).getstatic("java/math/BigInteger", "ONE", "Ljava/math/BigInteger;").invokevirtual("java/math/BigInteger", incrementing ? "add" : "subtract", "(Ljava/math/BigInteger;)Ljava/math/BigInteger;");
                                                        emit_integer_canonicalization(selected.result);
                                                    }
                                                    else if (result_kind == jvm_value_kind::reference)
                                                    {
                                                        m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload(jvm_slot(selected.result)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").aload(jvm_slot(selected.result)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").template append< opcode::lconst_1 >();
                                                        if constexpr (incrementing)
                                                        {
                                                            m_code.append< opcode::ladd >();
                                                        }
                                                        else
                                                        {
                                                            m_code.append< opcode::lsub >();
                                                        }
                                                        m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V");
                                                    }
                                                    else
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend increment or decrement received a non-integer, non-reference value");
                                                    }
                                                    m_code.append< opcode::aastore >();
                                                    emit_scalar_reference_results();
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::swap >)
                                                {
                                                    emit_swap(selected);
                                                    emit_scalar_reference_results();
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::get_value_byte >)
                                                {
                                                    emit_get_value_byte(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::set_value_byte >)
                                                {
                                                    emit_set_value_byte(selected);
                                                    emit_scalar_reference_results();
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::int_add >)
                                                {
                                                    emit_integer_binary< opcode::iadd, opcode::ladd >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::int_sub >)
                                                {
                                                    emit_integer_binary< opcode::isub, opcode::lsub >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::int_mul >)
                                                {
                                                    emit_integer_binary< opcode::imul, opcode::lmul >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::int_div >)
                                                {
                                                    emit_integer_division(selected.a, selected.b, selected.result, false);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::int_mod >)
                                                {
                                                    emit_integer_division(selected.a, selected.b, selected.result, true);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::float_add >)
                                                {
                                                    emit_float_binary< opcode::fadd, opcode::dadd >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::float_sub >)
                                                {
                                                    emit_float_binary< opcode::fsub, opcode::dsub >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::float_mul >)
                                                {
                                                    emit_float_binary< opcode::fmul, opcode::dmul >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::float_div >)
                                                {
                                                    emit_float_binary< opcode::fdiv, opcode::ddiv >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::float_from_int >)
                                                {
                                                    jvm_value_kind const source_kind = kind_of(selected.source);
                                                    jvm_value_kind const result_kind = kind_of(selected.result);
                                                    if (source_kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.iload(jvm_slot(selected.source));
                                                        if (integer_is_unsigned(selected.source) && integer_bit_width(selected.source) == 32)
                                                        {
                                                            m_code.append< opcode::i2l >();
                                                            emit_long_constant(m_code, std::numeric_limits< std::uint32_t >::max());
                                                            m_code.append< opcode::land >();
                                                            if (result_kind == jvm_value_kind::float_)
                                                            {
                                                                m_code.append< opcode::l2f >();
                                                            }
                                                            else if (result_kind == jvm_value_kind::double_)
                                                            {
                                                                m_code.append< opcode::l2d >();
                                                            }
                                                            else
                                                            {
                                                                throw compiler_bug("Quxlang's Cortado backend ITOF received a non-F32, non-F64 result local");
                                                            }
                                                        }
                                                        else if (result_kind == jvm_value_kind::float_)
                                                        {
                                                            m_code.append< opcode::i2f >();
                                                        }
                                                        else if (result_kind == jvm_value_kind::double_)
                                                        {
                                                            m_code.append< opcode::i2d >();
                                                        }
                                                        else
                                                        {
                                                            throw compiler_bug("Quxlang's Cortado backend ITOF received a non-F32, non-F64 result local");
                                                        }
                                                    }
                                                    else if (source_kind == jvm_value_kind::long_)
                                                    {
                                                        if (integer_is_unsigned(selected.source))
                                                        {
                                                            emit_integer_as_big_integer(selected.source);
                                                            if (result_kind == jvm_value_kind::float_)
                                                            {
                                                                m_code.invokevirtual("java/math/BigInteger", "floatValue", "()F");
                                                            }
                                                            else if (result_kind == jvm_value_kind::double_)
                                                            {
                                                                m_code.invokevirtual("java/math/BigInteger", "doubleValue", "()D");
                                                            }
                                                            else
                                                            {
                                                                throw compiler_bug("Quxlang's Cortado backend ITOF received a non-F32, non-F64 result local");
                                                            }
                                                        }
                                                        else if (result_kind == jvm_value_kind::float_)
                                                        {
                                                            m_code.lload(jvm_slot(selected.source)).template append< opcode::l2f >();
                                                        }
                                                        else if (result_kind == jvm_value_kind::double_)
                                                        {
                                                            m_code.lload(jvm_slot(selected.source)).template append< opcode::l2d >();
                                                        }
                                                        else
                                                        {
                                                            throw compiler_bug("Quxlang's Cortado backend ITOF received a non-F32, non-F64 result local");
                                                        }
                                                    }
                                                    else if (source_kind == jvm_value_kind::big_integer)
                                                    {
                                                        m_code.aload(jvm_slot(selected.source));
                                                        if (result_kind == jvm_value_kind::float_)
                                                        {
                                                            m_code.invokevirtual("java/math/BigInteger", "floatValue", "()F");
                                                        }
                                                        else if (result_kind == jvm_value_kind::double_)
                                                        {
                                                            m_code.invokevirtual("java/math/BigInteger", "doubleValue", "()D");
                                                        }
                                                        else
                                                        {
                                                            throw compiler_bug("Quxlang's Cortado backend ITOF received a non-F32, non-F64 result local");
                                                        }
                                                    }
                                                    else
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend ITOF received a non-integer source");
                                                    }
                                                    emit_store(m_code, result_kind, jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::iconv >)
                                                {
                                                    emit_integer_carrier_conversion(selected.from, selected.to);
                                                    emit_store(m_code, kind_of(selected.to), jvm_slot(selected.to));
                                                    canonicalize_integer(selected.to);
                                                    if (selected.convtype == vmir2::conversion_class::checked)
                                                    {
                                                        emit_checked_integer_conversion_validation(selected.from, selected.to);
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_and >)
                                                {
                                                    emit_integer_binary< opcode::iand, opcode::land >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_or >)
                                                {
                                                    emit_integer_binary< opcode::ior, opcode::lor >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_xor >)
                                                {
                                                    emit_integer_binary< opcode::ixor, opcode::lxor >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_nand >)
                                                {
                                                    emit_inverted_integer_binary< opcode::iand, opcode::land >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_nor >)
                                                {
                                                    emit_inverted_integer_binary< opcode::ior, opcode::lor >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_nxor >)
                                                {
                                                    emit_inverted_integer_binary< opcode::ixor, opcode::lxor >(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_implies > || std::is_same_v< instruction_type, vmir2::bitwise_implied >)
                                                {
                                                    jvm_value_kind const kind = kind_of(selected.result);
                                                    emit_load(m_code, kind, jvm_slot(selected.a));
                                                    if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_implies >)
                                                    {
                                                        emit_integer_inverse(kind);
                                                    }
                                                    emit_load(m_code, kind, jvm_slot(selected.b));
                                                    if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_implied >)
                                                    {
                                                        emit_integer_inverse(kind);
                                                    }
                                                    if (kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.append< opcode::ior >();
                                                    }
                                                    else if (kind == jvm_value_kind::long_)
                                                    {
                                                        m_code.append< opcode::lor >();
                                                    }
                                                    else
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend bitwise implication received a non-integer value");
                                                    }
                                                    emit_store(m_code, kind, jvm_slot(selected.result));
                                                    canonicalize_integer(selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_shift_up >)
                                                {
                                                    emit_bitwise_shift(selected.value, selected.amount, selected.result, true);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_shift_down >)
                                                {
                                                    emit_bitwise_shift(selected.value, selected.amount, selected.result, false);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_rotate_up >)
                                                {
                                                    emit_bitwise_rotate(selected.value, selected.amount, selected.result, true);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_rotate_down >)
                                                {
                                                    emit_bitwise_rotate(selected.value, selected.amount, selected.result, false);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::bitwise_inverse >)
                                                {
                                                    jvm_value_kind const kind = kind_of(selected.result);
                                                    emit_load(m_code, kind, jvm_slot(selected.value));
                                                    emit_integer_inverse(kind);
                                                    emit_store(m_code, kind, jvm_slot(selected.result));
                                                    canonicalize_integer(selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::int_cmp >)
                                                {
                                                    jvm_value_kind const kind = kind_of(selected.a);
                                                    emit_load(m_code, kind, jvm_slot(selected.a));
                                                    emit_load(m_code, kind, jvm_slot(selected.b));
                                                    if (kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.invokestatic("java/lang/Integer", integer_is_unsigned(selected.a) ? "compareUnsigned" : "compare", "(II)I");
                                                    }
                                                    else if (kind == jvm_value_kind::long_)
                                                    {
                                                        m_code.invokestatic("java/lang/Long", integer_is_unsigned(selected.a) ? "compareUnsigned" : "compare", "(JJ)I");
                                                    }
                                                    else if (kind == jvm_value_kind::big_integer)
                                                    {
                                                        m_code.invokevirtual("java/math/BigInteger", "compareTo", "(Ljava/math/BigInteger;)I");
                                                    }
                                                    else
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend integer comparison received a non-integer value");
                                                    }
                                                    m_code.istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::type_index_cmp >)
                                                {
                                                    m_code.lload(jvm_slot(selected.a));
                                                    m_code.lload(jvm_slot(selected.b));
                                                    m_code.invokestatic("java/lang/Long", "compareUnsigned", "(JJ)I");
                                                    m_code.istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::float_cmp >)
                                                {
                                                    jvm_value_kind const kind = kind_of(selected.a);
                                                    emit_load(m_code, kind, jvm_slot(selected.a));
                                                    emit_load(m_code, kind, jvm_slot(selected.b));
                                                    if (kind == jvm_value_kind::float_)
                                                    {
                                                        m_code.invokestatic("java/lang/Float", "compare", "(FF)I");
                                                    }
                                                    else if (kind == jvm_value_kind::double_)
                                                    {
                                                        m_code.invokestatic("java/lang/Double", "compare", "(DD)I");
                                                    }
                                                    else
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend floating comparison received a non-floating value");
                                                    }
                                                    m_code.istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::float_ieee_eq > || std::is_same_v< instruction_type, vmir2::float_ieee_ne > || std::is_same_v< instruction_type, vmir2::float_ieee_lt > || std::is_same_v< instruction_type, vmir2::float_ieee_gt >)
                                                {
                                                    label const comparison_true = m_code.new_label();
                                                    label const comparison_done = m_code.new_label();
                                                    jvm_value_kind const kind = kind_of(selected.a);
                                                    emit_load(m_code, kind, jvm_slot(selected.a));
                                                    emit_load(m_code, kind, jvm_slot(selected.b));
                                                    if (kind == jvm_value_kind::float_)
                                                    {
                                                        if constexpr (std::is_same_v< instruction_type, vmir2::float_ieee_lt >)
                                                        {
                                                            m_code.append< opcode::fcmpg >();
                                                        }
                                                        else
                                                        {
                                                            m_code.append< opcode::fcmpl >();
                                                        }
                                                    }
                                                    else if (kind == jvm_value_kind::double_)
                                                    {
                                                        if constexpr (std::is_same_v< instruction_type, vmir2::float_ieee_lt >)
                                                        {
                                                            m_code.append< opcode::dcmpg >();
                                                        }
                                                        else
                                                        {
                                                            m_code.append< opcode::dcmpl >();
                                                        }
                                                    }
                                                    else
                                                    {
                                                        throw rpnx::unimplemented();
                                                    }
                                                    if constexpr (std::is_same_v< instruction_type, vmir2::float_ieee_eq >)
                                                    {
                                                        m_code.branch< opcode::ifeq >(comparison_true);
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::float_ieee_ne >)
                                                    {
                                                        m_code.branch< opcode::ifne >(comparison_true);
                                                    }
                                                    else if constexpr (std::is_same_v< instruction_type, vmir2::float_ieee_lt >)
                                                    {
                                                        m_code.branch< opcode::iflt >(comparison_true);
                                                    }
                                                    else
                                                    {
                                                        m_code.branch< opcode::ifgt >(comparison_true);
                                                    }
                                                    m_code.append< opcode::iconst_0 >().branch< opcode::goto_ >(comparison_done).bind(comparison_true).append< opcode::iconst_1 >().bind(comparison_done).istore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::cmp_bool >)
                                                {
                                                    emit_comparison_boolean(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::to_bool >)
                                                {
                                                    emit_boolean_from_value(selected.from, selected.to, false);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::to_bool_not >)
                                                {
                                                    emit_boolean_from_value(selected.from, selected.to, true);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::pointer_cmp > || std::is_same_v< instruction_type, vmir2::global_cmp >)
                                                {
                                                    if (local_is_gc_pointer(selected.a) || local_is_gc_pointer(selected.b))
                                                    {
                                                        throw semantic_compilation_error("Quxlang does not define ordering comparisons on JVM GC pointers");
                                                    }
                                                    emit_pointer_comparison(selected.a, selected.b, selected.result);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::pointer_eq > || std::is_same_v< instruction_type, vmir2::global_eq >)
                                                {
                                                    if (local_is_gc_pointer(selected.a) || local_is_gc_pointer(selected.b))
                                                    {
                                                        label const equal = m_code.new_label();
                                                        label const complete = m_code.new_label();
                                                        m_code.aload(jvm_slot(selected.a)).aload(jvm_slot(selected.b)).template branch< opcode::if_acmpeq >(equal);
                                                        m_code.append< opcode::iconst_0 >().branch< opcode::goto_ >(complete);
                                                        m_code.bind(equal).append< opcode::iconst_1 >();
                                                        m_code.bind(complete).istore(jvm_slot(selected.result));
                                                        return;
                                                    }
                                                    emit_pointer_comparison(selected.a, selected.b, selected.result);
                                                    vmir2::cmp_bool relation{
                                                        .ordering = selected.result,
                                                        .relation = vmir2::comparison_relation::equal,
                                                        .result = selected.result,
                                                    };
                                                    emit_comparison_boolean(relation);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::pointer_ne > || std::is_same_v< instruction_type, vmir2::global_ne >)
                                                {
                                                    if (local_is_gc_pointer(selected.a) || local_is_gc_pointer(selected.b))
                                                    {
                                                        label const different = m_code.new_label();
                                                        label const complete = m_code.new_label();
                                                        m_code.aload(jvm_slot(selected.a)).aload(jvm_slot(selected.b)).template branch< opcode::if_acmpne >(different);
                                                        m_code.append< opcode::iconst_0 >().branch< opcode::goto_ >(complete);
                                                        m_code.bind(different).append< opcode::iconst_1 >();
                                                        m_code.bind(complete).istore(jvm_slot(selected.result));
                                                        return;
                                                    }
                                                    emit_pointer_comparison(selected.a, selected.b, selected.result);
                                                    vmir2::cmp_bool relation{
                                                        .ordering = selected.result,
                                                        .relation = vmir2::comparison_relation::not_equal,
                                                        .result = selected.result,
                                                    };
                                                    emit_comparison_boolean(relation);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::invoke >)
                                                {
                                                    emit_direct_invoke(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::invoke_virtual > || std::is_same_v< instruction_type, vmir2::inheritance_cast > || std::is_same_v< instruction_type, vmir2::struct_dynamic_cast > || std::is_same_v< instruction_type, vmir2::struct_type_is > || std::is_same_v< instruction_type, vmir2::struct_alloc_info >)
                                                {
                                                    throw lowering_compilation_error("Quxlang's Cortado backend does not yet lower inheritance runtime operations");
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::invoke_indirect >)
                                                {
                                                    emit_indirect_invoke(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::get_procedure_ptr >)
                                                {
                                                    emit_procedure_value(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::interface_init >)
                                                {
                                                    emit_interface_init(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::interface_invoke >)
                                                {
                                                    emit_interface_invoke(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::interface_is_default >)
                                                {
                                                    emit_interface_is_default(selected);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::constexpr_alloc > || std::is_same_v< instruction_type, vmir2::constexpr_dealloc >)
                                                {
                                                    throw lowering_compilation_error("Quxlang's Cortado backend cannot lower constexpr-only allocation operations in runtime VMIR");
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::constexpr_alloc_multiple > || std::is_same_v< instruction_type, vmir2::constexpr_dealloc_multiple >)
                                                {
                                                    throw lowering_compilation_error("Quxlang's Cortado backend cannot lower constexpr-only multiple-allocation operations in runtime VMIR");
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::compare_exchange >)
                                                {
                                                    type_symbol const target_reference_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.target_reference)).type);
                                                    if (!target_reference_type.type_is< ptrref_type >() || target_reference_type.get_as< ptrref_type >().target != type_symbol(initguard_type{}) || !m_reference_aliases.at(local_slot(selected.expected_reference)).has_value() || kind_of(selected.desired_value) != jvm_value_kind::long_)
                                                    {
                                                        throw rpnx::unimplemented();
                                                    }
                                                    vmir2::local_index const expected = resolved_reference(selected.expected_reference);
                                                    if (kind_of(expected) != jvm_value_kind::long_ || kind_of(selected.result) != jvm_value_kind::integer)
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend received an initialization-guard compare-exchange with an unsupported ABI");
                                                    }
                                                    emit_call_argument(selected.target_reference);
                                                    m_code.checkcast("quxlang/runtime/QuxlangReference").lload(jvm_slot(expected));
                                                    emit_call_argument(selected.desired_value);
                                                    m_code.invokestatic("quxlang/runtime/JavaInterop", "atomicCompareExchangeLong", "(Lquxlang/runtime/QuxlangReference;JJ)J").append< opcode::dup2 >().lload(jvm_slot(expected)).append< opcode::lcmp >();
                                                    label const exchanged = m_code.new_label();
                                                    label const complete = m_code.new_label();
                                                    m_code.branch< opcode::ifeq >(exchanged).lstore(jvm_slot(expected));
                                                    if (m_scalar_reference_owner_slots.at(local_slot(expected)).has_value())
                                                    {
                                                        emit_scalar_reference_owner_value(expected);
                                                    }
                                                    m_code.append< opcode::iconst_0 >().istore(jvm_slot(selected.result)).template branch< opcode::goto_ >(complete);
                                                    m_code.bind(exchanged).template append< opcode::pop2 >().template append< opcode::iconst_1 >().istore(jvm_slot(selected.result));
                                                    m_code.bind(complete);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::jvm_allocate_object_storage >)
                                                {
                                                    if (!m_input.storage_definitions.contains(selected.storage_type))
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend managed allocation is missing its TYPED_STORAGE definition: " + to_string(selected.storage_type));
                                                    }
                                                    std::string const class_name = typed_storage_class_name(selected.storage_type);
                                                    m_code.new_(class_name).append< opcode::dup >().invokespecial(class_name, "<init>", "()V").astore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::jvm_allocate_multiple_object_storage >)
                                                {
                                                    if (!m_input.storage_definitions.contains(selected.storage_type))
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend managed sequence allocation is missing its TYPED_STORAGE definition: " + to_string(selected.storage_type));
                                                    }
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                                                    emit_integer_as_long(selected.count);
                                                    m_code.invokestatic("java/lang/Math", "toIntExact", "(J)I").invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::jvm_deallocate_object_storage >)
                                                {
                                                    if (m_input.options.mode == backend_cortado_mode::address_sanitizer)
                                                    {
                                                        label const valid = m_code.new_label();
                                                        emit_single_storage_allocation(selected.pointer);
                                                        m_code.template append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "deallocated", "Z").template branch< opcode::ifeq >(valid).template append< opcode::pop >();
                                                        emit_runtime_exception(m_code, "Quxlang double deallocation");
                                                        m_code.bind(valid).append< opcode::iconst_1 >().putfield("quxlang/runtime/QuxlangObject", "deallocated", "Z");
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::jvm_deallocate_multiple_object_storage >)
                                                {
                                                    if (m_input.options.mode == backend_cortado_mode::address_sanitizer)
                                                    {
                                                        label const valid = m_code.new_label();
                                                        m_code.aload(jvm_slot(selected.pointer)).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").template append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "deallocated", "Z").template branch< opcode::ifeq >(valid).template append< opcode::pop >();
                                                        emit_runtime_exception(m_code, "Quxlang double deallocation");
                                                        m_code.bind(valid).append< opcode::iconst_1 >().putfield("quxlang/runtime/QuxlangObject", "deallocated", "Z");
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::jvm_gc_pointer_checked_cast >)
                                                {
                                                    type_symbol const target_type = unwrapped_type(m_routine.local_types.at(local_slot(selected.result)).type);
                                                    if (!target_type.type_is< ptrref_type >() || target_type.get_as< ptrref_type >().ptr_class != pointer_class::gc)
                                                    {
                                                        throw compiler_bug("checked JVM GC-pointer cast result is not a GC pointer");
                                                    }
                                                    ptrref_type const& pointer = target_type.get_as< ptrref_type >();
                                                    std::map< type_symbol, jvm_external_type_info >::const_iterator const external = m_input.external_types.find(pointer.target);
                                                    if (external == m_input.external_types.end())
                                                    {
                                                        throw compiler_bug("checked JVM GC-pointer cast target is absent from the compilation packet");
                                                    }
                                                    emit_call_argument(selected.source);
                                                    m_code.checkcast(external->second.internal_name).astore(jvm_slot(selected.result));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::get_object_ref >)
                                                {
                                                    if (!m_input.global_types.contains(selected.symbol))
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend global reference is absent from the aggregated closure: " + to_string(selected.symbol));
                                                    }
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().getstatic("quxlang/runtime/GeneratedGlobals", global_field_name(selected.symbol), "Lquxlang/runtime/QuxlangObject;");
                                                    if (selected.type == vmir2::access_type::storage)
                                                    {
                                                        emit_long_constant(m_code, std::numeric_limits< std::uint64_t >::max());
                                                    }
                                                    else
                                                    {
                                                        m_code.append< opcode::lconst_0 >();
                                                    }
                                                    m_code.invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.target_ref));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::get_antestatal_ref > || std::is_same_v< instruction_type, vmir2::initguard_global_get_ref >)
                                                {
                                                    if (!m_input.global_types.contains(selected.symbol))
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend global reference is absent from the aggregated closure: " + to_string(selected.symbol));
                                                    }
                                                    m_code.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().getstatic("quxlang/runtime/GeneratedGlobals", global_field_name(selected.symbol), "Lquxlang/runtime/QuxlangObject;").template append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V").astore(jvm_slot(selected.target_ref));
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::initguard_complete > || std::is_same_v< instruction_type, vmir2::initguard_abort >)
                                                {
                                                    if constexpr (std::is_same_v< instruction_type, vmir2::initguard_complete >)
                                                    {
                                                        emit_runtime_initguard_call(vmir_runtime_dependency::initguard_complete, selected.lock);
                                                    }
                                                    if constexpr (std::is_same_v< instruction_type, vmir2::initguard_abort >)
                                                    {
                                                        emit_runtime_initguard_call(vmir_runtime_dependency::initguard_abort, selected.lock);
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::thread_destructor_register >)
                                                {
                                                    throw lowering_compilation_error("PER_THREAD destructor registration is not supported by the JVM backend");
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::assert_instr >)
                                                {
                                                    label const valid = m_code.new_label();
                                                    m_code.iload(jvm_slot(selected.condition)).template branch< opcode::ifne >(valid);
                                                    emit_runtime_assertion_failure(selected);
                                                    m_code.bind(valid);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::lowering_error >)
                                                {
                                                    throw lowering_compilation_error(selected.message);
                                                }
                                                else if constexpr (std::is_same_v< instruction_type, vmir2::unimplemented >)
                                                {
                                                    throw rpnx::unimplemented();
                                                }
                                                else
                                                {
                                                    throw rpnx::unimplemented();
                                                }
                                            });
            }

            /** Lowers one VMIR block terminator while preserving empty boundary stacks. */
            void emit_terminator(vmir2::vm_terminator const& terminator, vmir2::state_map const& current_state)
            {
                rpnx::apply_visitor< void >(terminator,
                                            [&](auto& selected) -> void
                                            {
                                                using terminator_type = std::decay_t< decltype(selected) >;
                                                if constexpr (std::is_same_v< terminator_type, vmir2::jump >)
                                                {
                                                    emit_cleanup_edge(current_state, selected.target);
                                                }
                                                else if constexpr (std::is_same_v< terminator_type, vmir2::branch >)
                                                {
                                                    label const true_edge = m_code.new_label();
                                                    label const false_edge = m_code.new_label();
                                                    m_code.iload(jvm_slot(selected.condition)).template branch< opcode::ifne >(true_edge).template branch< opcode::goto_ >(false_edge);
                                                    m_code.bind(true_edge);
                                                    emit_cleanup_edge(current_state, selected.target_true);
                                                    m_code.bind(false_edge);
                                                    emit_cleanup_edge(current_state, selected.target_false);
                                                }
                                                else if constexpr (std::is_same_v< terminator_type, vmir2::tablebranch >)
                                                {
                                                    label const default_edge = m_code.new_label();
                                                    std::vector< label > target_edges;
                                                    target_edges.reserve(selected.targets.size());
                                                    for (std::size_t index = 0; index < selected.targets.size(); ++index)
                                                    {
                                                        target_edges.push_back(m_code.new_label());
                                                    }
                                                    std::optional< label > wide_index_default;
                                                    jvm_value_kind const index_kind = kind_of(selected.index);
                                                    if (index_kind == jvm_value_kind::integer)
                                                    {
                                                        m_code.iload(jvm_slot(selected.index));
                                                    }
                                                    else if (index_kind == jvm_value_kind::long_)
                                                    {
                                                        wide_index_default = m_code.new_label();
                                                        m_code.lload(jvm_slot(selected.index));
                                                        m_code.append< opcode::dup2 >().append< opcode::lconst_0 >().append< opcode::lcmp >().branch< opcode::iflt >(*wide_index_default).append< opcode::dup2 >();
                                                        emit_long_constant(m_code, static_cast< std::uint64_t >(std::numeric_limits< std::int32_t >::max()));
                                                        m_code.append< opcode::lcmp >().branch< opcode::ifgt >(*wide_index_default).append< opcode::l2i >();
                                                    }
                                                    else
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend TABLEBRANCH index is not an integer");
                                                    }
                                                    m_code.tableswitch(default_edge, 0, target_edges);
                                                    if (wide_index_default.has_value())
                                                    {
                                                        m_code.bind(*wide_index_default).append< opcode::pop2 >().branch< opcode::goto_ >(default_edge);
                                                    }
                                                    m_code.bind(default_edge);
                                                    emit_cleanup_edge(current_state, selected.default_target);
                                                    for (std::size_t index = 0; index < selected.targets.size(); ++index)
                                                    {
                                                        m_code.bind(target_edges.at(index));
                                                        emit_cleanup_edge(current_state, selected.targets.at(index));
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< terminator_type, vmir2::runtime_constexpr >)
                                                {
                                                    emit_cleanup_edge(current_state, selected.target_native);
                                                }
                                                else if constexpr (std::is_same_v< terminator_type, vmir2::initguard_try_acquire >)
                                                {
                                                    if (!m_input.global_types.contains(selected.symbol))
                                                    {
                                                        throw compiler_bug("Quxlang's Cortado backend initialization guard references a global outside the aggregated closure: " + to_string(selected.symbol));
                                                    }
                                                    emit_runtime_initguard_reference(selected.symbol);
                                                    m_code.astore(jvm_slot(selected.target_lock));
                                                    emit_runtime_initguard_call(vmir_runtime_dependency::initguard_try_acquire, selected.target_lock);
                                                    label const acquired_edge = m_code.new_label();
                                                    label const initialized_edge = m_code.new_label();
                                                    m_code.template branch< opcode::ifne >(acquired_edge).template branch< opcode::goto_ >(initialized_edge);
                                                    m_code.bind(acquired_edge);
                                                    emit_cleanup_edge(current_state, selected.target_acquired);
                                                    m_code.bind(initialized_edge);
                                                    emit_cleanup_edge(current_state, selected.target_already_initialized);
                                                }
                                                else if constexpr (std::is_same_v< terminator_type, vmir2::ret >)
                                                {
                                                    emit_return_cleanup(current_state);
                                                    std::map< std::string, vmir2::routine_parameter >::const_iterator const return_iter = m_routine.parameters.named.find("RETURN");
                                                    if (return_iter == m_routine.parameters.named.end())
                                                    {
                                                        emit_return(m_code, jvm_value_kind::void_);
                                                    }
                                                    else
                                                    {
                                                        jvm_value_kind const kind = kind_of(return_iter->second.local_index);
                                                        emit_load(m_code, kind, jvm_slot(return_iter->second.local_index));
                                                        emit_return(m_code, kind);
                                                    }
                                                }
                                                else if constexpr (std::is_same_v< terminator_type, vmir2::panic >)
                                                {
                                                    emit_runtime_panic(selected);
                                                }
                                                else
                                                {
                                                    throw rpnx::unimplemented();
                                                }
                                            });
            }

            cortado_compilable_unit const& m_input;
            type_symbol const& m_symbol;
            vmir2::functanoid_routine3 const& m_routine;
            std::map< type_symbol, routine_jvm_info > const& m_routine_infos;
            code_builder m_code;
            std::vector< std::optional< local_variable_index > > m_local_slots;
            std::vector< std::optional< vmir2::local_index > > m_reference_aliases;
            std::vector< std::optional< vmir2::local_index > > m_storage_initializers;
            std::vector< std::optional< vmir2::local_index > > m_storage_deinitializers;
            std::set< vmir2::local_index > m_storage_initializer_targets;
            std::vector< std::optional< local_variable_index > > m_array_element_reference_slots;
            std::vector< std::optional< local_variable_index > > m_scalar_reference_owner_slots;
            std::vector< pending_struct_initializer > m_pending_struct_initializers;
            std::set< vmir2::local_index > m_parameter_locals;
            std::vector< label > m_block_labels;
            local_variable_index m_reference_owner_slot;
            std::optional< local_variable_index > m_swap_a_value_slot;
            std::optional< local_variable_index > m_swap_b_value_slot;
            bool m_contains_swap_instruction = false;
        };

        /** Generates the managed allocation and lifetime-state runtime class. */
        static auto quxlang_object_class(bool address_sanitizer) -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder("quxlang/runtime/QuxlangObject", "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::invokes_special_super;
            static_cast< void >(builder.add_field("nextAllocationId", "Ljava/util/concurrent/atomic/AtomicLong;", rpnx::cortado::field_access_flags::is_private | rpnx::cortado::field_access_flags::is_static | rpnx::cortado::field_access_flags::is_final));
            static_cast< void >(builder.add_field("allocationId", "J", rpnx::cortado::field_access_flags::is_public));
            static_cast< void >(builder.add_field("underlyingStorageOwner", "Lquxlang/runtime/QuxlangObject;", rpnx::cortado::field_access_flags::is_public));
            static_cast< void >(builder.add_field("underlyingStorageIndex", "J", rpnx::cortado::field_access_flags::is_public));
            if (address_sanitizer)
            {
                static_cast< void >(builder.add_field("deallocated", "Z", rpnx::cortado::field_access_flags::is_public));
            }
            static_cast< void >(builder.add_field("initialized", "[Z", rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_final));
            static_cast< void >(builder.add_field("values", "[Ljava/lang/Object;", rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_final));

            code_builder class_initializer;
            class_initializer.new_("java/util/concurrent/atomic/AtomicLong").append< opcode::dup >().invokespecial("java/util/concurrent/atomic/AtomicLong", "<init>", "()V").putstatic("quxlang/runtime/QuxlangObject", "nextAllocationId", "Ljava/util/concurrent/atomic/AtomicLong;").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<clinit>", "()V", rpnx::cortado::method_access_flags::is_static, class_initializer));

            code_builder acquire_allocation_id;
            label const retry_allocation_id = acquire_allocation_id.new_label();
            label const allocation_id_available = acquire_allocation_id.new_label();
            acquire_allocation_id
                    .branch< opcode::goto_ >(retry_allocation_id)
                    .bind(retry_allocation_id)
                    .getstatic("quxlang/runtime/QuxlangObject", "nextAllocationId", "Ljava/util/concurrent/atomic/AtomicLong;")
                    .invokevirtual("java/util/concurrent/atomic/AtomicLong", "get", "()J")
                    .lstore({0})
                    .lload({0})
                    .append< opcode::lconst_1 >()
                    .append< opcode::ladd >()
                    .lstore({2})
                    .lload({2})
                    .append< opcode::lconst_0 >()
                    .append< opcode::lcmp >()
                    .branch< opcode::ifne >(allocation_id_available)
                    .new_("java/lang/OutOfMemoryError")
                    .append< opcode::dup >()
                    .invokespecial("java/lang/OutOfMemoryError", "<init>", "()V")
                    .append< opcode::athrow >()
                    .bind(allocation_id_available)
                    .getstatic("quxlang/runtime/QuxlangObject", "nextAllocationId", "Ljava/util/concurrent/atomic/AtomicLong;")
                    .lload({0})
                    .lload({2})
                    .invokevirtual("java/util/concurrent/atomic/AtomicLong", "compareAndSet", "(JJ)Z")
                    .branch< opcode::ifeq >(retry_allocation_id)
                    .lload({2})
                    .append< opcode::lreturn >();
            rpnx::cortado::method_access_flags const allocation_id_access = rpnx::cortado::method_access_flags::is_private | rpnx::cortado::method_access_flags::is_static;
            jvm_class_hierarchy hierarchy;
            static_cast< void >(builder.add_method("acquireAllocationId", "()J", allocation_id_access, acquire_allocation_id, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));

            code_builder constructor;
            constructor.aload({0}).invokespecial("java/lang/Object", "<init>", "()V").aload({0}).invokestatic("quxlang/runtime/QuxlangObject", "acquireAllocationId", "()J").putfield("quxlang/runtime/QuxlangObject", "allocationId", "J").aload({0}).iload({1}).anewarray("java/lang/Object").putfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").aload({0}).iload({1}).append(newarray_instruction{.element_type = newarray_type::boolean}).putfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "(I)V", rpnx::cortado::method_access_flags::is_public, constructor));

            code_builder default_constructor;
            default_constructor.aload({0}).append< opcode::iconst_1 >().invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "()V", rpnx::cortado::method_access_flags::is_public, default_constructor));
            return builder.build();
        }

        /** Generates Java interop operations shared by emitted Quxlang routines. */
        static auto java_interop_class() -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder("quxlang/runtime/JavaInterop", "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;

            code_builder atomic_load_long;
            atomic_load_long.aload({0}).getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").astore({1}).aload({0}).getfield("quxlang/runtime/QuxlangReference", "index", "J").invokestatic("java/lang/Math", "toIntExact", "(J)I").istore({2}).aload({1}).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").iload({2}).append< opcode::aaload >().checkcast("java/lang/Long").invokevirtual("java/lang/Long", "longValue", "()J").append< opcode::lreturn >();

            code_builder atomic_store_long;
            atomic_store_long.aload({0}).getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").astore({3}).aload({0}).getfield("quxlang/runtime/QuxlangReference", "index", "J").invokestatic("java/lang/Math", "toIntExact", "(J)I").istore({4}).aload({3}).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").iload({4}).lload({1}).invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;").append< opcode::aastore >().aload({3}).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").iload({4}).append< opcode::iconst_1 >().append< opcode::bastore >().append< opcode::return_ >();

            code_builder atomic_compare_exchange_long;
            label const compare_exchange_complete = atomic_compare_exchange_long.new_label();
            atomic_compare_exchange_long.aload({0}).getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").astore({5}).aload({0}).getfield("quxlang/runtime/QuxlangReference", "index", "J").invokestatic("java/lang/Math", "toIntExact", "(J)I").istore({6}).aload({5}).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").iload({6}).append< opcode::aaload >().checkcast("java/lang/Long").invokevirtual("java/lang/Long", "longValue", "()J").lstore({7}).lload({7}).lload({1}).append< opcode::lcmp >().branch< opcode::ifne >(compare_exchange_complete).aload({5}).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").iload({6}).lload({3}).invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;").append< opcode::aastore >().aload({5}).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").iload({6}).append< opcode::iconst_1 >().append< opcode::bastore >().bind(compare_exchange_complete).lload({7}).append< opcode::lreturn >();

            jvm_class_hierarchy hierarchy;
            rpnx::cortado::method_access_flags const public_static = rpnx::cortado::method_access_flags::is_public | rpnx::cortado::method_access_flags::is_static;
            rpnx::cortado::method_access_flags const public_static_synchronized = public_static | rpnx::cortado::method_access_flags::is_synchronized;
            static_cast< void >(builder.add_method("atomicLoadLong", "(Lquxlang/runtime/QuxlangReference;)J", public_static_synchronized, atomic_load_long, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
            static_cast< void >(builder.add_method("atomicStoreLong", "(Lquxlang/runtime/QuxlangReference;J)V", public_static_synchronized, atomic_store_long, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
            static_cast< void >(builder.add_method("atomicCompareExchangeLong", "(Lquxlang/runtime/QuxlangReference;JJ)J", public_static_synchronized, atomic_compare_exchange_long, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
            return builder.build();
        }

        /** Generates deterministic managed storage for reached globals and snapshots. */
        static auto globals_class(cortado_compilable_unit const& input, std::map< type_symbol, routine_jvm_info > const& routine_infos) -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder("quxlang/runtime/GeneratedGlobals", "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;

            std::vector< std::string > unit_test_name_factory_methods;
            unit_test_name_factory_methods.reserve(input.unit_tests.size());
            for (std::size_t index = 0; index < input.unit_tests.size(); ++index)
            {
                std::string method_name = "unitTestName" + std::to_string(index);
                code_builder factory;
                std::string const& name = input.unit_tests.at(index).name;
                std::span< std::byte const > const bytes(reinterpret_cast< std::byte const* >(name.data()), name.size());
                emit_readonly_byte_span_object(factory, bytes, {0});
                factory.append< opcode::areturn >();
                jvm_class_hierarchy hierarchy;
                static_cast< void >(builder.add_method(method_name, "()Lquxlang/runtime/QuxlangObject;", rpnx::cortado::method_access_flags::is_private | rpnx::cortado::method_access_flags::is_static, factory, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
                unit_test_name_factory_methods.push_back(std::move(method_name));
            }

            std::set< std::string > field_names;
            code_builder initializer;
            for (std::pair< type_symbol const, type_symbol > const& global : input.global_types)
            {
                std::string field_name = global_field_name(global.first);
                if (!field_names.insert(field_name).second)
                {
                    throw compiler_bug("Quxlang's Cortado backend generated a global field-name collision");
                }
                static_cast< void >(builder.add_field(field_name, "Lquxlang/runtime/QuxlangObject;", rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_static | rpnx::cortado::field_access_flags::is_final));

                initializer.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >().append< opcode::iconst_1 >().invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V");
                std::map< type_symbol, antestatal_value >::const_iterator const value = input.global_values.find(global.first);
                if (value != input.global_values.end())
                {
                    initializer.append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >();
                    emit_antestatal_storage_shape(initializer, input, global.second, value->second);
                    initializer.append< opcode::aastore >();
                }
                initializer.putstatic("quxlang/runtime/GeneratedGlobals", std::move(field_name), "Lquxlang/runtime/QuxlangObject;");

                if (value == input.global_values.end())
                {
                    std::string initialization_field_name = global_initialization_field_name(global.first);
                    if (!field_names.insert(initialization_field_name).second)
                    {
                        throw compiler_bug("Quxlang's Cortado backend generated a global initialization field-name collision");
                    }
                    static_cast< void >(builder.add_field(initialization_field_name, "Lquxlang/runtime/QuxlangObject;", rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_static | rpnx::cortado::field_access_flags::is_final));
                }
            }

            for (std::pair< type_symbol const, type_symbol > const& global : input.global_types)
            {
                std::map< type_symbol, antestatal_value >::const_iterator const value = input.global_values.find(global.first);
                if (value != input.global_values.end())
                {
                    std::vector< std::uint64_t > cell_path{0};
                    emit_antestatal_value_fill(initializer, input, routine_infos, global.first, cell_path, global.second, value->second);
                    continue;
                }

                std::optional< std::string > compiler_builtin_name;
                if (global.first.type_is< builtin_symbol >())
                {
                    compiler_builtin_name = global.first.get_as< builtin_symbol >().name;
                }
                bool const has_compiler_value = compiler_builtin_name == "ACTIVE_STEPPING" || compiler_builtin_name == "STEPPING_COUNT" || compiler_builtin_name == "UNIT_TEST_COUNT" || compiler_builtin_name == "UNIT_TEST_NAMES" || compiler_builtin_name == "UNIT_TEST_PROC";
                initializer.getstatic("quxlang/runtime/GeneratedGlobals", global_field_name(global.first), "Lquxlang/runtime/QuxlangObject;").getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >();
                if (compiler_builtin_name == "ACTIVE_STEPPING")
                {
                    emit_long_constant(initializer, 0);
                    emit_boxed_stack_value(initializer, jvm_value_kind::long_);
                }
                else if (compiler_builtin_name == "STEPPING_COUNT")
                {
                    emit_long_constant(initializer, 1);
                    emit_boxed_stack_value(initializer, jvm_value_kind::long_);
                }
                else if (compiler_builtin_name == "UNIT_TEST_COUNT")
                {
                    emit_long_constant(initializer, input.unit_tests.size());
                    emit_boxed_stack_value(initializer, jvm_value_kind::long_);
                }
                else if (compiler_builtin_name == "UNIT_TEST_NAMES")
                {
                    initializer.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                    emit_int_constant(initializer, static_cast< std::uint32_t >(input.unit_tests.size()));
                    initializer.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").astore({0});
                    for (std::size_t index = 0; index < input.unit_tests.size(); ++index)
                    {
                        initializer.aload({0}).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                        emit_int_constant(initializer, static_cast< std::uint32_t >(index));
                        initializer.invokestatic("quxlang/runtime/GeneratedGlobals", unit_test_name_factory_methods.at(index), "()Lquxlang/runtime/QuxlangObject;").append< opcode::aastore >().aload({0}).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                        emit_int_constant(initializer, static_cast< std::uint32_t >(index));
                        initializer.append< opcode::iconst_1 >().append< opcode::bastore >();
                    }
                    initializer.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload({0}).append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V");
                }
                else if (compiler_builtin_name == "UNIT_TEST_PROC")
                {
                    initializer.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >();
                    emit_int_constant(initializer, static_cast< std::uint32_t >(input.unit_tests.size()));
                    initializer.invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").astore({0});
                    for (std::size_t index = 0; index < input.unit_tests.size(); ++index)
                    {
                        type_symbol const& procedure = input.unit_tests.at(index).procedure_symbol;
                        if (!routine_infos.contains(procedure))
                        {
                            throw compiler_bug("Quxlang's Cortado backend UNIT_TEST_PROC target has no generated routine information");
                        }
                        std::string const adapter_name = callable_adapter_class_name(procedure);
                        initializer.aload({0}).getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;");
                        emit_int_constant(initializer, static_cast< std::uint32_t >(index));
                        initializer.new_(adapter_name).append< opcode::dup >().invokespecial(adapter_name, "<init>", "()V").append< opcode::aastore >().aload({0}).getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z");
                        emit_int_constant(initializer, static_cast< std::uint32_t >(index));
                        initializer.append< opcode::iconst_1 >().append< opcode::bastore >();
                    }
                    initializer.new_("quxlang/runtime/QuxlangReference").append< opcode::dup >().aload({0}).append< opcode::lconst_0 >().invokespecial("quxlang/runtime/QuxlangReference", "<init>", "(Lquxlang/runtime/QuxlangObject;J)V");
                }
                else
                {
                    emit_boxed_default_value(initializer, value_kind(input, global.second));
                }
                initializer.append< opcode::aastore >().getstatic("quxlang/runtime/GeneratedGlobals", global_field_name(global.first), "Lquxlang/runtime/QuxlangObject;").getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::iconst_0 >().append< opcode::iconst_1 >().append< opcode::bastore >();
                initializer.new_("quxlang/runtime/QuxlangObject").append< opcode::dup >().append< opcode::iconst_1 >().invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "values", "[Ljava/lang/Object;").append< opcode::iconst_0 >();
                emit_long_constant(initializer, has_compiler_value ? 2 : 0);
                initializer.invokestatic("java/lang/Long", "valueOf", "(J)Ljava/lang/Long;").append< opcode::aastore >().append< opcode::dup >().getfield("quxlang/runtime/QuxlangObject", "initialized", "[Z").append< opcode::iconst_0 >().append< opcode::iconst_1 >().append< opcode::bastore >().putstatic("quxlang/runtime/GeneratedGlobals", global_initialization_field_name(global.first), "Lquxlang/runtime/QuxlangObject;");
            }
            initializer.append< opcode::return_ >();
            jvm_class_hierarchy hierarchy;
            static_cast< void >(builder.add_method("<clinit>", "()V", rpnx::cortado::method_access_flags::is_static, initializer, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
            return builder.build();
        }

        /** Generates a managed storage class retaining declared Quxlang alternatives. */
        static auto typed_storage_class(type_symbol const& storage_type) -> rpnx::cortado::class_file
        {
            std::string class_name = typed_storage_class_name(storage_type);
            rpnx::cortado::class_file_builder builder(class_name, "quxlang/runtime/QuxlangObject", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;
            static_cast< void >(builder.add_field("declaredAlternatives", "[Ljava/lang/String;", rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_static | rpnx::cortado::field_access_flags::is_final));

            storage const& declared_storage = storage_type.as< storage >();
            code_builder class_initializer;
            emit_int_constant(class_initializer, static_cast< std::uint32_t >(declared_storage.storable_types.size()));
            class_initializer.anewarray("java/lang/String");
            std::uint32_t alternative_index = 0;
            for (type_symbol const& alternative : declared_storage.storable_types)
            {
                class_initializer.append< opcode::dup >();
                emit_int_constant(class_initializer, alternative_index);
                class_initializer.ldc_string(to_string(alternative));
                class_initializer.append< opcode::aastore >();
                ++alternative_index;
            }
            class_initializer.putstatic(class_name, "declaredAlternatives", "[Ljava/lang/String;").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<clinit>", "()V", rpnx::cortado::method_access_flags::is_static, class_initializer));

            code_builder constructor;
            constructor.aload({0}).append< opcode::iconst_1 >().invokespecial("quxlang/runtime/QuxlangObject", "<init>", "(I)V").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "()V", rpnx::cortado::method_access_flags::is_public, constructor));
            return builder.build();
        }

        /** Generates the owner-and-logical-index Quxlang reference runtime class. */
        static auto quxlang_reference_class() -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder("quxlang/runtime/QuxlangReference", "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;
            static_cast< void >(builder.add_field("owner", "Lquxlang/runtime/QuxlangObject;", rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_final));
            static_cast< void >(builder.add_field("index", "J", rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_final));

            code_builder constructor;
            constructor.aload({0}).invokespecial("java/lang/Object", "<init>", "()V").aload({0}).aload({1}).putfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").aload({0}).lload({2}).putfield("quxlang/runtime/QuxlangReference", "index", "J").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "(Lquxlang/runtime/QuxlangObject;J)V", rpnx::cortado::method_access_flags::is_public, constructor));

            code_builder compare;
            label const left_null = compare.new_label();
            label const left_reference = compare.new_label();
            label const left_done = compare.new_label();
            label const right_null = compare.new_label();
            label const right_reference = compare.new_label();
            label const right_done = compare.new_label();
            label const compare_indexes = compare.new_label();

            compare.aload({0}).branch< opcode::ifnull >(left_null).aload({0}).instanceof_("quxlang/runtime/QuxlangReference").branch< opcode::ifne >(left_reference).aload({0}).checkcast("quxlang/runtime/QuxlangObject").getfield("quxlang/runtime/QuxlangObject", "allocationId", "J").lstore({2}).append< opcode::lconst_0 >().lstore({4}).branch< opcode::goto_ >(left_done).bind(left_reference).aload({0}).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").getfield("quxlang/runtime/QuxlangObject", "allocationId", "J").lstore({2}).aload({0}).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").lstore({4}).branch< opcode::goto_ >(left_done).bind(left_null).append< opcode::lconst_0 >().lstore({2}).append< opcode::lconst_0 >().lstore({4}).bind(left_done).aload({1}).branch< opcode::ifnull >(right_null).aload({1}).instanceof_("quxlang/runtime/QuxlangReference").branch< opcode::ifne >(right_reference).aload({1}).checkcast("quxlang/runtime/QuxlangObject").getfield("quxlang/runtime/QuxlangObject", "allocationId", "J").lstore({6}).append< opcode::lconst_0 >().lstore({8}).branch< opcode::goto_ >(right_done).bind(right_reference).aload({1}).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "owner", "Lquxlang/runtime/QuxlangObject;").getfield("quxlang/runtime/QuxlangObject", "allocationId", "J").lstore({6}).aload({1}).checkcast("quxlang/runtime/QuxlangReference").getfield("quxlang/runtime/QuxlangReference", "index", "J").lstore({8}).branch< opcode::goto_ >(right_done).bind(right_null).append< opcode::lconst_0 >().lstore({6}).append< opcode::lconst_0 >().lstore({8}).bind(right_done).lload({2}).lload({6}).invokestatic("java/lang/Long", "compareUnsigned", "(JJ)I").append< opcode::dup >().branch< opcode::ifeq >(compare_indexes).append< opcode::ireturn >().bind(compare_indexes).append< opcode::pop >().lload({4}).lload({8}).invokestatic("java/lang/Long", "compareUnsigned", "(JJ)I").append< opcode::ireturn >();
            static_cast< void >(builder.add_method("compare", "(Ljava/lang/Object;Ljava/lang/Object;)I", rpnx::cortado::method_access_flags::is_public | rpnx::cortado::method_access_flags::is_static, compare));
            return builder.build();
        }

        /** Generates an argument frame for a routine exceeding the JVM parameter-slot limit. */
        static auto argument_frame_class(cortado_compilable_unit const& input, vmir2::functanoid_routine3 const& routine, std::string const& class_name) -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder(class_name, "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;

            std::size_t parameter_index = 0;
            auto add_parameter_field = [&](vmir2::routine_parameter const& parameter) -> void
            {
                jvm_value_kind const kind = value_kind(input, routine.local_types.at(local_slot(parameter.local_index)).type);
                static_cast< void >(builder.add_field("p" + std::to_string(parameter_index), descriptor_for_kind(kind), rpnx::cortado::field_access_flags::is_public));
                ++parameter_index;
            };
            for (vmir2::routine_parameter const& parameter : routine.parameters.positional)
            {
                add_parameter_field(parameter);
            }
            for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine.parameters.named)
            {
                if (parameter.first != "RETURN")
                {
                    add_parameter_field(parameter.second);
                }
            }

            code_builder constructor;
            constructor.aload({0}).invokespecial("java/lang/Object", "<init>", "()V").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "()V", rpnx::cortado::method_access_flags::is_public, constructor));
            return builder.build();
        }

        /** Generates the runtime object used to represent a Quxlang interface value. */
        static auto interface_value_class() -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder("quxlang/runtime/QuxlangInterface", "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;
            rpnx::cortado::field_access_flags const public_final = rpnx::cortado::field_access_flags::is_public | rpnx::cortado::field_access_flags::is_final;
            static_cast< void >(builder.add_field("isDefault", "Z", public_final));
            static_cast< void >(builder.add_field("functions", "Ljava/util/HashMap;", public_final));

            code_builder constructor;
            constructor.aload({0}).invokespecial("java/lang/Object", "<init>", "()V");
            constructor.aload({0}).iload({1}).putfield("quxlang/runtime/QuxlangInterface", "isDefault", "Z");
            constructor.aload({0}).new_("java/util/HashMap").append< opcode::dup >().invokespecial("java/util/HashMap", "<init>", "()V").putfield("quxlang/runtime/QuxlangInterface", "functions", "Ljava/util/HashMap;").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "(Z)V", rpnx::cortado::method_access_flags::is_public, constructor));
            return builder.build();
        }

        /** Generates the generic base class used by Quxlang procedure values. */
        static auto callable_base_class() -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder("quxlang/runtime/QuxlangCallable", "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_abstract | rpnx::cortado::class_access_flags::invokes_special_super;
            code_builder constructor;
            constructor.aload({0}).invokespecial("java/lang/Object", "<init>", "()V").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "()V", rpnx::cortado::method_access_flags::is_public, constructor));

            code_builder invoke;
            invoke.new_("java/lang/UnsupportedOperationException").append< opcode::dup >().invokespecial("java/lang/UnsupportedOperationException", "<init>", "()V").append< opcode::athrow >();
            jvm_class_hierarchy hierarchy;
            static_cast< void >(builder.add_method("invoke", "([Ljava/lang/Object;)Ljava/lang/Object;", rpnx::cortado::method_access_flags::is_public, invoke, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
            return builder.build();
        }

        /** Generates a generic-call adapter for one concrete Quxlang routine. */
        static auto callable_adapter_class(cortado_compilable_unit const& input, vmir2::functanoid_routine3 const& routine, routine_jvm_info const& routine_info, std::string const& adapter_name) -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder(adapter_name, "quxlang/runtime/QuxlangCallable", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;

            code_builder constructor;
            constructor.aload({0}).invokespecial("quxlang/runtime/QuxlangCallable", "<init>", "()V").append< opcode::return_ >();
            static_cast< void >(builder.add_method("<init>", "()V", rpnx::cortado::method_access_flags::is_public, constructor));

            code_builder invoke;
            std::size_t parameter_index = 0;
            auto emit_parameter = [&](vmir2::routine_parameter const& parameter) -> void
            {
                jvm_value_kind const kind = value_kind(input, routine.local_types.at(local_slot(parameter.local_index)).type);
                invoke.aload({1});
                emit_int_constant(invoke, static_cast< std::uint32_t >(parameter_index));
                invoke.append< opcode::aaload >();
                emit_unboxed_stack_value(invoke, kind);
                ++parameter_index;
            };

            if (routine_info.argument_frame_class_name.has_value())
            {
                std::string const& frame_name = *routine_info.argument_frame_class_name;
                invoke.new_(frame_name).append< opcode::dup >().invokespecial(frame_name, "<init>", "()V");
                std::size_t frame_field_index = 0;
                auto store_frame_parameter = [&](vmir2::routine_parameter const& parameter) -> void
                {
                    jvm_value_kind const kind = value_kind(input, routine.local_types.at(local_slot(parameter.local_index)).type);
                    invoke.append< opcode::dup >();
                    emit_parameter(parameter);
                    invoke.putfield(frame_name, "p" + std::to_string(frame_field_index), descriptor_for_kind(kind));
                    ++frame_field_index;
                };
                for (vmir2::routine_parameter const& parameter : routine.parameters.positional)
                {
                    store_frame_parameter(parameter);
                }
                for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine.parameters.named)
                {
                    if (parameter.first != "RETURN")
                    {
                        store_frame_parameter(parameter.second);
                    }
                }
            }
            else
            {
                for (vmir2::routine_parameter const& parameter : routine.parameters.positional)
                {
                    emit_parameter(parameter);
                }
                for (std::pair< std::string const, vmir2::routine_parameter > const& parameter : routine.parameters.named)
                {
                    if (parameter.first != "RETURN")
                    {
                        emit_parameter(parameter.second);
                    }
                }
            }

            invoke.invokestatic(routine_info.class_name, "invoke", routine_info.descriptor);
            if (routine_info.return_kind == jvm_value_kind::void_)
            {
                invoke.append< opcode::aconst_null >();
            }
            else
            {
                emit_boxed_stack_value(invoke, routine_info.return_kind);
            }
            invoke.append< opcode::areturn >();
            jvm_class_hierarchy hierarchy;
            static_cast< void >(builder.add_method("invoke", "([Ljava/lang/Object;)Ljava/lang/Object;", rpnx::cortado::method_access_flags::is_public, invoke, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
            return builder.build();
        }

        /** Generates the executable or unit-test Java main class. */
        static auto entrypoint_class(cortado_compilable_unit const& input, std::map< type_symbol, routine_jvm_info > const& routines) -> rpnx::cortado::class_file
        {
            rpnx::cortado::class_file_builder builder("quxlang/runtime/GeneratedMain", "java/lang/Object", {0, 61});
            builder.access_flags() = rpnx::cortado::class_access_flags::is_public | rpnx::cortado::class_access_flags::is_final | rpnx::cortado::class_access_flags::invokes_special_super;
            static_cast< void >(builder.add_field("arguments", "[Ljava/lang/String;", rpnx::cortado::field_access_flags::is_private | rpnx::cortado::field_access_flags::is_static));

            code_builder argument_count;
            argument_count.getstatic("quxlang/runtime/GeneratedMain", "arguments", "[Ljava/lang/String;").append< opcode::arraylength >().append< opcode::ireturn >();
            static_cast< void >(builder.add_method("argumentCount", "()I", rpnx::cortado::method_access_flags::is_public | rpnx::cortado::method_access_flags::is_static, argument_count));

            code_builder argument_at;
            argument_at.getstatic("quxlang/runtime/GeneratedMain", "arguments", "[Ljava/lang/String;").iload({0}).append< opcode::aaload >().append< opcode::areturn >();
            static_cast< void >(builder.add_method("argumentAt", "(I)Ljava/lang/String;", rpnx::cortado::method_access_flags::is_public | rpnx::cortado::method_access_flags::is_static, argument_at));

            code_builder main;
            if (!input.entry_procedure.has_value())
            {
                throw compiler_bug("Quxlang's Cortado backend executable packet has no entry procedure");
            }
            routine_jvm_info const& entry = routines.at(*input.entry_procedure);
            if (entry.descriptor != "()I")
            {
                throw compiler_bug("Quxlang's Cortado backend executable entry has an unexpected JVM descriptor");
            }
            main.aload({0}).append< opcode::arraylength >().append< opcode::iconst_1 >().append< opcode::iadd >().anewarray("java/lang/String").putstatic("quxlang/runtime/GeneratedMain", "arguments", "[Ljava/lang/String;");
            main.getstatic("quxlang/runtime/GeneratedMain", "arguments", "[Ljava/lang/String;").append< opcode::iconst_0 >().ldc_string("quxlang").append< opcode::aastore >();
            main.aload({0}).append< opcode::iconst_0 >().getstatic("quxlang/runtime/GeneratedMain", "arguments", "[Ljava/lang/String;").append< opcode::iconst_1 >().aload({0}).append< opcode::arraylength >().invokestatic("java/lang/System", "arraycopy", "(Ljava/lang/Object;ILjava/lang/Object;II)V");
            main.invokestatic(entry.class_name, "invoke", entry.descriptor).invokestatic("java/lang/System", "exit", "(I)V").append< opcode::return_ >();
            jvm_class_hierarchy hierarchy;
            static_cast< void >(builder.add_method("main", "([Ljava/lang/String;)V", rpnx::cortado::method_access_flags::is_public | rpnx::cortado::method_access_flags::is_static, main, {}, rpnx::cortado::class_hierarchy_resolver_ref(hierarchy)));
            return builder.build();
        }
        /** Generates all classes and serializes the deterministic Cortado JAR. */
        static auto emit(cortado_compilable_unit const& input) -> std::vector< std::byte >
        {
            std::map< type_symbol, routine_jvm_info > routine_infos;
            std::set< std::string > class_names;
            for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine : input.routines)
            {
                std::string class_name = procedure_class_name(routine.first);
                if (!class_names.insert(class_name).second)
                {
                    throw compiler_bug("Quxlang's Cortado backend generated a procedure class-name collision");
                }
                std::optional< std::string > argument_frame_class_name;
                if (routine_parameter_slot_count(input, routine.second) > 255)
                {
                    argument_frame_class_name = class_name + "Arguments";
                }
                routine_infos.emplace(routine.first, routine_jvm_info{
                                                         .class_name = std::move(class_name),
                                                         .descriptor = routine_descriptor(input, routine.second, argument_frame_class_name),
                                                         .argument_frame_class_name = std::move(argument_frame_class_name),
                                                         .return_kind = routine_return_kind(input, routine.second),
                                                     });
            }

            std::map< std::string, std::vector< std::byte > > entries;
            std::string const manifest = "Manifest-Version: 1.0\r\nMain-Class: quxlang.runtime.GeneratedMain\r\n\r\n";
            entries.emplace("META-INF/MANIFEST.MF", bytes_from_string(manifest));
            entries.emplace("quxlang/runtime/QuxlangObject.class", validated_class_bytes(quxlang_object_class(input.options.mode == backend_cortado_mode::address_sanitizer)));
            entries.emplace("quxlang/runtime/JavaInterop.class", validated_class_bytes(java_interop_class()));
            entries.emplace("quxlang/runtime/GeneratedGlobals.class", validated_class_bytes(globals_class(input, routine_infos)));
            entries.emplace("quxlang/runtime/QuxlangReference.class", validated_class_bytes(quxlang_reference_class()));
            entries.emplace("quxlang/runtime/QuxlangInterface.class", validated_class_bytes(interface_value_class()));
            entries.emplace("quxlang/runtime/QuxlangCallable.class", validated_class_bytes(callable_base_class()));
            entries.emplace("quxlang/runtime/GeneratedMain.class", validated_class_bytes(entrypoint_class(input, routine_infos)));
            for (type_symbol const& storage_type : input.storage_definitions)
            {
                std::string class_name = typed_storage_class_name(storage_type);
                entries.emplace(class_name + ".class", validated_class_bytes(typed_storage_class(storage_type)));
            }

            for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine : input.routines)
            {
                try
                {
                    routine_jvm_info const& info = routine_infos.at(routine.first);
                    if (info.argument_frame_class_name.has_value())
                    {
                        entries.emplace(*info.argument_frame_class_name + ".class", validated_class_bytes(argument_frame_class(input, routine.second, *info.argument_frame_class_name)));
                    }
                    std::string const adapter_name = callable_adapter_class_name(routine.first);
                    entries.emplace(adapter_name + ".class", validated_class_bytes(callable_adapter_class(input, routine.second, info, adapter_name)));
                    routine_emitter emitter(input, routine.first, routine.second, routine_infos);
                    entries.emplace(info.class_name + ".class", validated_class_bytes(emitter.emit(info.class_name)));
                }
                catch (compilation_error& error)
                {
                    error.traceback.push_back(trace_frame{
                        .trace_context = "reached routine " + to_string(routine.first),
                        .location = std::nullopt,
                    });
                    throw;
                }
                catch (rpnx::cortado::assembly_error const& error)
                {
                    throw rpnx::cortado::assembly_error(std::string(error.what()) + " while emitting reached routine " + to_string(routine.first));
                }
            }

            rpnx::cortado::jar_file archive;
            for (std::pair< std::string const, std::vector< std::byte > >& entry : entries)
            {
                archive.entries.push_back(rpnx::cortado::jar_entry{
                    .name = entry.first,
                    .data = std::move(entry.second),
                    .compression = rpnx::cortado::compression_method::deflated,
                    .flags = rpnx::cortado::jar_entry_flags::utf8_names,
                    .modification_date = rpnx::cortado::zip_dos_date{0x0021},
                });
            }
            return rpnx::cortado::serialize_jar(archive);
        }
    };

    /** Emits a Cortado JAR and classifies assembly failures by compiler diagnostic category. */
    auto emit_jar(cortado_compilable_unit const& input) -> std::vector< std::byte >
    {
        try
        {
            return cortado_jar_emitter_impl::emit(input);
        }
        catch (rpnx::cortado::assembly_error const& error)
        {
            std::string const message = error.what();
            if (cortado_jar_emitter_impl::assembly_failure_is_classfile_limit(message))
            {
                throw lowering_compilation_error("Quxlang's Cortado backend exceeded a JVM classfile limit: " + message);
            }
            throw compiler_bug("Quxlang's Cortado backend generated inconsistent symbolic JVM bytecode: " + message);
        }
        catch (rpnx::cortado::validation_error const& error)
        {
            throw compiler_bug("Quxlang's Cortado backend generated an invalid JVM classfile: " + std::string(error.what()));
        }
        catch (rpnx::cortado::unsupported_feature_error const& error)
        {
            static_cast< void >(error);
            throw rpnx::unimplemented();
        }
    }
} // namespace quxlang::cortado_backend
