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
    assert(size <= bytes.elements.size());
    const size_t max_bytes = std::min<size_t>(bytes.elements.size(), size);
    memcpy(bytes.elements.data(), p, max_bytes);
    bytes.size = uint8_t(max_bytes);
}

} // namespace unassemblize
