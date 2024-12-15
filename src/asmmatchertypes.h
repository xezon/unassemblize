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
#pragma once

#include "function.h"
#include <array>
#include <string>
#include <variant>
#include <vector>

namespace unassemblize
{
enum class AsmMatchStrictness
{
    Lenient, // Unknown to known/unknown symbol pairs are treated as match.
    Undecided, // Unknown to known/unknown symbol pairs are treated as undecided, maybe match or mismatch.
    Strict, // Unknown to known/unknown symbol pairs are treated as mismatch.
};

enum class AsmMatchValue
{
    IsMatch,
    IsMaybeMatch,
    IsMaybeMismatch = IsMaybeMatch,
    IsMismatch,
};

struct AsmMismatchInfo
{
    enum MismatchReason : uint16_t
    {
        MismatchReason_Missing = 1 << 0, // Instruction is missing on one side.
        MismatchReason_Invalid = 1 << 1, // Instruction is invalid on one side.
        MismatchReason_JumpLen = 1 << 2, // Jump length is different.
    };

    AsmMatchValue get_match_value(AsmMatchStrictness strictness) const;

    bool is_match() const;
    bool is_mismatch() const;

    bool is_maybe_match() const;
    bool is_maybe_mismatch() const;

    uint16_t mismatch_bits = 0; // Bits representing positions where instructions are mismatching.
    uint16_t maybe_mismatch_bits = 0; // Bits representing positions where instructions are maybe mismatching.
    uint16_t mismatch_reasons = 0;
};
static_assert(sizeof(AsmMismatchInfo) <= 8);

struct AsmLabelPair
{
    std::array<const AsmLabel *, 2> pair; // A pointer can be null.
};

struct AsmInstructionPair
{
    std::array<const AsmInstruction *, 2> pair = {}; // A pointer can be null.
    AsmMismatchInfo mismatch_info;
};

using AsmComparisonRecord = std::variant<AsmLabelPair, AsmInstructionPair>;
using AsmComparisonRecords = std::vector<AsmComparisonRecord>;

AsmMatchStrictness to_asm_match_strictness(const char *str);

struct AsmComparisonResult
{
    uint32_t get_instruction_count() const;
    uint32_t get_match_count(AsmMatchStrictness strictness) const;
    uint32_t get_max_match_count(AsmMatchStrictness strictness) const;
    uint32_t get_mismatch_count(AsmMatchStrictness strictness) const;
    uint32_t get_max_mismatch_count(AsmMatchStrictness strictness) const;

    // Returns 0..1
    float get_similarity(AsmMatchStrictness strictness) const;
    // Returns 0..1
    float get_max_similarity(AsmMatchStrictness strictness) const;

    // Returns 0..100
    int8_t get_similarity_as_int(AsmMatchStrictness strictness) const;
    // Returns 0..100
    int8_t get_max_similarity_as_int(AsmMatchStrictness strictness) const;

    AsmComparisonRecords records;
    uint32_t label_count = 0;
    uint32_t match_count = 0;
    uint32_t maybe_match_count = 0; // Alias maybe mismatch, could be a match or mismatch.
    uint32_t mismatch_count = 0;
};

struct NamedFunction
{
    using Id = uint32_t;
    static constexpr Id InvalidId = ~Id(0);

    bool is_disassembled() const;
    TriState is_linked_to_source_file() const;

    std::string name;
    Function function;

    Id id = InvalidId;

    // Is set false if function could not be linked to a source file.
    bool canLinkToSourceFile = true;
};
using NamedFunctions = std::vector<NamedFunction>;
using NamedFunctionPair = std::array<NamedFunction *, 2>;
using NamedFunctionsPair = std::array<NamedFunctions *, 2>;
using ConstNamedFunctionsPair = std::array<const NamedFunctions *, 2>;

struct NamedFunctionMatchInfo
{
    bool is_matched() const;

    IndexT matched_index = ~IndexT(0); // Links to MatchedFunctions.
};
using NamedFunctionMatchInfos = std::vector<NamedFunctionMatchInfo>;

/*
 * Pairs a function from 2 executables that can be matched.
 */
struct MatchedFunction
{
    bool is_compared() const;

    std::array<IndexT, 2> named_idx_pair = {}; // Links to NamedFunctions.
    AsmComparisonResult comparison;
};
using MatchedFunctions = std::vector<MatchedFunction>;

struct MatchedFunctionsData
{
    MatchedFunctions matchedFunctions;
    std::array<NamedFunctionMatchInfos, 2> namedFunctionMatchInfosArray;
};

/*
 * Groups function matches of the same compiland or source file together.
 */
struct NamedFunctionBundle
{
    using Id = uint32_t;
    static constexpr Id InvalidId = ~Id(0);

    std::string name; // Compiland or source file name.

    std::vector<IndexT> matchedFunctionIndices; // Links to MatchedFunctions.
    std::vector<IndexT> matchedNamedFunctionIndices; // Links to NamedFunctions. In sync with matchedFunctionsIndices.
    std::vector<IndexT> unmatchedNamedFunctionIndices; // Links to NamedFunctions.
    std::vector<IndexT> allNamedFunctionIndices; // Links to NamedFunctions. Contains matched and unmatched ones.

    Id id = InvalidId;
};
using NamedFunctionBundles = std::vector<NamedFunctionBundle>;

enum class MatchBundleType
{
    Compiland, // Functions will be bundled by the compilands they belong to.
    SourceFile, // Functions will be bundled by the source files they belong to (.h .cpp).
    None, // Functions will be bundled into one.

    Count
};

MatchBundleType to_match_bundle_type(const char *str);

struct StringPair
{
    std::array<std::string, 2> pair;
};

inline ConstFunctionPair to_const_function_pair(ConstNamedFunctionsPair named_functions_pair, const MatchedFunction &matched)
{
    return ConstFunctionPair{
        &named_functions_pair[0]->at(matched.named_idx_pair[0]).function,
        &named_functions_pair[1]->at(matched.named_idx_pair[1]).function};
}

inline NamedFunctionPair to_named_function_pair(NamedFunctionsPair named_functions_pair, const MatchedFunction &matched)
{
    return NamedFunctionPair{
        &named_functions_pair[0]->at(matched.named_idx_pair[0]),
        &named_functions_pair[1]->at(matched.named_idx_pair[1])};
}

} // namespace unassemblize
