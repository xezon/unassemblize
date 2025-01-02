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
enum Side : uint8_t
{
    LeftSide = 0,
    RightSide = 1,
};

inline Side get_opposite_side(Side side)
{
    return static_cast<Side>((side + 1) % 2);
}

enum class AsmMatchStrictness
{
    Lenient, // Unknown to known/unknown symbol pairs are treated as match.
    Undecided, // Unknown to known/unknown symbol pairs are treated as undecided, maybe match or mismatch.
    Strict, // Unknown to known/unknown symbol pairs are treated as mismatch.
};
inline constexpr size_t AsmMatchStrictnessCount = 3;

inline constexpr std::array<const char *, AsmMatchStrictnessCount> s_asmMatchStrictnessNames = {
    "Lenient",
    "Undecided",
    "Strict"};

inline constexpr const char *to_string(AsmMatchStrictness strictness)
{
    return s_asmMatchStrictnessNames[static_cast<size_t>(strictness)];
}
AsmMatchStrictness to_asm_match_strictness(std::string_view str);

enum class AsmMatchValue
{
    IsMatch,
    IsMaybeMatch,
    IsMaybeMismatch = IsMaybeMatch, // Opposite wording, but same meaning.
    IsMismatch,
};

// Extended match value. Same as the other, but with two more states after mismatch.
enum class AsmMatchValueEx
{
    IsMatch = AsmMatchValue::IsMatch,
    IsMaybeMatch = AsmMatchValue::IsMaybeMatch,
    IsMaybeMismatch = AsmMatchValue::IsMaybeMismatch, // Opposite wording, but same meaning.
    IsMismatch = AsmMatchValue::IsMismatch,
    IsMissingLeft,
    IsMissingRight,

    Count
};

inline constexpr std::array<std::string_view, size_t(AsmMatchValueEx::Count)> AsmMatchValueStringArray =
    {"==", "??", "xx", "<<", ">>"};
static_assert(size_t(AsmMatchValueEx::Count) == 5, "Adjust array if this length has changed.");
static_assert(AsmMatchValueStringArray[0].size() == AsmMatchValueStringArray[1].size(), "Expects same length");
static_assert(AsmMatchValueStringArray[0].size() == AsmMatchValueStringArray[2].size(), "Expects same length");
static_assert(AsmMatchValueStringArray[0].size() == AsmMatchValueStringArray[3].size(), "Expects same length");
static_assert(AsmMatchValueStringArray[0].size() == AsmMatchValueStringArray[4].size(), "Expects same length");

using AsmMismatchReason = uint16_t;
enum AsmMismatchReason_ : AsmMismatchReason
{
    AsmMismatchReason_JumpLen = 1 << 0, // Jump length is different.
    AsmMismatchReason_MissingLeft = 1 << 1, // Instruction is missing on the left side.
    AsmMismatchReason_MissingRight = 1 << 2, // Instruction is missing on the right side.
    AsmMismatchReason_Missing = AsmMismatchReason_MissingLeft | AsmMismatchReason_MissingRight,
    AsmMismatchReason_InvalidLeft = 1 << 3, // Instruction is invalid on the left side.
    AsmMismatchReason_InvalidRight = 1 << 4, // Instruction is invalid on the right side.
    AsmMismatchReason_Invalid = AsmMismatchReason_InvalidLeft | AsmMismatchReason_InvalidRight,
};

struct AsmMismatchInfo
{
    AsmMatchValue get_match_value(AsmMatchStrictness strictness) const;
    AsmMatchValueEx get_match_value_ex(AsmMatchStrictness strictness) const;

    bool is_match() const;
    bool is_mismatch() const;

    bool is_maybe_match() const;
    bool is_maybe_mismatch() const;

    uint16_t mismatch_bits = 0; // Bit positions where instructions are mismatching. Mutually exclusive.
    uint16_t maybe_mismatch_bits = 0; // Bit positions where instructions are maybe mismatching. Mutually exclusive.
    AsmMismatchReason mismatch_reasons = 0;
};
static_assert(sizeof(AsmMismatchInfo) <= 8);

struct AsmComparisonRecord
{
    uint8_t is_symbol() const;

    std::array<const AsmInstruction *, 2> pair = {}; // One pointer can be null.
    AsmMismatchInfo mismatch_info;
};

using AsmComparisonRecords = std::vector<AsmComparisonRecord>;

std::optional<ptrdiff_t> get_record_distance(
    const AsmComparisonRecords &records,
    Side side,
    Address64T address1,
    Address64T address2);

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
    uint32_t symbol_count = 0; // Number of records that contain at least one symbol.
    uint32_t match_count = 0;
    uint32_t maybe_match_count = 0; // Alias maybe mismatch, could be a match or mismatch.
    uint32_t mismatch_count = 0;
};

struct NamedFunction
{
    using Id = uint32_t;
    static constexpr Id InvalidId = ~Id(0);

    bool is_disassembled() const; // Is async compatible.
    TriState is_linked_to_source_file() const; // Is async compatible.

    std::string name;
    Function function;

    Id id = InvalidId;
    bool isDisassembled = false;
    TriState isLinkedToSourceFile = TriState::False;
};
using NamedFunctions = std::vector<NamedFunction>;
using NamedFunctionPair = std::array<NamedFunction *, 2>;
using ConstNamedFunctionPair = std::array<const NamedFunction *, 2>;
using NamedFunctionsPair = std::array<NamedFunctions *, 2>;
using ConstNamedFunctionsPair = std::array<const NamedFunctions *, 2>;

struct NamedFunctionMatchInfo
{
    bool is_matched() const;

    IndexT matched_index = ~IndexT(0); // Links to MatchedFunctions.
};
using NamedFunctionMatchInfos = std::vector<NamedFunctionMatchInfo>;

// Pairs a function from 2 executables that can be matched.
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

using BuildBundleFlags = uint8_t;
enum BuildBundleFlags_ : BuildBundleFlags
{
    BuildMatchedFunctionIndices = 1 << 0,
    BuildMatchedNamedFunctionIndices = 1 << 1,
    BuildUnmatchedNamedFunctionIndices = 1 << 2,
    BuildAllNamedFunctionIndices = 1 << 3,

    BuildBundleFlagsAll = 255u,
};

// Groups function matches of the same compiland or source file together.
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

    BuildBundleFlags flags = 0;
};
using NamedFunctionBundles = std::vector<NamedFunctionBundle>;

enum class MatchBundleType
{
    Compiland, // Functions will be bundled by the compilands they belong to.
    SourceFile, // Functions will be bundled by the source files they belong to (.h .cpp).
    None, // Functions will be bundled into one.

    Count
};

MatchBundleType to_match_bundle_type(std::string_view str);

} // namespace unassemblize
