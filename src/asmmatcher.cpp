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
#include "asmmatcher.h"
#include "strings.h"

namespace unassemblize
{
AsmInstructionVariant AsmMatcher::s_nullInstructionVariant(AsmNull{});

AsmComparisonResult AsmMatcher::run_comparison(ConstFunctionPair function_pair, uint32_t lookahead_limit)
{
    AsmComparisonResult comparison;

    const AsmInstructionVariants &instructions0 = function_pair[0]->get_instructions();
    const AsmInstructionVariants &instructions1 = function_pair[1]->get_instructions();
    assert(instructions0.size() != 0);
    assert(instructions1.size() != 0);

    // Creates all instruction splits in advance to avoid redundant splits when visiting instructions multiple times.
    const InstructionTextArrays arrays0 = split_instruction_texts(instructions0);
    const InstructionTextArrays arrays1 = split_instruction_texts(instructions1);
    const InstructionTextArray empty_array;

    const size_t inst_count0 = function_pair[0]->get_instruction_count();
    const size_t inst_count1 = function_pair[1]->get_instruction_count();
    const size_t label_count0 = function_pair[0]->get_label_count();
    const size_t label_count1 = function_pair[1]->get_label_count();

    {
        const size_t label_count_comb = label_count0 + label_count1;

        // Over-reserves with both label counts on both sides because labels can be unaligned in worst case mismatches.
        // On top of that, empty entries can take up space on either side, so this adds a guessed margin.
        size_t max_size = std::max(inst_count0 + label_count_comb, inst_count1 + label_count_comb);
        max_size = (size_t)((double)max_size * 1.2);
        comparison.records.reserve(max_size);
    }

    const size_t count0 = instructions0.size();
    const size_t count1 = instructions1.size();
    size_t i0 = 0;
    size_t i1 = 0;

    while (i0 < count0 || i1 < count1)
    {
        bool do_lookahead = false;

        const InstructionTextArray &array0 = (i0 < count0) ? arrays0[i0] : empty_array;
        const InstructionTextArray &array1 = (i1 < count1) ? arrays1[i1] : empty_array;

        AsmMismatchInfo mismatch_info;

        {
            const AsmInstructionVariant &variant0 = (i0 < count0) ? instructions0[i0] : s_nullInstructionVariant;
            const AsmInstructionVariant &variant1 = (i1 < count1) ? instructions1[i1] : s_nullInstructionVariant;

            // Check for any labels.

            const AsmLabel *label0 = std::get_if<AsmLabel>(&variant0);
            const AsmLabel *label1 = std::get_if<AsmLabel>(&variant1);

            if (label0 != nullptr || label1 != nullptr)
            {
                AsmLabelPair label_pair;
                label_pair.pair[0] = label0;
                label_pair.pair[1] = label1;
                comparison.records.emplace_back(std::move(label_pair));
                ++comparison.label_count;

                if (label0 != nullptr)
                    ++i0;
                if (label1 != nullptr)
                    ++i1;

                continue;
            }

            // Check for missing instruction on either side and mismatching instructions.

            const AsmInstruction *instruction0 = std::get_if<AsmInstruction>(&variant0);
            const AsmInstruction *instruction1 = std::get_if<AsmInstruction>(&variant1);

            assert(instruction0 != nullptr || instruction1 != nullptr);

            mismatch_info = create_mismatch_info(instruction0, instruction1, &array0, &array1);

            if (mismatch_info.is_match())
            {
                do_lookahead = false;
            }
            else
            {
                // Lookahead if 'mismatch' or 'maybe mismatch'. Perhaps there is a better match ahead.
                // No lookahead if instruction is missing on one side.
                do_lookahead = (mismatch_info.mismatch_reasons & AsmMismatchInfo::MismatchReason_Missing) == 0;
            }

            if (do_lookahead)
            {
                // Lookahead instructions to check if there is a match further ahead.
                assert(!mismatch_info.is_match());
                assert(instruction0 != nullptr);
                assert(instruction1 != nullptr);

                size_t lookahead_limit0 = lookahead_limit;
                size_t lookahead_limit1 = lookahead_limit;
                size_t k0 = 0;
                size_t k1 = 0;
                ++k0;

                while (i0 + k0 < count0 && i1 + k1 < count1 && k0 < lookahead_limit0 && k1 < lookahead_limit1)
                {
                    // Lookahead takes turns on both sides.
                    // The first lookahead match will determine the side that skips ahead.
                    if (k0 > k1)
                    {
                        AsmInstructionVariants::const_iterator lookahead_base_it = instructions0.begin() + i0;
                        AsmInstructionVariants::const_iterator lookahead_last_it = lookahead_base_it + k0;
                        const InstructionTextArray &lookahead_last_array = arrays0[i0 + k0];
                        LookaheadResult lookahead_result = run_lookahead_comparison(
                            0,
                            lookahead_base_it,
                            lookahead_last_it,
                            lookahead_last_array,
                            *instruction1,
                            array1,
                            comparison);

                        if (lookahead_result.is_label)
                        {
                            // Skip over label.
                            ++k0;
                            ++lookahead_limit0;
                        }
                        else if (lookahead_result.is_matching)
                        {
                            // Set new base index and break.
                            mismatch_info = lookahead_result.mismatch_info;
                            i0 += k0;
                            break;
                        }
                        else
                        {
                            // Increment opposite side index to look at next.
                            ++k1;
                        }
                    }
                    else
                    {
                        AsmInstructionVariants::const_iterator lookahead_base_it = instructions1.begin() + i1;
                        AsmInstructionVariants::const_iterator lookahead_last_it = lookahead_base_it + k1;
                        const InstructionTextArray &lookahead_last_array = arrays1[i1 + k1];
                        LookaheadResult lookahead_result = run_lookahead_comparison(
                            1,
                            lookahead_base_it,
                            lookahead_last_it,
                            lookahead_last_array,
                            *instruction0,
                            array0,
                            comparison);

                        if (lookahead_result.is_label)
                        {
                            // Skip over label.
                            ++k1;
                            ++lookahead_limit1;
                        }
                        else if (lookahead_result.is_matching)
                        {
                            // Set new base index and break.
                            mismatch_info = lookahead_result.mismatch_info;
                            i1 += k1;
                            break;
                        }
                        else
                        {
                            // Increment opposite side index to look at next.
                            ++k0;
                        }
                    }
                }
            }
        }

        const AsmInstructionVariant &variant0 = (i0 < count0) ? instructions0[i0] : s_nullInstructionVariant;
        const AsmInstructionVariant &variant1 = (i1 < count1) ? instructions1[i1] : s_nullInstructionVariant;

        const AsmInstruction *instruction0 = std::get_if<AsmInstruction>(&variant0);
        const AsmInstruction *instruction1 = std::get_if<AsmInstruction>(&variant1);

        assert(mismatch_info.is_mismatch() || (instruction0 != nullptr && instruction1 != nullptr));

        {
            AsmInstructionPair instruction_pair;
            instruction_pair.pair[0] = instruction0;
            instruction_pair.pair[1] = instruction1;
            instruction_pair.mismatch_info = mismatch_info;
            comparison.records.emplace_back(std::move(instruction_pair));
        }

        if (mismatch_info.is_match())
            ++comparison.match_count;
        else if (mismatch_info.is_maybe_match())
            ++comparison.maybe_match_count;
        else if (mismatch_info.is_mismatch())
            ++comparison.mismatch_count;

        if (instruction0 != nullptr)
            ++i0;
        if (instruction1 != nullptr)
            ++i1;
    }

    assert(comparison.label_count >= std::max(label_count0, label_count1));
    assert(comparison.get_instruction_count() >= std::max(inst_count0, inst_count1));
    assert(comparison.get_instruction_count() + comparison.label_count == comparison.records.size());

    return comparison;
}

AsmMatcher::LookaheadResult AsmMatcher::run_lookahead_comparison(
    size_t lookahead_side,
    AsmInstructionVariants::const_iterator lookahead_base_it,
    AsmInstructionVariants::const_iterator lookahead_last_it,
    const InstructionTextArray &lookahead_last_array,
    const AsmInstruction &opposite_base_instruction,
    const InstructionTextArray &opposite_base_array,
    AsmComparisonResult &comparison)
{
    assert(lookahead_side < 2);
    assert(lookahead_base_it < lookahead_last_it);

    const size_t opposite_side = (lookahead_side + 1) % 2;
    LookaheadResult lookahead_result;

    if (const AsmLabel *label = std::get_if<AsmLabel>(&*lookahead_last_it))
    {
        lookahead_result.is_label = true;
    }
    else
    {
        assert(std::holds_alternative<AsmInstruction>(*lookahead_last_it));
        const AsmInstruction &lookahead_last_instruction = std::get<AsmInstruction>(*lookahead_last_it);

        lookahead_result.mismatch_info = create_mismatch_info(
            &lookahead_last_instruction,
            &opposite_base_instruction,
            &lookahead_last_array,
            &opposite_base_array);

        if (lookahead_result.mismatch_info.is_match())
        {
            // Lookahead instruction is matching with base instruction on the other side.
            lookahead_result.is_matching = true;

            for (auto it = lookahead_base_it; it < lookahead_last_it; ++it)
            {
                const AsmInstructionVariant &variant = *it;
                if (const AsmLabel *label = std::get_if<AsmLabel>(&variant))
                {
                    // Insert the label and go ahead.
                    AsmLabelPair label_pair;
                    label_pair.pair[lookahead_side] = label;
                    label_pair.pair[opposite_side] = nullptr;
                    comparison.records.emplace_back(std::move(label_pair));
                    ++comparison.label_count;
                }
                else
                {
                    // These are all mismatches because above it hit the first match upon lookahead.
                    assert(std::holds_alternative<AsmInstruction>(variant));
                    const AsmInstruction &instruction = std::get<AsmInstruction>(variant);
                    AsmInstructionPair instruction_pair;
                    AsmMismatchInfo mismatch_info;
                    mismatch_info.mismatch_bits = ~uint16_t(0);
                    instruction_pair.pair[lookahead_side] = &instruction;
                    instruction_pair.pair[opposite_side] = nullptr;
                    instruction_pair.mismatch_info = mismatch_info;
                    comparison.records.emplace_back(std::move(instruction_pair));
                    ++comparison.mismatch_count;
                }
            }
        }
    }

    return lookahead_result;
}

AsmMismatchInfo AsmMatcher::create_mismatch_info(
    const AsmInstruction *instruction0,
    const AsmInstruction *instruction1,
    const InstructionTextArray *array0,
    const InstructionTextArray *array1)
{
    AsmMismatchInfo mismatch_info;

    if (instruction0 == nullptr || instruction1 == nullptr)
    {
        mismatch_info.mismatch_reasons |= AsmMismatchInfo::MismatchReason_Missing;
    }
    else if (instruction0->isInvalid != instruction1->isInvalid)
    {
        mismatch_info.mismatch_reasons |= AsmMismatchInfo::MismatchReason_Invalid;
    }
    else
    {
        if (array0 != nullptr && array1 != nullptr)
        {
            mismatch_info = compare_asm_text(*array0, *array1);
        }
        else
        {
            mismatch_info = compare_asm_text(instruction0->text, instruction1->text);
        }

        if (has_jump_len_mismatch(*instruction0, *instruction1))
        {
            mismatch_info.mismatch_reasons |= AsmMismatchInfo::MismatchReason_JumpLen;
        }
    }

    return mismatch_info;
}

bool AsmMatcher::has_jump_len_mismatch(const AsmInstruction &instruction0, const AsmInstruction &instruction1)
{
    return instruction0.isJump && instruction1.isJump && instruction0.jumpLen != instruction1.jumpLen;
}

AsmMismatchInfo AsmMatcher::compare_asm_text(std::string_view text0, std::string_view text1)
{
    const InstructionTextArray array0 = split_instruction_text(text0);
    const InstructionTextArray array1 = split_instruction_text(text1);

    return compare_asm_text(array0, array1);
}

AsmMismatchInfo AsmMatcher::compare_asm_text(const InstructionTextArray &array0, const InstructionTextArray &array1)
{
    // Note: All symbols, including pseudo symbols, are expected to be enclosed by quotes.

    AsmMismatchInfo result;

    std::string dummy;

    for (size_t i = 0; i < array0.size || i < array1.size; ++i)
    {
        const std::string &word0 = (i < array0.size) ? array0.elements[i] : dummy;
        const std::string &word1 = (i < array1.size) ? array1.elements[i] : dummy;
        const char *c0 = word0.c_str();
        const char *c1 = word1.c_str();
        int in_quote = -1;

        for (; *c0 != '\0' || *c1 != '\0'; ++c0, ++c1)
        {
            if (*c0 == '\"' && *c1 == '\"')
            {
                // String is entering or leaving a quoted symbol name.
                in_quote = in_quote < 0 ? 0 : -1;
                continue;
            }
            else if (in_quote >= 0)
            {
                ++in_quote;
            }

            if (in_quote == 1)
            {
                assert(*c0 != '\"');
                assert(*c1 != '\"');

                // Skip ahead unknown symbols, such as "unk_12A0".
                SkipSymbolResult skip0 = skip_unknown_symbol(c0);
                SkipSymbolResult skip1 = skip_unknown_symbol(c1);

                bool skipped0 = skip0.skipped();
                bool skipped1 = skip1.skipped();

                if (skipped0 && skipped1 && skip0.skipped_prefix != skip1.skipped_prefix)
                {
                    // Abort skipping if the prefix labels differ somehow.
                    skipped0 = false;
                    skipped1 = false;
                }

                // If one side skipped an unknown symbol, then skip the other symbol as well.
                if (skipped0 && !skipped1)
                {
                    skip1.skipped_str = skip_known_symbol(c1);
                    skipped1 = true;
                }
                else if (!skipped0 && skipped1)
                {
                    skip0.skipped_str = skip_known_symbol(c0);
                    skipped0 = true;
                }

                assert(skipped0 == skipped1);

                // If just one side is prefixed with "loc_", then the symbols certainly do not match.
                // This only happens when comparing different kinds of instructions.
                if (skip0.skipped_prefix == s_prefix_loc || skip1.skipped_prefix == s_prefix_loc)
                {
                    if (skip0.skipped_prefix != skip1.skipped_prefix)
                    {
                        skipped0 = false;
                        skipped1 = false;
                    }
                }

                // If at least one symbol was skipped, then this quote is done.
                if (skipped0)
                {
                    c0 = skip0.skipped_str;
                    c1 = skip1.skipped_str;

                    assert(*c0 == '\"');
                    assert(*c1 == '\"');

                    if (skip0.skipped_prefix != s_prefix_loc)
                    {
                        // Never mismatch on the "loc_" prefix, which is intended for jump labels.
                        result.maybe_mismatch_bits |= (1 << i);
                    }

                    in_quote = -1;
                    continue;
                }
            }

            if (*c0 != *c1)
            {
                // This section is mismatching. Break.
                result.mismatch_bits |= (1 << i);
                break;
            }
        }
    }

    // Verifies that no bits are shared across both bit fields.
    assert((result.mismatch_bits ^ result.maybe_mismatch_bits) == (result.mismatch_bits | result.maybe_mismatch_bits));

    return result;
}

AsmMatcher::SkipSymbolResult AsmMatcher::skip_unknown_symbol(const char *str)
{
    SkipSymbolResult result;

    for (const std::string_view prefix : s_prefix_array)
    {
        if (0 == strncasecmp(str, prefix.data(), prefix.size()))
        {
            str += prefix.size();
            while (*str != '\"')
            {
                ++str;
            }
            result.skipped_prefix = prefix;
            break;
        }
    }
    result.skipped_str = str;
    return result;
}

const char *AsmMatcher::skip_known_symbol(const char *str)
{
    while (*str != '\"' && *str != '\0')
    {
        ++str;
    }
    return str;
}

AsmMatcher::InstructionTextArrays AsmMatcher::split_instruction_texts(const AsmInstructionVariants &instructions)
{
    InstructionTextArrays arrays;
    const size_t size = instructions.size();
    arrays.resize(size);
    for (size_t i = 0; i < size; ++i)
    {
        if (const AsmInstruction *instruction = std::get_if<AsmInstruction>(&instructions[i]))
        {
            arrays[i] = split_instruction_text(instruction->text);
        }
    }
    return arrays;
}

AsmMatcher::InstructionTextArray AsmMatcher::split_instruction_text(std::string_view text)
{
    InstructionTextArray arr;
    size_t index = 0;
    char sep = ' ';
    bool in_quote = false;
    const char *end = text.data() + text.size();

    for (const char *c = text.data(); c != end; ++c)
    {
        if (*c == '\"')
        {
            // Does not look for separator inside quoted text.
            in_quote = !in_quote;
        }
        else if (!in_quote && *c == sep)
        {
            // Change word separator for operands.
            sep = ',';
            // Omit spaces between operands.
            while (*(c + 1) == ' ')
                ++c;
            // Increment word index.
            ++index;
            assert(index < arr.elements.size());
            continue;
        }
        arr.elements[index].push_back(*c);
    }
    arr.size = index + 1;
    return arr;
}

} // namespace unassemblize
