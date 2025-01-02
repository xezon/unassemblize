/**
 * @file
 *
 * @brief Class to extract relevant symbols from PDB files
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "pdbreadertypes.h"

#ifdef _WIN32
#define PDB_READER_WIN32
#endif

#ifdef PDB_READER_WIN32
struct IDiaDataSource;
struct IDiaEnumSourceFiles;
struct IDiaLineNumber;
struct IDiaSession;
struct IDiaSourceFile;
struct IDiaSymbol;
#endif

namespace unassemblize
{
class PdbReader
{
public:
    PdbReader();

    void set_verbose(bool verbose) { m_verbose = verbose; }

    bool load(const std::string &pdb_filename);
    void unload();

    bool is_loaded() const { return !m_pdbFilename.empty(); }
    const std::string &get_filename() const { return m_pdbFilename; }
    const PdbCompilandInfoVector &get_compilands() const { return m_compilands; }
    const PdbSourceFileInfoVector &get_source_files() const { return m_sourceFiles; }
    const PdbSymbolInfoVector &get_symbols() const { return m_symbols; }
    const PdbFunctionInfoVector &get_functions() const { return m_functions; }
    const PdbExeInfo &get_exe_info() const { return m_exe; }

    const PdbFunctionInfo *find_function_by_address(Address64T address) const;

    void load_json(const nlohmann::json &js);
    bool load_config(const std::string &file_name);
    void save_json(nlohmann::json &js, bool overwrite_sections = false) const;
    bool save_config(const std::string &file_name, bool overwrite_sections = false) const;

private:
    void build_function_address_to_index_map();

#ifdef PDB_READER_WIN32
    bool load_dia(const std::string &pdb_file);
    void unload_dia();

    bool read_symbols();
    bool read_global_scope();

    IDiaEnumSourceFiles *get_enum_source_files();
    bool read_source_files();
    void read_source_file_initial(IDiaSourceFile *pSourceFile);

    bool read_compilands();
    void read_compiland_symbol(PdbCompilandInfo &compilandInfo, IndexT compilandId, IDiaSymbol *pSymbol);
    void read_compiland_function(PdbFunctionInfo &functionInfo, IndexT functionId, IDiaSymbol *pSymbol);
    void read_compiland_function_start(PdbFunctionInfo &functionInfo, IDiaSymbol *pSymbol);
    void read_compiland_function_end(PdbFunctionInfo &functionInfo, IDiaSymbol *pSymbol);

    void read_public_function(PdbFunctionInfo &functionInfo, IDiaSymbol *pSymbol);

    void read_source_file_for_compiland(PdbCompilandInfo &compilandInfo, IDiaSourceFile *pSourceFile);
    void read_source_file_for_function(PdbFunctionInfo &functionInfo, IndexT functionId, IDiaSourceFile *pSourceFile);
    void read_line(PdbFunctionInfo &function_info, IDiaLineNumber *pLine);

    bool read_publics();
    bool read_globals();
    void read_common_symbol(PdbSymbolInfo &symbolInfo, IDiaSymbol *pSymbol);
    void read_public_symbol(PdbSymbolInfo &symbolInfo, IDiaSymbol *pSymbol);
    void read_global_symbol(PdbSymbolInfo &symbolInfo, IDiaSymbol *pSymbol);

    // This function decides on one of the input names by a fixed rule.
    const std::string &get_relevant_symbol_name(const std::string &name1, const std::string &name2);
    bool add_or_update_symbol(PdbSymbolInfo &&symbolInfo);
#endif

private:
#ifdef PDB_READER_WIN32
    // Intermediate structures used during Pdb read.
    // #TODO: Move away into struct and pass to functions?

    StringToIndexMapT m_sourceFileNameToIndexMap;
    Address64ToIndexMapT m_symbolAddressToIndexMap;
    IDiaDataSource *m_pDiaSource = nullptr;
    IDiaSession *m_pDiaSession = nullptr;
    IDiaSymbol *m_pDiaSymbol = nullptr;
    uint32_t m_dwMachineType = 0;
    bool m_coInitialized = false;
#endif
    bool m_verbose = false;

    // Persistent structures created after Pdb read.

    std::string m_pdbFilename;
    // Compilands indices match DIA2 indices.
    PdbCompilandInfoVector m_compilands;
    // Source Files indices do not match DIA2 indices (aka "unique id").
    PdbSourceFileInfoVector m_sourceFiles;
    PdbFunctionInfoVector m_functions;
    Address64ToIndexMapT m_functionAddressToIndexMap;
    // Symbols contains every public and global symbol, including functions.
    PdbSymbolInfoVector m_symbols;
    PdbExeInfo m_exe;
};

} // namespace unassemblize
