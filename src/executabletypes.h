/**
 * @file
 *
 * @brief Types to store relevant data from executable files
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "commontypes.h"
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace unassemblize
{
enum class ExeSectionType
{
    Code,
    Data,

    Unknown,
};

ExeSectionType to_section_type(std::string_view str);
const char *to_string(ExeSectionType type);

struct ExeSectionInfo
{
    std::string name;
    const uint8_t *data = nullptr;
    Address64T address = 0; // Position of the section within the executable.
    uint64_t size = 0;
    ExeSectionType type = ExeSectionType::Unknown;
};

struct ExeSymbol
{
    std::string name;
    Address64T address = 0; // Position of the symbol within the executable.
    uint64_t size = 0;
};

struct ExeObjectSection
{
    std::string name;
    Address64T offset = 0; // Position of the object within the executable.
    uint64_t size = 0;
};

struct ExeObject
{
    std::string name;
    std::vector<ExeObjectSection> sections;
};

struct ExeImageData
{
    Address64T imageBase = 0; // Default image base address if the ASLR is not enabled.
    Address64T sectionsBegin = ~Address64T(0); // Begin address of first section within the executable.
    Address64T sectionsEnd = 0; // End address of last section within the executable.
    uint32_t codeAlignment = sizeof(uint32_t);
    uint32_t dataAlignment = sizeof(uint32_t);
    uint8_t codePad = 0x90; // NOP
    uint8_t dataPad = 0x00;
};

using ExeSections = std::vector<ExeSectionInfo>;
using ExeSymbols = std::vector<ExeSymbol>;
using ExeObjects = std::vector<ExeObject>;

} // namespace unassemblize
