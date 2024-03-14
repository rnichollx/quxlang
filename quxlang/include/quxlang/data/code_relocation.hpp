//
// Created by Ryan Nicholl on 1/27/24.
//

#ifndef CODE_RELOCATION_HPP
#define CODE_RELOCATION_HPP
#include <string>

namespace quxlang
{
    enum class relocation_address_type
    {
        // Stores the absolute address of the symbol
        absolute,

        // Stores the relative address of the symbol
        relative,

        // Stores the relative address of the symbol
        // unsigned (increase only)
        relative_unsigned,


    };

    enum class relocation_target_type
    {
        // Direct: The value stored is a pointer to the target symbol
        address,

        // Pointer: The value stored is an address to a pointer to the target symbol
        pointer_to_address,

        // Value copy: Copy the value of the symbol
        value_copy,

        // Call: The value stored is a pointer to a function,
        // either a direct call or a linker-generated stub that
        // calls the function
        call,

        // Stub: The value stored is a pointer to a function which
        // returns the address of the target symbol
        stub

    };

    enum class relocation_write_method
    {
        // Clears the bits before writing
        set,

        // Adds to the value that was previously there
        add,

        // Subtracts from the value that was previously there
        subtract,
    };

    enum class relocation_bit_ordering
    {
        lsb_to_msb,
        msb_to_lsb
    };

    enum class relocation_byte_ordering
    {
        little_endian,
        big_endian,
    };

    struct symbol_relocation
    {
        // The second to perform a relocation on
        std::string relocation_section;

        // The offset of the relocation within the section
        std::size_t relocation_offset;

        // The name of symbol for the relocation
        std::string target_symbol;

        // The section of the symbol, for relocations in formats where multiple symbols can have the same name
        // if they are in different sections. For other formats, this can be an empty string.
        std::string target_section;

        // Added to the value before it is stored in the relocation
        ssize_t target_offset;

        // A shift operation that is applied to the target address prior to writing the value.
        // Positive values are right shifts, negative values are left shifts
        // Example, if the address is 0xFFA0 and the shift value is 4, the value written will be 0x0FFA
        ssize_t target_bit_shift;

        // The type of address to store (ignored if target_type is value_copy)
        relocation_address_type address_type;

        // The type of relocation being used
        relocation_target_type target_type;

        // How to write the data for the relocation
        relocation_write_method write_method;

        // The number of bits written out for the relocation (occurs after the shift)
        size_t bits_width;


        // This indicates how to order the bytes in the relocation when bits_width is greater than 8
        relocation_byte_ordering byte_ordering;

        // This indicates how to order the bits in the relocation when relocation_bit_offset is non-zero
        relocation_bit_ordering bit_ordering;

        // A bit based offset used to start at a bit other than the least significant bit of the first byte
        // Example, using 4 bits_width, with offset 1, if the data to write at the relocation is 0b0000
        // and the data alreadyat that byte is 0b11111111,  with relocation_bit_ordering::lsb_to_msb,
        // the result would be 0x11100001
        uint8_t relocation_bit_offset;


    };
}

#endif //CODE_RELOCATION_HPP