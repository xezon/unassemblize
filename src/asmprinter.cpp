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
#include "asmprinter.h"
#include "filecontentstorage.h"
#include "util.h"
#include <fmt/core.h>

namespace unassemblize
{
void AsmPrinter::append_to_string(
    std::string &str,
    const Executable &executable,
    const Function &function,
    uint32_t indent_len)
{
    constexpr std::string_view eol = "\n";

    const AsmInstructions &instructions = function.get_instructions();
    if (instructions.empty())
        return;

    // The first variant is expected to be a label if it is the begin of a function.
    std::string name;
    if (instructions[0].isSymbol)
    {
        const ExeSymbol *symbol = get_symbol_or_pseudo_symbol(instructions[0].address, executable, function);
        assert(symbol != nullptr);
        name = symbol->name;
    }
    else
    {
        name = "_unknown_";
    }

    std::string header = fmt::format(
        ".intel_syntax noprefix{:s}{:s}"
        ".globl {:s}{:s}",
        eol,
        eol,
        name,
        eol);

    // Simple estimation based on game.dat
    str.reserve(header.size() + instructions.size() * (indent_len + 24));
    str += header;

    for (const AsmInstruction &instruction : instructions)
    {
        if (instruction.isSymbol)
        {
            const ExeSymbol *symbol = get_symbol_or_pseudo_symbol(instruction.address, executable, function);
            assert(symbol != nullptr);
            str += symbol->name;
            str += ':';
            str += eol;
        }

        str += to_string(instruction, indent_len);
        str += eol;
    }
}

std::string AsmPrinter::to_string(const AsmInstruction &instruction, size_t indent_len)
{
    constexpr std::string_view strip_quote = "\"";
    std::string str;

    if (instruction.isInvalid)
    {
        // Assign unrecognized instruction as comment.
        str = fmt::format("; Unrecognized opcode at address:{:08x} bytes:{:s}", instruction.address, instruction.text);
    }
    else
    {
        // Assign assembler text.
        str.reserve(indent_len + instruction.text.size());
        append_whitespace_inplace(str, indent_len);
        str.append(instruction.text);
        util::strip_inplace(str, strip_quote);

        if (instruction.isJump)
        {
            // Append jump distance as inline comment.
            str += fmt::format(" ; {:+d} bytes", instruction.jumpLen);
        }
    }

    return str;
}

void AsmPrinter::append_to_string(
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
    uint32_t sourceline_len)
{
    if (comparison.records.empty())
        return;

    constexpr std::string_view eol = "\n";
    constexpr size_t end_eol_count = 4;

    // Keeps buffer allocated for reuse.
    m_buffers.lines.clear();
    m_buffers.lines.resize(comparison.records.size() + comparison.symbol_count);
    m_buffers.misc_buf.reserve(1024);

    struct LineRegion
    {
        int begin = -1;
        int end = -1;
    };
    std::array<LineRegion, 2> assembler_regions;
    std::array<LineRegion, 2> source_code_regions;

    {
        // Create assembler lines in vector.
        const uint32_t source_len = sourcecode_len + sourceline_len;
        Side side = LeftSide;

        if (source_len > 0 && source_file_texts.pair[side] != nullptr)
        {
            source_code_regions[side].begin = m_buffers.lines[0].size();
            append_source_code(
                m_buffers,
                comparison.records,
                *source_file_texts.pair[side],
                side,
                sourcecode_len,
                sourceline_len);
            source_code_regions[side].end = m_buffers.lines[0].size();
        }

        if (byte_count > 0)
        {
            append_bytes(m_buffers, comparison.records, side, byte_count);
        }

        assembler_regions[side].begin = m_buffers.lines[0].size();
        append_assembler(
            m_buffers,
            comparison.records,
            *executable_pair[side],
            named_function_pair[side]->function,
            side,
            asm_len,
            indent_len);
        assembler_regions[side].end = m_buffers.lines[0].size();

        append_comparison(m_buffers, comparison.records, match_strictness);
        side = RightSide;

        assembler_regions[side].begin = m_buffers.lines[0].size();
        append_assembler(
            m_buffers,
            comparison.records,
            *executable_pair[side],
            named_function_pair[side]->function,
            side,
            asm_len,
            indent_len);
        assembler_regions[side].end = m_buffers.lines[0].size();

        if (byte_count > 0)
        {
            append_bytes(m_buffers, comparison.records, side, byte_count);
        }

        if (source_len > 0 && source_file_texts.pair[side] != nullptr)
        {
            source_code_regions[side].begin = m_buffers.lines[0].size();
            append_source_code(
                m_buffers,
                comparison.records,
                *source_file_texts.pair[side],
                side,
                sourcecode_len,
                sourceline_len);
            source_code_regions[side].end = m_buffers.lines[0].size();
        }
    }

    {
        // Create misc info.
        m_buffers.misc_buf.clear();

        const std::string_view name = named_function_pair[0]->name;
        const uint32_t match_count = comparison.get_match_count(match_strictness);
        const uint32_t max_match_count = comparison.get_max_match_count(match_strictness);
        const uint32_t mismatch_count = comparison.get_mismatch_count(match_strictness);
        const uint32_t max_mismatch_count = comparison.get_max_mismatch_count(match_strictness);
        const float similarity = comparison.get_similarity(match_strictness);
        const float max_similarity = comparison.get_max_similarity(match_strictness);

        m_buffers.misc_buf += name;
        m_buffers.misc_buf += eol;
        m_buffers.misc_buf += fmt::format("match count: {:d}", match_count);
        if (max_match_count != match_count)
        {
            m_buffers.misc_buf += fmt::format(" or {:d}", max_match_count);
        }
        m_buffers.misc_buf += eol;
        m_buffers.misc_buf += fmt::format("mismatch count: {:d}", mismatch_count);
        if (max_mismatch_count != mismatch_count)
        {
            m_buffers.misc_buf += fmt::format(" or {:d}", max_mismatch_count);
        }
        m_buffers.misc_buf += eol;
        m_buffers.misc_buf += fmt::format("similarity: {:.1f} %", similarity * 100.f);
        if (max_similarity != similarity)
        {
            m_buffers.misc_buf += fmt::format(" or {:.1f} %", max_similarity * 100.f);
        }
        m_buffers.misc_buf += eol;
        m_buffers.misc_buf += eol;

        str.append(m_buffers.misc_buf);
    }

    {
        // Create file names.
        m_buffers.misc_buf.clear();

        {
            const LineRegion &region = source_code_regions[0];

            if (region.begin < region.end)
            {
                pad_whitespace_inplace(m_buffers.misc_buf, region.begin);
                std::string filename_copy = source_file_texts.pair[0]->filename;
                front_truncate_inplace(filename_copy, region.end - region.begin);
                m_buffers.misc_buf += filename_copy;
                pad_whitespace_inplace(m_buffers.misc_buf, region.end);
            }
        }

        for (size_t i = 0; i < 2; ++i)
        {
            const LineRegion &region = assembler_regions[i];

            assert(region.begin < region.end);
            pad_whitespace_inplace(m_buffers.misc_buf, region.begin);
            std::string filename_copy = executable_pair[i]->get_filename();
            front_truncate_inplace(filename_copy, region.end - region.begin);
            m_buffers.misc_buf += filename_copy;
            pad_whitespace_inplace(m_buffers.misc_buf, region.end);
        }

        {
            const LineRegion &region = source_code_regions[1];

            if (region.begin < region.end)
            {
                pad_whitespace_inplace(m_buffers.misc_buf, region.begin);
                std::string filename_copy = source_file_texts.pair[1]->filename;
                front_truncate_inplace(filename_copy, region.end - region.begin);
                m_buffers.misc_buf += filename_copy;
                pad_whitespace_inplace(m_buffers.misc_buf, region.end);
            }
        }

        m_buffers.misc_buf += eol;
        str.append(m_buffers.misc_buf);
    }

    // Add all lines to output string.
    for (std::string &line : m_buffers.lines)
    {
        str.append(line);
        str.append(eol);
    }

    // Add line breaks at the end.
    for (size_t i = 0; i < end_eol_count; ++i)
    {
        str.append(eol);
    }
}

void AsmPrinter::append_source_code(
    Buffers &buffers,
    const AsmComparisonRecords &records,
    const TextFileContent &source_file_text,
    Side side,
    uint32_t sourcecode_len,
    uint32_t sourceline_len)
{
    if (sourceline_len > 0)
        sourceline_len += 1; // +1 for colon.

    size_t line_idx = 0;

    for (const AsmComparisonRecord &record : records)
    {
        // Label Row

        if (record.is_symbol())
        {
            append_whitespace_inplace(buffers.lines[line_idx], sourceline_len + sourcecode_len);
            ++line_idx;
        }

        // Instruction Row

        std::string &line = buffers.lines[line_idx];
        const size_t offset = line.size();
        const AsmInstruction *instruction = record.pair[side];
        if (instruction != nullptr)
        {
            const uint16_t line_idx = instruction->get_line_index();

            if (line_idx < source_file_text.lines.size())
            {
                buffers.misc_buf.assign(fmt::format("{:05d}:", instruction->lineNumber));
                if (buffers.misc_buf.size() > sourceline_len)
                {
                    buffers.misc_buf.erase(0, buffers.misc_buf.size() - sourceline_len);
                }
                line.append(buffers.misc_buf);

                if (instruction->isFirstLine)
                {
                    buffers.misc_buf.assign(source_file_text.lines[line_idx]);
                    truncate_inplace(buffers.misc_buf, sourcecode_len);
                    line.append(buffers.misc_buf);
                }
            }
        }
        pad_whitespace_inplace(line, sourceline_len + sourcecode_len + offset);
        ++line_idx;
    }
    assert(line_idx == buffers.lines.size());
}

void AsmPrinter::append_bytes(Buffers &buffers, const AsmComparisonRecords &records, Side side, uint32_t byte_count)
{
    byte_count = std::min<uint32_t>(byte_count, AsmInstruction::BytesArray::MaxSize);
    const size_t bytes_len = byte_count * (2 + 1);
    size_t line_idx = 0;

    for (const AsmComparisonRecord &record : records)
    {
        // Label Row

        if (record.is_symbol())
        {
            append_whitespace_inplace(buffers.lines[line_idx], bytes_len);
            ++line_idx;
        }

        // Instruction Row

        std::string &line = buffers.lines[line_idx];
        const size_t offset = line.size();
        const AsmInstruction *instruction = record.pair[side];
        if (instruction != nullptr)
        {
            const size_t usable_byte_count = std::min<size_t>(byte_count, instruction->bytes.size());

            for (size_t b = 0; b < usable_byte_count; ++b)
            {
                line += fmt::format("{:02x} ", instruction->bytes[b]);
            }
        }
        pad_whitespace_inplace(line, bytes_len + offset);
        ++line_idx;
    }
    assert(line_idx == buffers.lines.size());
}

void AsmPrinter::append_assembler(
    Buffers &buffers,
    const AsmComparisonRecords &records,
    const Executable &executable,
    const Function &function,
    Side side,
    uint32_t asm_len,
    uint32_t indent_len)
{
    constexpr size_t address_len = 8;

    if (asm_len > 0)
        asm_len += indent_len;

    size_t line_idx = 0;

    for (const AsmComparisonRecord &record : records)
    {
        const AsmInstruction *instruction = record.pair[side];

        // Label Row

        if (instruction != nullptr && instruction->isSymbol)
        {
            std::string &line = buffers.lines[line_idx];
            const size_t offset = line.size();
            append_whitespace_inplace(line, address_len);

            if (asm_len > 0)
            {
                const ExeSymbol *symbol = get_symbol_or_pseudo_symbol(instruction->address, executable, function);
                assert(symbol != nullptr);
                buffers.misc_buf = symbol->name;
                buffers.misc_buf += ':';
                truncate_inplace(buffers.misc_buf, asm_len);
                line.append(buffers.misc_buf);
            }
            pad_whitespace_inplace(line, address_len + asm_len + offset);
            ++line_idx;
        }
        else if (record.is_symbol())
        {
            append_whitespace_inplace(buffers.lines[line_idx], address_len + asm_len);
            ++line_idx;
        }

        // Instruction Row

        std::string &line = buffers.lines[line_idx];
        const size_t offset = line.size();
        if (instruction != nullptr)
        {
            line.append(fmt::format("{:08x}", instruction->address));
            assert(line.size() - offset == address_len);

            if (asm_len > 0)
            {
                buffers.misc_buf.assign(to_string(*instruction, indent_len));
                truncate_inplace(buffers.misc_buf, asm_len);
                line.append(buffers.misc_buf);
            }
        }
        pad_whitespace_inplace(line, address_len + asm_len + offset);
        ++line_idx;
    }
    assert(line_idx == buffers.lines.size());
}

void AsmPrinter::append_comparison(
    Buffers &buffers,
    const AsmComparisonRecords &records,
    AsmMatchStrictness match_strictness)
{
    const size_t count = records.size();
    size_t line_idx = 0;

    for (const AsmComparisonRecord &record : records)
    {
        // Label Row

        if (record.is_symbol())
        {
            append_whitespace_inplace(buffers.lines[line_idx], AsmMatchValueStringArray[0].size() + 2);
            ++line_idx;
        }

        // Instruction Row

        const AsmMatchValueEx match_value = record.mismatch_info.get_match_value_ex(match_strictness);
        std::string &line = buffers.lines[line_idx];
        line.push_back(' ');
        line.append(AsmMatchValueStringArray[size_t(match_value)]);
        line.push_back(' ');
        ++line_idx;
    }
    assert(line_idx == buffers.lines.size());
}

void AsmPrinter::truncate_inplace(std::string &str, size_t max_len)
{
    if (str.size() > max_len)
    {
        str.resize(max_len);
        // End string on 2 dots to clarify that it has been truncated.
        for (int i = 0; max_len != 0 && i < 2; ++i, --max_len)
            str[max_len - 1] = '.';
    }
}

void AsmPrinter::front_truncate_inplace(std::string &str, size_t max_len)
{
    if (str.size() > max_len)
    {
        str.erase(0, str.size() - max_len);
        // Begin string on 2 dots to clarify that it has been truncated.
        for (int i = 0; max_len != 0 && i < 2; ++i, --max_len)
            str[i] = '.';
    }
}

void AsmPrinter::pad_whitespace_inplace(std::string &str, size_t len)
{
    if (str.size() < len)
    {
        const size_t pad_count = len - str.size();
        append_whitespace_inplace(str, pad_count);
    }
}

void AsmPrinter::append_whitespace_inplace(std::string &str, size_t len)
{
    str.insert(str.end(), len, ' ');
}

} // namespace unassemblize
