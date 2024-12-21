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
void AsmPrinter::append_to_string(std::string &str, const AsmInstructionVariants &instructions, uint32_t indent_len)
{
    constexpr std::string_view eol = "\n";

    // The first variant is expected to be a label if it is the begin of a function.
    std::string name;
    if (const AsmLabel *label = std::get_if<AsmLabel>(&instructions[0]))
    {
        name = label->label;
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

    for (const AsmInstructionVariant &variant : instructions)
    {
        if (const AsmInstruction *instruction = std::get_if<AsmInstruction>(&variant))
        {
            str += to_string(*instruction, indent_len);
        }
        else if (const AsmLabel *label = std::get_if<AsmLabel>(&variant))
        {
            str += to_string(*label);
        }

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

std::string AsmPrinter::to_string(const AsmLabel &label)
{
    return fmt::format("{:s}:", label.label);
}

void AsmPrinter::append_to_string(
    std::string &str,
    const AsmComparisonResult &comparison,
    const StringPair &exe_filenames,
    const TextFileContentPair &source_file_texts,
    AsmMatchStrictness match_strictness,
    uint32_t indent_len,
    uint32_t asm_len,
    uint32_t byte_count,
    uint32_t sourcecode_len,
    uint32_t sourceline_len)
{
    if (comparison.records.empty())
    {
        return;
    }

    constexpr std::string_view eol = "\n";
    constexpr size_t end_eol_count = 4;

    // Keeps buffer allocated for reuse.
    m_buffers.lines.clear();
    m_buffers.lines.resize(comparison.records.size());
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
        size_t i = 0;

        if (source_len > 0 && source_file_texts.pair[i] != nullptr)
        {
            source_code_regions[i].begin = m_buffers.lines[0].size();
            append_source_code(m_buffers, comparison.records, *source_file_texts.pair[i], i, sourcecode_len, sourceline_len);
            source_code_regions[i].end = m_buffers.lines[0].size();
        }

        if (byte_count > 0)
        {
            append_bytes(m_buffers, comparison.records, i, byte_count);
        }

        assembler_regions[i].begin = m_buffers.lines[0].size();
        append_assembler(m_buffers, comparison.records, i, asm_len, indent_len);
        assembler_regions[i].end = m_buffers.lines[0].size();

        append_comparison(m_buffers, comparison.records, match_strictness);
        ++i;

        assembler_regions[i].begin = m_buffers.lines[0].size();
        append_assembler(m_buffers, comparison.records, i, asm_len, indent_len);
        assembler_regions[i].end = m_buffers.lines[0].size();

        if (byte_count > 0)
        {
            append_bytes(m_buffers, comparison.records, i, byte_count);
        }

        if (source_len > 0 && source_file_texts.pair[i] != nullptr)
        {
            source_code_regions[i].begin = m_buffers.lines[0].size();
            append_source_code(m_buffers, comparison.records, *source_file_texts.pair[i], i, sourcecode_len, sourceline_len);
            source_code_regions[i].end = m_buffers.lines[0].size();
        }
    }

    {
        // Create misc info.
        m_buffers.misc_buf.clear();

        // The first line is expected to be a label if it is the begin of a function.
        const AsmLabelPair *label = std::get_if<AsmLabelPair>(&comparison.records[0]);
        const std::string name = label != nullptr ? label->pair[0]->label : "_unknown_";

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
            std::string filename_copy = exe_filenames.pair[i];
            front_truncate_inplace(filename_copy, region.end - region.begin);
            m_buffers.misc_buf += filename_copy;
            pad_whitespace_inplace(m_buffers.misc_buf, region.end);
        }

        {
            const LineRegion &region = source_code_regions[1];

            if (region.begin < region.end)
            {
                pad_whitespace_inplace(m_buffers.misc_buf, region.begin);
                std::string filename_copy = exe_filenames.pair[1];
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
    size_t side_idx,
    uint32_t sourcecode_len,
    uint32_t sourceline_len)
{
    assert(buffers.lines.size() == records.size());

    if (sourceline_len > 0)
        sourceline_len += 1; // +1 for colon.

    const size_t count = records.size();

    for (size_t i = 0; i < count; ++i)
    {
        const AsmComparisonRecord &record = records[i];
        std::string &line = buffers.lines[i];
        const size_t offset = line.size();

        if (const AsmInstructionPair *instruction_pair = std::get_if<AsmInstructionPair>(&record))
        {
            const AsmInstruction *instruction = instruction_pair->pair[side_idx];
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
        }
        else if (const AsmLabelPair *label_pair = std::get_if<AsmLabelPair>(&record))
        {
            // Does nothing.
        }
        else
        {
            assert(false);
        }

        pad_whitespace_inplace(line, sourceline_len + sourcecode_len + offset);
    }
}

void AsmPrinter::append_bytes(Buffers &buffers, const AsmComparisonRecords &records, size_t side_idx, uint32_t byte_count)
{
    assert(buffers.lines.size() == records.size());

    byte_count = std::min<uint32_t>(byte_count, AsmInstruction::BytesArray::MaxSize);
    const size_t bytes_len = byte_count * (2 + 1);
    const size_t count = records.size();

    for (size_t i = 0; i < count; ++i)
    {
        const AsmComparisonRecord &record = records[i];
        std::string &line = buffers.lines[i];
        const size_t offset = line.size();

        if (const AsmInstructionPair *instruction_pair = std::get_if<AsmInstructionPair>(&record))
        {
            const AsmInstruction *instruction = instruction_pair->pair[side_idx];
            if (instruction != nullptr)
            {
                const size_t usable_byte_count = std::min<size_t>(byte_count, instruction->bytes.size);

                for (size_t b = 0; b < usable_byte_count; ++b)
                {
                    line += fmt::format("{:02x} ", instruction->bytes.elements[b]);
                }
            }
        }
        else if (const AsmLabelPair *label_pair = std::get_if<AsmLabelPair>(&record))
        {
            // Does nothing.
        }
        else
        {
            assert(false);
        }

        pad_whitespace_inplace(line, bytes_len + offset);
    }
}

void AsmPrinter::append_assembler(
    Buffers &buffers,
    const AsmComparisonRecords &records,
    size_t side_idx,
    uint32_t asm_len,
    uint32_t indent_len)
{
    constexpr size_t address_len = 8;

    assert(buffers.lines.size() == records.size());

    if (asm_len > 0)
        asm_len += indent_len;

    const size_t count = records.size();

    for (size_t i = 0; i < count; ++i)
    {
        const AsmComparisonRecord &record = records[i];
        std::string &line = buffers.lines[i];
        const size_t offset = line.size();

        if (const AsmInstructionPair *instruction_pair = std::get_if<AsmInstructionPair>(&record))
        {
            const AsmInstruction *instruction = instruction_pair->pair[side_idx];
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
        }
        else if (const AsmLabelPair *label_pair = std::get_if<AsmLabelPair>(&record))
        {
            const AsmLabel *label = label_pair->pair[side_idx];
            if (label != nullptr)
            {
                append_whitespace_inplace(line, address_len);

                if (asm_len > 0)
                {
                    buffers.misc_buf.assign(to_string(*label));
                    truncate_inplace(buffers.misc_buf, asm_len);
                    line.append(buffers.misc_buf);
                }
            }
        }
        else
        {
            assert(false);
        }

        pad_whitespace_inplace(line, address_len + asm_len + offset);
    }
}

void AsmPrinter::append_comparison(
    Buffers &buffers,
    const AsmComparisonRecords &records,
    AsmMatchStrictness match_strictness)
{
    constexpr std::string_view equal = " == ";
    constexpr std::string_view unequal = " xx ";
    constexpr std::string_view maybe_equal = " ?? ";
    constexpr std::string_view left_missing = " >> ";
    constexpr std::string_view right_missing = " << ";
    static_assert(equal.size() == unequal.size(), "Expects same length");
    static_assert(equal.size() == maybe_equal.size(), "Expects same length");
    static_assert(equal.size() == left_missing.size(), "Expects same length");
    static_assert(equal.size() == right_missing.size(), "Expects same length");

    assert(buffers.lines.size() == records.size());

    const size_t count = records.size();

    for (size_t i = 0; i < count; ++i)
    {
        const AsmComparisonRecord &record = records[i];
        std::string &line = buffers.lines[i];

        if (const AsmInstructionPair *instruction_pair = std::get_if<AsmInstructionPair>(&record))
        {
            const AsmMatchValue match_value = instruction_pair->mismatch_info.get_match_value(match_strictness);

            switch (match_value)
            {
                case AsmMatchValue::IsMatch:
                    line.append(equal);
                    break;

                case AsmMatchValue::IsMaybeMatch:
                    line.append(maybe_equal);
                    break;

                case AsmMatchValue::IsMismatch: {
                    const AsmInstruction *instruction0 = instruction_pair->pair[0];
                    const AsmInstruction *instruction1 = instruction_pair->pair[1];
                    if (instruction0 != nullptr && instruction1 != nullptr)
                    {
                        line.append(unequal);
                    }
                    else if (instruction0 == nullptr && instruction1 != nullptr)
                    {
                        line.append(left_missing);
                    }
                    else if (instruction0 != nullptr && instruction1 == nullptr)
                    {
                        line.append(right_missing);
                    }
                    else
                    {
                        assert(false);
                    }
                    break;
                }
                default:
                    assert(false);
                    break;
            }
        }
        else if (const AsmLabelPair *label_pair = std::get_if<AsmLabelPair>(&record))
        {
            append_whitespace_inplace(line, equal.size());
        }
        else
        {
            assert(false);
        }
    }
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
