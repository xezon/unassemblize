/**
 * @file
 *
 * @brief Types to store relevant symbols from PDB files
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "executabletypes.h"
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace unassemblize
{
// A copy of DIA2's CV_SourceChksum_t
enum class CV_Chksum
{
    CHKSUM_TYPE_NONE = 0, // indicates no checksum is available
    CHKSUM_TYPE_MD5,
    CHKSUM_TYPE_SHA1,
    CHKSUM_TYPE_SHA_256,
};

// A copy of DIA2's CV_call_e
enum class CV_Call
{
    CV_CALL_NEAR_C = 0x00, // near right to left push, caller pops stack
    CV_CALL_FAR_C = 0x01, // far right to left push, caller pops stack
    CV_CALL_NEAR_PASCAL = 0x02, // near left to right push, callee pops stack
    CV_CALL_FAR_PASCAL = 0x03, // far left to right push, callee pops stack
    CV_CALL_NEAR_FAST = 0x04, // near left to right push with regs, callee pops stack
    CV_CALL_FAR_FAST = 0x05, // far left to right push with regs, callee pops stack
    CV_CALL_SKIPPED = 0x06, // skipped (unused) call index
    CV_CALL_NEAR_STD = 0x07, // near standard call
    CV_CALL_FAR_STD = 0x08, // far standard call
    CV_CALL_NEAR_SYS = 0x09, // near sys call
    CV_CALL_FAR_SYS = 0x0a, // far sys call
    CV_CALL_THISCALL = 0x0b, // this call (this passed in register)
    CV_CALL_MIPSCALL = 0x0c, // Mips call
    CV_CALL_GENERIC = 0x0d, // Generic call sequence
    CV_CALL_ALPHACALL = 0x0e, // Alpha call
    CV_CALL_PPCCALL = 0x0f, // PPC call
    CV_CALL_SHCALL = 0x10, // Hitachi SuperH call
    CV_CALL_ARMCALL = 0x11, // ARM call
    CV_CALL_AM33CALL = 0x12, // AM33 call
    CV_CALL_TRICALL = 0x13, // TriCore Call
    CV_CALL_SH5CALL = 0x14, // Hitachi SuperH-5 call
    CV_CALL_M32RCALL = 0x15, // M32R Call
    CV_CALL_CLRCALL = 0x16, // clr call
    CV_CALL_INLINE = 0x17, // Marker for routines always inlined and thus lacking a convention
    CV_CALL_NEAR_VECTOR = 0x18, // near left to right push with regs, callee pops stack
    CV_CALL_SWIFT = 0x19, // Swift calling convention
    CV_CALL_RESERVED = 0x20 // first unused call enumeration

    // Do NOT add any more machine specific conventions.  This is to be used for
    // calling conventions in the source only (e.g. __cdecl, __stdcall).
};

// Only absVirtual is used currently. Perhaps remove the others variables.
struct PdbAddress
{
    uint32_t section_as_index() const { return section - 1; }

    Address64T absVirtual = ~Address64T(0); // Position of the function within the executable
    Address32T relVirtual = ~Address32T(0); // Relative position of the function within its block
    uint32_t section = 0; // Section part of location
    Address32T offset = 0; // Offset part of location
};

struct PdbSourceLineInfo
{
    uint16_t lineNumber = 0; // Line number in document
    uint16_t offset = 0; // Offset from function address
    uint16_t length = 0; // Length of asm bytes
};
using PdbSourceLineInfoVector = std::vector<PdbSourceLineInfo>;

// #TODO: Use just one name string?
struct PdbSymbolInfo
{
    PdbAddress address;
    uint32_t length = 0;

    std::string decoratedName;
    std::string undecoratedName;
    std::string globalName;
};
using PdbSymbolInfoVector = std::vector<PdbSymbolInfo>;

// #TODO: Use just one name string?
struct PdbFunctionInfo
{
    bool has_valid_source_file_id() const { return sourceFileId != -1; }

    IndexT sourceFileId = -1; // Synonymous for index, can be -1, does not match DIA2 index
    IndexT compilandId = -1; // Synonymous for index, can not be -1, matches DIA2 index

    PdbAddress address;
    PdbAddress debugStartAddress;
    PdbAddress debugEndAddress;
    uint32_t length = 0;
    CV_Call call = CV_Call(-1);

    std::string decoratedName; // ?nameToLowercaseKey@NameKeyGenerator@@QAE?AW4NameKeyType@@PBD@Z
    std::string undecoratedName; // public: enum NameKeyType __thiscall NameKeyGenerator::nameToLowercaseKey(char const *)
    std::string globalName; // NameKeyGenerator::nameToLowercaseKey

    PdbSourceLineInfoVector sourceLines;
};
using PdbFunctionInfoVector = std::vector<PdbFunctionInfo>;

struct PdbSourceFileInfo
{
    std::string name;
    CV_Chksum checksumType = CV_Chksum::CHKSUM_TYPE_NONE;
    std::vector<uint8_t> checksum;
    std::vector<IndexT> compilandIds; // Synonymous for indices
    std::vector<IndexT> functionIds; // Synonymous for indices
};
using PdbSourceFileInfoVector = std::vector<PdbSourceFileInfo>;

struct PdbCompilandInfo
{
    std::string name; // File name of the compiland's object file.
    std::vector<IndexT> sourceFileIds; // Synonymous for indices
    std::vector<IndexT> functionIds; // Synonymous for indices
};
using PdbCompilandInfoVector = std::vector<PdbCompilandInfo>;

struct PdbExeInfo
{
    std::string exeFileName; // Name of the .exe file.
    std::string pdbFilePath; // Full path for the .exe file's .pdb or .dbg file.
};

void to_json(nlohmann::json &js, const PdbAddress &d);
void from_json(const nlohmann::json &js, PdbAddress &d);

void to_json(nlohmann::json &js, const PdbSourceLineInfo &d);
void from_json(const nlohmann::json &js, PdbSourceLineInfo &d);

void to_json(nlohmann::json &js, const PdbCompilandInfo &d);
void from_json(const nlohmann::json &js, PdbCompilandInfo &d);

void to_json(nlohmann::json &js, const PdbSourceFileInfo &d);
void from_json(const nlohmann::json &js, PdbSourceFileInfo &d);

void to_json(nlohmann::json &js, const PdbSymbolInfo &d);
void from_json(const nlohmann::json &js, PdbSymbolInfo &d);

void to_json(nlohmann::json &js, const PdbFunctionInfo &d);
void from_json(const nlohmann::json &js, PdbFunctionInfo &d);

void to_json(nlohmann::json &js, const PdbExeInfo &d);
void from_json(const nlohmann::json &js, PdbExeInfo &d);

// Can pass PdbSymbolInfo or PdbFunctionInfo
template<class T>
const std::string &to_exe_symbol_name(const T &pdb_symbol)
{
    if (!pdb_symbol.decoratedName.empty())
    {
        return pdb_symbol.decoratedName;
    }
    else if (!pdb_symbol.globalName.empty())
    {
        return pdb_symbol.globalName;
    }
    else
    {
        return pdb_symbol.undecoratedName;
    }
}

// Can pass PdbSymbolInfo or PdbFunctionInfo
template<class T>
ExeSymbol to_exe_symbol(const T &pdb_symbol)
{
    ExeSymbol exe_symbol;
    exe_symbol.name = to_exe_symbol_name(pdb_symbol);
    exe_symbol.address = pdb_symbol.address.absVirtual;
    exe_symbol.size = pdb_symbol.length;
    return exe_symbol;
}

template<class Iterator>
ExeSymbols to_exe_symbols(Iterator begin, Iterator end)
{
    const size_t size = std::distance(begin, end);
    ExeSymbols symbols;
    symbols.reserve(size);
    for (; begin != end; ++begin)
    {
        symbols.emplace_back(std::move(to_exe_symbol(*begin)));
    }
    return symbols;
}

} // namespace unassemblize
