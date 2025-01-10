/**
 * @file
 *
 * @brief Types to store relevant data for asm matching
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "asmmatchertypes.h"
#include "util.h"

namespace unassemblize
{
AsmMatchStrictness to_asm_match_strictness(std::string_view str)
{
    for (size_t i = 0; i < AsmMatchStrictnessCount; ++i)
    {
        if (util::equals_nocase(str, to_string(AsmMatchStrictness(i))))
        {
            return AsmMatchStrictness(i);
        }
    }
    printf("Unrecognized asm match to_asm_match_strictness '%.*s'. Defaulting to 'Undecided'", PRINTF_STRING(str));
    return AsmMatchStrictness::Undecided;
}

AsmMatchValue AsmMismatchInfo::get_match_value(AsmMatchStrictness strictness) const
{
    switch (strictness)
    {
        case AsmMatchStrictness::Lenient:
            if (mismatch_bits == 0 && mismatch_reasons == 0)
                return AsmMatchValue::IsMatch;
            else
                return AsmMatchValue::IsMismatch;

        default:
        case AsmMatchStrictness::Undecided:
            if (mismatch_bits == 0 && maybe_mismatch_bits == 0 && mismatch_reasons == 0)
                return AsmMatchValue::IsMatch;
            else if (maybe_mismatch_bits != 0)
                return AsmMatchValue::IsMaybeMatch;
            else
                return AsmMatchValue::IsMismatch;

        case AsmMatchStrictness::Strict:
            if (mismatch_bits == 0 && maybe_mismatch_bits == 0 && mismatch_reasons == 0)
                return AsmMatchValue::IsMatch;
            else
                return AsmMatchValue::IsMismatch;
    }
}

AsmMatchValueEx AsmMismatchInfo::get_match_value_ex(AsmMatchStrictness strictness) const
{
    const AsmMatchValue matchValue = get_match_value(strictness);
    if (matchValue == AsmMatchValue::IsMismatch)
    {
        if (mismatch_reasons & AsmMismatchReason_MissingLeft)
            return AsmMatchValueEx::IsMissingLeft;
        else if (mismatch_reasons & AsmMismatchReason_MissingRight)
            return AsmMatchValueEx::IsMissingRight;
        else
            return AsmMatchValueEx::IsMismatch;
    }
    else
    {
        return static_cast<AsmMatchValueEx>(matchValue);
    }
}

bool AsmMismatchInfo::is_match() const
{
    return mismatch_bits == 0 && maybe_mismatch_bits == 0 && mismatch_reasons == 0;
}

bool AsmMismatchInfo::is_mismatch() const
{
    return mismatch_bits != 0 || mismatch_reasons != 0;
}

bool AsmMismatchInfo::is_maybe_match() const
{
    return mismatch_bits == 0 && maybe_mismatch_bits != 0 && mismatch_reasons == 0;
}

bool AsmMismatchInfo::is_maybe_mismatch() const
{
    return is_maybe_match();
}

uint8_t AsmComparisonRecord::is_symbol() const
{
    uint8_t bits = 0;
    for (int i = 0; i < 2; ++i)
    {
        if (pair[i] != nullptr && pair[i]->isSymbol)
            bits |= (1 << i);
    }
    return bits;
}

std::optional<ptrdiff_t> get_record_distance(
    const AsmComparisonRecords &records,
    Side side,
    Address64T address1,
    Address64T address2)
{
    // Linear search instead of binary search, because there can be null elements. Expensive.

    AsmComparisonRecords::const_iterator it1 =
        std::find_if(records.begin(), records.end(), [=](const AsmComparisonRecord &record) {
            const AsmInstruction *instruction = record.pair[side];
            if (instruction != nullptr)
                return instruction->address == address1;
            return false;
        });

    if (it1 != records.end())
    {
        // Search front or back range depending where address2 is.
        AsmComparisonRecords::const_iterator begin;
        AsmComparisonRecords::const_iterator end;
        if (address1 < address2)
        {
            begin = it1;
            end = records.end();
        }
        else
        {
            begin = records.begin();
            end = it1;
        }
        AsmComparisonRecords::const_iterator it2 =
            std::find_if(records.begin(), records.end(), [=](const AsmComparisonRecord &record) {
                const AsmInstruction *instruction = record.pair[side];
                if (instruction != nullptr)
                    return instruction->address == address2;
                return false;
            });

        if (it2 != records.end())
        {
            return std::distance(it1, it2);
        }
    }
    return std::nullopt;
}

uint32_t AsmComparisonResult::get_instruction_count() const
{
    return static_cast<uint32_t>(records.size());
}

uint32_t AsmComparisonResult::get_match_count(AsmMatchStrictness strictness) const
{
    switch (strictness)
    {
        case AsmMatchStrictness::Lenient:
            return match_count + maybe_match_count;
        default:
        case AsmMatchStrictness::Undecided:
            return match_count;
        case AsmMatchStrictness::Strict:
            return match_count;
    }
}

uint32_t AsmComparisonResult::get_max_match_count(AsmMatchStrictness strictness) const
{
    switch (strictness)
    {
        case AsmMatchStrictness::Lenient:
            return match_count + maybe_match_count;
        default:
        case AsmMatchStrictness::Undecided:
            return match_count + maybe_match_count;
        case AsmMatchStrictness::Strict:
            return match_count;
    }
}

uint32_t AsmComparisonResult::get_mismatch_count(AsmMatchStrictness strictness) const
{
    switch (strictness)
    {
        case AsmMatchStrictness::Lenient:
            return mismatch_count;
        default:
        case AsmMatchStrictness::Undecided:
            return mismatch_count;
        case AsmMatchStrictness::Strict:
            return mismatch_count + maybe_match_count;
    }
}

uint32_t AsmComparisonResult::get_max_mismatch_count(AsmMatchStrictness strictness) const
{
    switch (strictness)
    {
        case AsmMatchStrictness::Lenient:
            return mismatch_count;
        default:
        case AsmMatchStrictness::Undecided:
            return mismatch_count + maybe_match_count;
        case AsmMatchStrictness::Strict:
            return mismatch_count + maybe_match_count;
    }
}

float AsmComparisonResult::get_similarity(AsmMatchStrictness strictness) const
{
    return float(get_match_count(strictness)) / float(get_instruction_count());
}

float AsmComparisonResult::get_max_similarity(AsmMatchStrictness strictness) const
{
    return float(get_max_match_count(strictness)) / float(get_instruction_count());
}

int8_t AsmComparisonResult::get_similarity_as_int(AsmMatchStrictness strictness) const
{
    return get_match_count(strictness) * 100 / get_instruction_count();
}

int8_t AsmComparisonResult::get_max_similarity_as_int(AsmMatchStrictness strictness) const
{
    return get_max_match_count(strictness) * 100 / get_instruction_count();
}

bool NamedFunction::is_disassembled() const
{
    if (isDisassembled)
    {
        assert(!function.get_instructions().empty());
    }
    return isDisassembled;
}

TriState NamedFunction::is_linked_to_source_file() const
{
    if (isLinkedToSourceFile == TriState::True)
    {
        assert(!function.get_source_file_name().empty());
    }
    return isLinkedToSourceFile;
}

bool NamedFunctionMatchInfo::is_matched() const
{
    return matched_index != ~IndexT(0);
}

bool MatchedFunction::is_compared() const
{
    return !comparison.records.empty();
}

MatchBundleType to_match_bundle_type(std::string_view str)
{
    if (util::equals_nocase(str, "compiland"))
    {
        return MatchBundleType::Compiland;
    }
    else if (util::equals_nocase(str, "sourcefile"))
    {
        return MatchBundleType::SourceFile;
    }
    else if (util::equals_nocase(str, "none"))
    {
        return MatchBundleType::None;
    }
    else
    {
        printf("Unrecognized match bundle type '%.*s'. Defaulting to 'None'", PRINTF_STRING(str));
        return MatchBundleType::None;
    }
    static_assert(size_t(MatchBundleType::Count) == 3, "Enum was changed. Update conditions.");
}

} // namespace unassemblize
