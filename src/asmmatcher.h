/**
 * @file
 *
 * @brief Class to compare assembler texts
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "asmmatchertypes.h"

namespace unassemblize
{
class AsmMatcher
{
    using InstructionTextArray = SizedArray<std::string, size_t, 4>;
    using InstructionTextArrays = std::vector<InstructionTextArray>;

    struct LookaheadResult
    {
        AsmMismatchInfo mismatch_info;
        bool is_label = false; // The lookahead has hit a label.
        bool is_matching = false; // The lookahead is considered a match. It could be a maybe match.
    };

    struct SkipSymbolResult
    {
        bool skipped() const { return !skipped_prefix.empty(); }

        const char *skipped_str = nullptr;
        std::string_view skipped_prefix;
    };

public:
    /*
     * Runs a comparison on the given FunctionMatch.
     * The returned result will retain a dependency on that FunctionMatch object.
     */
    static AsmComparisonResult run_comparison(ConstFunctionPair function_pair, uint32_t lookahead_limit);

private:
    /*
     * Looks ahead one side of the instruction list and compares
     * its last instruction with the base instruction of the opposite side.
     */
    static LookaheadResult run_lookahead_comparison(
        size_t lookahead_side,
        AsmInstructionVariants::const_iterator lookahead_base_it,
        AsmInstructionVariants::const_iterator lookahead_last_it,
        const InstructionTextArray &lookahead_last_array,
        const AsmInstruction &opposite_base_instruction,
        const InstructionTextArray &opposite_base_array,
        AsmComparisonResult &comparison);

    // Passing arrays is optional, but recommended for performance reasons.
    static AsmMismatchInfo create_mismatch_info(
        const AsmInstruction *instruction0,
        const AsmInstruction *instruction1,
        const InstructionTextArray *array0 = nullptr,
        const InstructionTextArray *array1 = nullptr);

    static bool has_jump_len_mismatch(const AsmInstruction &instruction0, const AsmInstruction &instruction1);
    static AsmMismatchInfo compare_asm_text(std::string_view text0, std::string_view text1);
    static AsmMismatchInfo compare_asm_text(const InstructionTextArray &array0, const InstructionTextArray &array1);
    static SkipSymbolResult skip_unknown_symbol(const char *str);
    static const char *skip_known_symbol(const char *str);

    /*
     * Splits instruction text string to text array.
     * "mov dword ptr[eax], 0x10" becomes {"mov", "dword ptr[eax]", "0x10"}
     */
    static InstructionTextArrays split_instruction_texts(const AsmInstructionVariants &instructions);
    static InstructionTextArray split_instruction_text(std::string_view text);

    static AsmInstructionVariant s_nullInstructionVariant;
};

} // namespace unassemblize
