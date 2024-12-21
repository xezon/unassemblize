/**
 * @file
 *
 * @brief Function types
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "commontypes.h"
#include <array>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace unassemblize
{
constexpr std::string_view s_prefix_sub = "sub_";
constexpr std::string_view s_prefix_off = "off_";
constexpr std::string_view s_prefix_unk = "unk_";
constexpr std::string_view s_prefix_loc = "loc_";
constexpr std::array<std::string_view, 4> s_prefix_array = {s_prefix_sub, s_prefix_off, s_prefix_unk, s_prefix_loc};

// #TODO: implement default value where exe object decides internally what to do.
enum class AsmFormat
{
    IGAS,
    AGAS,
    MASM,

    DEFAULT,
};

AsmFormat to_asm_format(std::string_view str);

/*
 * Intermediate instruction data between Zydis disassemble and final text generation.
 */
struct AsmInstruction
{
    using BytesArray = SizedArray<uint8_t, uint8_t, 11>;

    AsmInstruction()
    {
        address = 0;
        std::fill_n(bytes.elements.data(), bytes.elements.size(), 0);
        isJump = false;
        isJumpedTo = false;
        isInvalid = false;
        isFirstLine = false;
        jumpLen = 0;
        lineNumber = 0;
    }

    void set_bytes(const uint8_t *p, size_t size);
    uint16_t get_line_index() const { return lineNumber - 1; } // Returns ~0 when invalid

    Address64T address; // Position of the instruction within the executable.
    BytesArray bytes;
    bool isJump : 1; // Instruction is a jump.
    bool isJumpedTo : 1; // Instruction is jumped to or called.
    bool isInvalid : 1; // Instruction was not read or formatted correctly.
    bool isFirstLine : 1; // This instruction is the first one that corresponds to its line number.
    union
    {
        int16_t jumpLen; // Jump length in bytes.
    };
    uint16_t lineNumber; // Line number in the source file - if exists.
    std::string text; // Instruction mnemonics and operands with address symbol substitution.
};

struct AsmLabel
{
    std::string label;
};

struct AsmNull
{
};

using AsmInstructionVariant = std::variant<AsmLabel, AsmInstruction, AsmNull>;
using AsmInstructionVariants = std::vector<AsmInstructionVariant>;

} // namespace unassemblize
