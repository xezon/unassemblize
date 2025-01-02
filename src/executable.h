/**
 * @file
 *
 * @brief Class encapsulating the executable being disassembled.
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
#include "functiontypes.h"
#include "pdbreadertypes.h"
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <stdio.h>

namespace LIEF
{
class Binary;
}

namespace unassemblize
{
class Executable
{
public:
    Executable();
    ~Executable();

    void set_verbose(bool verbose) { m_verbose = verbose; }

    bool load(const std::string &exe_filename);
    void unload();

    bool load_config(const char *file_name, bool overwrite_symbols = false);
    bool save_config(const char *file_name) const;

    bool is_loaded() const;
    const std::string &get_filename() const;
    const ExeSections &get_sections() const;
    const ExeSectionInfo *find_section(Address64T address) const;
    const ExeSectionInfo *find_section(const std::string &name) const;
    const ExeSectionInfo *get_code_section() const;

    Address64T image_base() const; // Default image base address if the ASLR is not enabled.
    Address64T code_section_begin_from_image_base() const; // Begin address of .text section plus image base.
    Address64T code_section_end_from_image_base() const; // End address of .text section plus image base.
    Address64T all_sections_begin_from_image_base() const; // Begin address of first section plus image base.
    Address64T all_sections_end_from_image_base() const; // End address of last section plus image base.

    const ExeSymbols &get_symbols() const;
    const ExeSymbol *get_symbol(Address64T address) const;
    const ExeSymbol *get_symbol(const std::string &name) const;
    const ExeSymbol *get_symbol_from_image_base(Address64T address) const; // Adds the image base before symbol lookup.

    // Adds series of new symbols if not already present.
    void add_symbols(const ExeSymbols &symbols, bool overwrite = false);
    void add_symbols(const PdbSymbolInfoVector &symbols, bool overwrite = false);

    // Adds new symbol if not already present.
    void add_symbol(const ExeSymbol &symbol, bool overwrite = false);

private:
    ExeSectionInfo *find_section(const std::string &name);

    void load_symbols(nlohmann::json &js, bool overwrite_symbols);
    void dump_symbols(nlohmann::json &js) const;

    void load_sections(nlohmann::json &js);
    void dump_sections(nlohmann::json &js) const;

    void load_objects(nlohmann::json &js);
    void dump_objects(nlohmann::json &js) const;

private:
    std::string m_exeFilename;
    std::unique_ptr<LIEF::Binary> m_binary;

    ExeSections m_sections;
    IndexT m_codeSectionIdx = ~IndexT(0);

    bool m_verbose = false;

    ExeSymbols m_symbols;
    Address64ToIndexMapT m_symbolAddressToIndexMap;

    // Using multimap, because there can be multiple symbols sharing the same name.
    MultiStringToIndexMapT m_symbolNameToIndexMap;

    ExeObjects m_targetObjects;

    ExeImageData m_imageData;
};

} // namespace unassemblize
