/**
 * @file
 *
 * @brief Class to print asm texts with
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
#include "function.h"

namespace unassemblize
{
struct TextFileContent;
struct TextFileContentPair;

class AsmPrinter
{
    struct Buffers
    {
        std::vector<std::string> lines;
        std::string misc_buf;
    };

public:
    // Appends texts from instruction data to a string.
    static void append_to_string(
        std::string &str,
        const Executable &executable,
        const Function &function,
        uint32_t indent_len);

    // Appends texts from instruction data of a comparison result to a string.
    void append_to_string(
        std::string &str,
        const AsmComparisonResult &comparison,
        ConstNamedFunctionPair named_function_pair,
        ConstExecutablePair executable_pair,
        const TextFileContentPair &source_file_texts,
        AsmMatchStrictness match_strictness,
        uint32_t indent_len,
        uint32_t asm_len,
        uint32_t byte_count,
        uint32_t sourcecode_len,
        uint32_t sourceline_len);

private:
    static std::string to_string(const AsmInstruction &instruction, size_t indent_len);

    static void append_source_code(
        Buffers &buffers,
        const AsmComparisonRecords &records,
        const TextFileContent &source_file_text,
        Side side,
        uint32_t sourcecode_len,
        uint32_t sourceline_len);
    static void append_bytes(Buffers &buffers, const AsmComparisonRecords &records, Side side, uint32_t byte_count);
    static void append_assembler(
        Buffers &buffers,
        const AsmComparisonRecords &records,
        const Executable &executable,
        const Function &function,
        Side side,
        uint32_t asm_len,
        uint32_t indent_len);
    static void append_comparison(
        Buffers &buffers,
        const AsmComparisonRecords &records,
        AsmMatchStrictness match_strictness);

    static void truncate_inplace(std::string &str, size_t max_len);
    static void front_truncate_inplace(std::string &str, size_t max_len);
    static void pad_whitespace_inplace(std::string &str, size_t len);
    static void append_whitespace_inplace(std::string &str, size_t len);

private:
    Buffers m_buffers;
};

} // namespace unassemblize
