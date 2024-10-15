/**
 * @file
 *
 * @brief Data to store relevant data from executable files
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace unassemblize
{
using Address64T = uint64_t;
using Address32T = uint32_t;
using IndexT = uint32_t;

enum ExeSectionType
{
    SECTION_DATA,
    SECTION_CODE,
};

struct ExeSectionInfo
{
    const uint8_t *data;
    Address64T address;
    uint64_t size;
    ExeSectionType type;
};

struct ExeSymbol
{
    std::string name;
    Address64T address = 0;
    uint64_t size = 0;
};

struct ExeObjectSection
{
    std::string name;
    Address64T offset;
    uint64_t size;
};

struct ExeObject
{
    std::string name;
    std::list<ExeObjectSection> sections; // TODO: vector
};

struct ExeImageData
{
    Address64T imageBase = 0; // Default image base address if the ASLR is not enabled.
    Address64T imageEnd = 0; // Image end address.
    uint32_t codeAlignment = sizeof(uint32_t);
    uint32_t dataAlignment = sizeof(uint32_t);
    uint8_t codePad = 0x90; // NOP
    uint8_t dataPad = 0x00;
};

using ExeSectionMap = std::map<std::string, ExeSectionInfo>; // TODO: unordered_map maybe
using ExeSymbols = std::vector<ExeSymbol>;
using ExeObjects = std::list<ExeObject>; // TODO: vector

} // namespace unassemblize