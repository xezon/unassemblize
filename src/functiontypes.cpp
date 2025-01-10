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
#include "functiontypes.h"
#include "util.h"
#include <assert.h>

namespace unassemblize
{
AsmFormat to_asm_format(std::string_view str)
{
    if (util::equals_nocase(str, "igas"))
    {
        return AsmFormat::IGAS;
    }
    else if (util::equals_nocase(str, "agas"))
    {
        return AsmFormat::AGAS;
    }
    else if (util::equals_nocase(str, "masm"))
    {
        return AsmFormat::MASM;
    }
    else if (util::equals_nocase(str, "default"))
    {
        return AsmFormat::DEFAULT;
    }
    else
    {
        printf("Unrecognized asm format '%.*s'. Defaulting to 'DEFAULT'", PRINTF_STRING(str));
        return AsmFormat::DEFAULT;
    }
    static_assert(size_t(AsmFormat::DEFAULT) == 3, "Enum was changed. Update switch case.");
}

void AsmInstruction::set_bytes(const uint8_t *p, size_t size)
{
    assert(size <= bytes.max_size());
    const size_t max_bytes = std::min<size_t>(bytes.max_size(), size);
    memcpy(bytes.data(), p, max_bytes);
    bytes.set_size(static_cast<uint8_t>(max_bytes));
}

std::optional<ptrdiff_t> get_instruction_distance(
    const AsmInstructions &instructions,
    Address64T address1,
    Address64T address2)
{
    AsmInstructions::const_iterator it1 = std::lower_bound(instructions.begin(), instructions.end(), address1);
    if (it1 != instructions.end() && it1->address == address1)
    {
        AsmInstructions::const_iterator it2 = std::lower_bound(instructions.begin(), instructions.end(), address2);
        if (it2 != instructions.end() && it2->address == address2)
        {
            return std::distance(it1, it2);
        }
    }
    return std::nullopt;
}

InstructionTextArray split_instruction_text(std::string_view text)
{
    InstructionTextArray arr;
    size_t index = 0;
    size_t wordLen;
    char wordSeperator = ' ';
    bool in_quote = false;
    const char *textEnd = text.data() + text.size();
    const char *wordBegin = text.data();
    const char *c = text.data();

    while (c != textEnd)
    {
        if (*c == '\"')
        {
            // Does not look for separator inside quoted text.
            in_quote = !in_quote;
        }
        else if (!in_quote && *c == wordSeperator)
        {
            // Lock word.
            wordLen = static_cast<size_t>(c - wordBegin);
            assert(wordLen != 0);
            arr[index] = {wordBegin, wordLen};
            // Change word separator for operands.
            wordSeperator = ',';
            // Skip separator
            ++c;
            // Omit spaces between operands.
            while (*c == ' ')
                ++c;
            // Increment word index.
            ++index;
            assert(index < arr.max_size());
            // Store new word begin.
            wordBegin = c;
            continue;
        }
        ++c;
    }

    wordLen = static_cast<size_t>(c - wordBegin);
    assert(wordLen != 0);
    arr[index] = {wordBegin, wordLen};
    arr.set_size(index + 1);
    return arr;
}

} // namespace unassemblize
