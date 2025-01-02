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
#include "pdbreader.h"
#include "util.h"
#include <array>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#ifdef PDB_READER_WIN32
#include <dia2.h>
#endif

namespace unassemblize
{
const char *const s_compilands = "pdb_compilands";
const char *const s_sourceFiles = "pdb_source_files";
const char *const s_functions = "pdb_functions";
const char *const s_symbols = "pdb_symbols";
const char *const s_exe = "pdb_exe";

PdbReader::PdbReader()
#ifdef PDB_READER_WIN32
    :
    m_dwMachineType(CV_CFL_80386)
#endif
{
}

bool PdbReader::load(const std::string &pdb_filename)
{
    unload();

    bool success = false;

    const std::string full_path = util::abs_path(pdb_filename);

#ifdef PDB_READER_WIN32
    if (load_dia(full_path))
    {
        success = read_symbols();

        if (success)
        {
            m_pdbFilename = full_path;
        }
    }
    unload_dia();
#endif

    return success;
}

void PdbReader::unload()
{
    util::free_container(m_pdbFilename);
    util::free_container(m_compilands);
    util::free_container(m_sourceFiles);
    util::free_container(m_functions);
    util::free_container(m_symbols);
    util::free_container(m_functionAddressToIndexMap);
    m_exe = PdbExeInfo();
}

const PdbFunctionInfo *PdbReader::find_function_by_address(Address64T address) const
{
    Address64ToIndexMapT::const_iterator it = m_functionAddressToIndexMap.find(address);
    if (it != m_functionAddressToIndexMap.cend())
    {
        return &m_functions[it->second];
    }
    return nullptr;
}

void PdbReader::load_json(const nlohmann::json &js)
{
    js.at(s_compilands).get_to(m_compilands);
    js.at(s_sourceFiles).get_to(m_sourceFiles);
    js.at(s_symbols).get_to(m_symbols);
    js.at(s_functions).get_to(m_functions);
    js.at(s_exe).get_to(m_exe);

    build_function_address_to_index_map();
}

bool PdbReader::load_config(const std::string &file_name)
{
    if (m_verbose)
    {
        printf("Loading config file '%s'...\n", file_name.c_str());
    }

    nlohmann::json js;

    {
        std::ifstream fs(file_name);

        if (fs.fail())
        {
            return false;
        }

        js = nlohmann::json::parse(fs);
    }

    load_json(js);

    return true;
}

void PdbReader::save_json(nlohmann::json &js, bool overwrite_sections) const
{
    // Don't dump if we already have sections for these.

    if (overwrite_sections || js.find(s_compilands) == js.end())
    {
        js[s_compilands] = m_compilands;
    }
    if (overwrite_sections || js.find(s_sourceFiles) == js.end())
    {
        js[s_sourceFiles] = m_sourceFiles;
    }
    if (overwrite_sections || js.find(s_symbols) == js.end())
    {
        js[s_symbols] = m_symbols;
    }
    if (overwrite_sections || js.find(s_functions) == js.end())
    {
        js[s_functions] = m_functions;
    }
    if (overwrite_sections || js.find(s_exe) == js.end())
    {
        js[s_exe] = m_exe;
    }
}

bool PdbReader::save_config(const std::string &file_name, bool overwrite_sections) const
{
    if (m_verbose)
    {
        printf("Saving config file '%s'...\n", file_name.c_str());
    }

    nlohmann::json js;

    // Parse the config file if it already exists and update it.
    {
        std::ifstream fs(file_name);

        if (!fs.fail())
        {
            js = nlohmann::json::parse(fs);
        }
    }

    save_json(js, overwrite_sections);

    std::ofstream fs(file_name);
    fs << std::setw(2) << js << std::endl;

    return !fs.fail();
}

void PdbReader::build_function_address_to_index_map()
{
    const size_t size = m_functions.size();
    m_functionAddressToIndexMap.reserve(size);
    for (IndexT i = 0; i < size; ++i)
    {
        [[maybe_unused]] auto [_, added] = m_functionAddressToIndexMap.try_emplace(m_functions[i].address.absVirtual, i);
        assert(added);
    }
}

#ifdef PDB_READER_WIN32

bool PdbReader::load_dia(const std::string &pdb_file)
{
    unload_dia();

    HRESULT hr = CoInitialize(NULL);

    if (FAILED(hr))
    {
        wprintf(L"CoInitialize failed - HRESULT = %08X\n", hr);
        return false;
    }

    m_coInitialized = true;

    // Obtain access to the provider

    hr = CoCreateInstance(__uuidof(DiaSource), NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **)&m_pDiaSource);

    if (FAILED(hr))
    {
        wprintf(L"CoCreateInstance failed - HRESULT = %08X\n", hr);
        return false;
    }

    // Open and prepare a program database (.pdb) file as a debug data source

    const std::wstring wfilename = util::to_utf16(pdb_file);

    hr = m_pDiaSource->loadDataFromPdb(wfilename.c_str());

    if (FAILED(hr))
    {
        wprintf(L"loadDataFromPdb failed - HRESULT = %08X\n", hr);
        return false;
    }

    // Open a session for querying symbols

    hr = m_pDiaSource->openSession(&m_pDiaSession);

    if (FAILED(hr))
    {
        wprintf(L"openSession failed - HRESULT = %08X\n", hr);
        return false;
    }

    // Retrieve a reference to the global scope

    hr = m_pDiaSession->get_globalScope(&m_pDiaSymbol);

    if (hr != S_OK)
    {
        wprintf(L"get_globalScope failed\n");
        return false;
    }

    // Set Machine type for getting correct register names

    DWORD dwMachType = 0;
    if (m_pDiaSymbol->get_machineType(&dwMachType) == S_OK)
    {
        switch (dwMachType)
        {
            case IMAGE_FILE_MACHINE_I386:
                m_dwMachineType = CV_CFL_80386;
                break;
            case IMAGE_FILE_MACHINE_IA64:
                m_dwMachineType = CV_CFL_IA64;
                break;
            case IMAGE_FILE_MACHINE_AMD64:
                m_dwMachineType = CV_CFL_AMD64;
                break;
        }
    }

    return true;
}

void PdbReader::unload_dia()
{
    if (m_coInitialized)
    {
        if (m_pDiaSession != nullptr)
        {
            m_pDiaSession->Release();
            m_pDiaSession = nullptr;
        }

        if (m_pDiaSymbol != nullptr)
        {
            m_pDiaSymbol->Release();
            m_pDiaSymbol = nullptr;
        }

        CoUninitialize();
        m_pDiaSource = nullptr;

        m_coInitialized = false;
    }
}

bool PdbReader::read_symbols()
{
    m_functions.reserve(1024 * 100);

    bool ok = true;
    ok = ok && read_global_scope();
    ok = ok && read_publics();
    ok = ok && read_globals();
    ok = ok && read_source_files();
    ok = ok && read_compilands();

    m_functions.shrink_to_fit();
    m_symbols.shrink_to_fit();

    util::free_container(m_sourceFileNameToIndexMap);
    util::free_container(m_symbolAddressToIndexMap);

    build_function_address_to_index_map();

#if 0 // Collects all functions that are missing in public & global symbols.
    std::vector<PdbFunctionInfo *> missingFunctions;
    for (PdbFunctionInfo &function : m_functions) {
        auto it = std::find_if(m_symbols.begin(), m_symbols.end(), [&](const PdbSymbolInfo &symbol) {
            return symbol.decoratedName == function.decoratedName;
        });
        if (it == m_symbols.end()) {
            missingFunctions.push_back(&function);
        }
    }
#endif

    return ok;
}

bool PdbReader::read_global_scope()
{
    {
        BSTR name;
        if (m_pDiaSymbol->get_name(&name) == S_OK)
        {
            m_exe.exeFileName = util::to_utf8(name);
            SysFreeString(name);
        }
    }
    {
        BSTR name;
        if (m_pDiaSymbol->get_symbolsFileName(&name) == S_OK)
        {
            m_exe.pdbFilePath = util::to_utf8(name);
            SysFreeString(name);
        }
    }

    return !m_exe.exeFileName.empty() && !m_exe.pdbFilePath.empty();
}

IDiaEnumSourceFiles *PdbReader::get_enum_source_files()
{
    // https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/idiaenumsourcefiles

    IDiaEnumSourceFiles *pUnknown = NULL;
    REFIID iid = __uuidof(IDiaEnumSourceFiles);
    IDiaEnumTables *pEnumTables = NULL;
    IDiaTable *pTable = NULL;
    ULONG celt = 0;

    if (m_pDiaSession->getEnumTables(&pEnumTables) != S_OK)
    {
        wprintf(L"ERROR - GetTable() getEnumTables\n");
        return NULL;
    }
    while (pEnumTables->Next(1, &pTable, &celt) == S_OK && celt == 1)
    {
        // There is only one table that matches the given iid
        HRESULT hr = pTable->QueryInterface(iid, (void **)&pUnknown);
        pTable->Release();
        if (hr == S_OK)
        {
            break;
        }
    }
    pEnumTables->Release();
    return pUnknown;
}

bool PdbReader::read_source_files()
{
    IDiaEnumSourceFiles *pEnumSourceFiles = get_enum_source_files();
    if (pEnumSourceFiles != nullptr)
    {
        {
            LONG count = 0;
            pEnumSourceFiles->get_Count(&count);
            m_sourceFiles.reserve(count);
            m_sourceFileNameToIndexMap.reserve(count);
        }

        IDiaSourceFile *pSourceFile;
        ULONG celt = 0;

        // Go through all the source files.

        while (SUCCEEDED(pEnumSourceFiles->Next(1, &pSourceFile, &celt)) && (celt == 1))
        {
            read_source_file_initial(pSourceFile);
            pSourceFile->Release();
        }
        pEnumSourceFiles->Release();
        return true;
    }
    return false;
}

void PdbReader::read_source_file_initial(IDiaSourceFile *pSourceFile)
{
    // Note: get_uniqueId returns a random id. Do not trust it.

    const IndexT sourceFileIndex = m_sourceFiles.size();
    m_sourceFiles.emplace_back();
    PdbSourceFileInfo &fileInfo = m_sourceFiles.back();

    assert(fileInfo.name.empty());

    // Populate source file info.
    {
        BSTR name;
        if (pSourceFile->get_fileName(&name) == S_OK)
        {
            fileInfo.name = util::to_utf8(name);
            [[maybe_unused]] auto [_, added] = m_sourceFileNameToIndexMap.try_emplace(fileInfo.name, sourceFileIndex);
            assert(added);
            SysFreeString(name);
        }
    }
    {
        DWORD checksumType = CHKSUM_TYPE_NONE;
        if (pSourceFile->get_checksumType(&checksumType) == S_OK)
        {
            fileInfo.checksumType = static_cast<CV_Chksum>(checksumType);
        }
    }
    {
        BYTE checksum[256];
        DWORD cbChecksum = sizeof(checksum);
        if (pSourceFile->get_checksum(cbChecksum, &cbChecksum, checksum) == S_OK)
        {
            fileInfo.checksum.assign(checksum, checksum + cbChecksum);
        }
    }
    {
        IDiaEnumSymbols *pEnumCompilands;
        if (pSourceFile->get_compilands(&pEnumCompilands) == S_OK)
        {
            {
                LONG count = 0;
                pEnumCompilands->get_Count(&count);
                fileInfo.compilandIds.reserve(count);
            }

            IDiaSymbol *pCompiland;
            ULONG celt = 0;

            while (SUCCEEDED(pEnumCompilands->Next(1, &pCompiland, &celt)) && (celt == 1))
            {
                DWORD indexId;
                if (pCompiland->get_symIndexId(&indexId) == S_OK)
                {
                    fileInfo.compilandIds.push_back(indexId);
                }
            }
        }
    }
}

bool PdbReader::read_compilands()
{
    // Retrieve the compilands.

    IDiaEnumSymbols *pEnumCompilands;

    if (FAILED(m_pDiaSymbol->findChildren(SymTagCompiland, NULL, nsNone, &pEnumCompilands)))
    {
        return false;
    }

    {
        LONG count = 0;
        pEnumCompilands->get_Count(&count);
        m_compilands.reserve(count);
    }

    IDiaSymbol *pCompiland;
    ULONG celt = 0;

    // Go through all the compilands.

    while (SUCCEEDED(pEnumCompilands->Next(1, &pCompiland, &celt)) && (celt == 1))
    {
        const auto compilandId = static_cast<IndexT>(m_compilands.size());
        m_compilands.emplace_back();
        PdbCompilandInfo &compilandInfo = m_compilands.back();

        {
            BSTR name;
            if (pCompiland->get_name(&name) == S_OK)
            {
                compilandInfo.name = util::to_utf8(name);
                SysFreeString(name);
            }
        }

        {
            // Every compiland could contain multiple references to the source files which were used to build it.
            // Retrieve all source files by compiland by passing NULL for the name of the source file.

            IDiaEnumSourceFiles *pEnumSourceFiles;

            if (SUCCEEDED(m_pDiaSession->findFile(pCompiland, NULL, nsNone, &pEnumSourceFiles)))
            {
                IDiaSourceFile *pSourceFile;

                {
                    LONG count = 0;
                    pEnumSourceFiles->get_Count(&count);
                    compilandInfo.sourceFileIds.reserve(count);
                }

                while (SUCCEEDED(pEnumSourceFiles->Next(1, &pSourceFile, &celt)) && (celt == 1))
                {
                    read_source_file_for_compiland(compilandInfo, pSourceFile);
                    pSourceFile->Release();
                }

                pEnumSourceFiles->Release();
            }
        }

        {
            // Go through all the symbols defined in this compiland.

            IDiaEnumSymbols *pEnumChildren;

            if (SUCCEEDED(pCompiland->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren)))
            {
                {
                    LONG count = 0;
                    pEnumCompilands->get_Count(&count);
                    compilandInfo.functionIds.reserve(count);
                }

                IDiaSymbol *pSymbol;
                ULONG celtChildren = 0;

                while (SUCCEEDED(pEnumChildren->Next(1, &pSymbol, &celtChildren)) && (celtChildren == 1))
                {
                    read_compiland_symbol(compilandInfo, compilandId, pSymbol);
                    pSymbol->Release();
                }

                pEnumChildren->Release();
            }
        }

        pCompiland->Release();
    }

    pEnumCompilands->Release();

    m_compilands.shrink_to_fit();

    return true;
}

void PdbReader::read_compiland_symbol(PdbCompilandInfo &compilandInfo, IndexT compilandId, IDiaSymbol *pSymbol)
{
    // This function provides skeleton functionality, as seen in DIA2DUMP app.

    DWORD dwSymTag;

    if (pSymbol->get_symTag(&dwSymTag) != S_OK)
    {
        wprintf(L"ERROR - get_symTag() failed\n");
        return;
    }

    const auto symTag = static_cast<enum SymTagEnum>(dwSymTag);

    switch (symTag)
    {
        case SymTagCompilandDetails:
            break;

        case SymTagCompilandEnv:
            break;

        case SymTagData:
            break;

        case SymTagFunction: {
            const auto functionId = static_cast<IndexT>(m_functions.size());
            compilandInfo.functionIds.push_back(functionId);
            m_functions.emplace_back();
            PdbFunctionInfo &functionInfo = m_functions.back();
            functionInfo.compilandId = compilandId;
            read_compiland_function(functionInfo, functionId, pSymbol);
            assert(functionInfo.address.absVirtual != 0);
            break;
        }

        case SymTagBlock:
            break;

        case SymTagAnnotation:
            break;

        case SymTagLabel:
            break;

        case SymTagEnum:
        case SymTagTypedef:
        case SymTagUDT:
        case SymTagBaseClass:
            break;

        case SymTagFuncDebugStart:
            read_compiland_function_start(m_functions.back(), pSymbol);
            break;

        case SymTagFuncDebugEnd:
            read_compiland_function_end(m_functions.back(), pSymbol);
            break;

        case SymTagFunctionArgType:
        case SymTagFunctionType:
        case SymTagPointerType:
        case SymTagArrayType:
        case SymTagBaseType:
            break;

        case SymTagThunk:
            break;

        case SymTagCallSite:
            break;

        case SymTagHeapAllocationSite:
            break;

        case SymTagCoffGroup:
            break;

        default:
            break;
    }

    switch (symTag)
    {
        case SymTagFunction:
        case SymTagBlock:
        case SymTagAnnotation:
        case SymTagUDT: {
            IDiaEnumSymbols *pEnumChildren;

            if (SUCCEEDED(pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren)))
            {
                IDiaSymbol *pChild;
                ULONG celt = 0;

                while (SUCCEEDED(pEnumChildren->Next(1, &pChild, &celt)) && (celt == 1))
                {
                    read_compiland_symbol(compilandInfo, compilandId, pChild);
                    pChild->Release();
                }

                pEnumChildren->Release();
            }
        }
    }
}

void PdbReader::read_compiland_function(PdbFunctionInfo &functionInfo, IndexT functionId, IDiaSymbol *pSymbol)
{
    ULONGLONG dwVA;
    DWORD dwRVA;
    DWORD dwSec;
    DWORD dwOff;
    ULONGLONG ulLen;
    DWORD dwCall;

    if (pSymbol->get_virtualAddress(&dwVA) == S_OK)
    {
        functionInfo.address.absVirtual = dwVA;
    }
    if (pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK)
    {
        functionInfo.address.relVirtual = dwRVA;
    }
    if (pSymbol->get_addressSection(&dwSec) == S_OK)
    {
        functionInfo.address.section = dwSec;
    }
    if (pSymbol->get_addressOffset(&dwOff) == S_OK)
    {
        functionInfo.address.offset = dwOff;
    }
    if (pSymbol->get_length(&ulLen) == S_OK)
    {
        functionInfo.length = down_cast<uint32_t>(ulLen);
    }
    if (pSymbol->get_callingConvention(&dwCall) == S_OK)
    {
        functionInfo.call = static_cast<CV_Call>(dwCall);
    }

    {
        BSTR name;
        if (pSymbol->get_undecoratedName(&name) == S_OK)
        {
            functionInfo.undecoratedName = util::to_utf8(name);
            SysFreeString(name);
        }
    }

    {
        BSTR name;
        if (pSymbol->get_name(&name) == S_OK)
        {
            functionInfo.globalName = util::to_utf8(name);
            SysFreeString(name);
        }
    }

    {
        IDiaEnumSymbols *pEnumSymbols;

        if (SUCCEEDED(m_pDiaSymbol->findChildrenExByVA(SymTagPublicSymbol, NULL, nsNone, dwVA, &pEnumSymbols)))
        {
            // Note: There can be more than one public symbol for a function.
            IDiaSymbol *pChildSymbol;
            ULONG celt = 0;

            while (SUCCEEDED(pEnumSymbols->Next(1, &pChildSymbol, &celt)) && (celt == 1))
            {
                // findChildrenExByVA may return unrelated symbols in proximity of the search address.
                // Therefore, skip child symbols with addresses that do not match the search address.
                ULONGLONG dwChildVA;
                if (pChildSymbol->get_virtualAddress(&dwChildVA) != S_OK)
                    continue;
                if (dwChildVA != dwVA)
                    continue;

                read_public_function(functionInfo, pChildSymbol);
                pChildSymbol->Release();
            }

            pEnumSymbols->Release();
        }
    }

    {
        IDiaEnumLineNumbers *pLines;

        if (SUCCEEDED(m_pDiaSession->findLinesByVA(dwVA, DWORD(ulLen), &pLines)))
        {
            {
                LONG count = 0;
                pLines->get_Count(&count);
                functionInfo.sourceLines.reserve(count);
            }

            IDiaLineNumber *pLine;
            ULONG celt = 0;
            int line_index = 0;

            while (SUCCEEDED(pLines->Next(1, &pLine, &celt)) && (celt == 1))
            {
                if (line_index++ == 0)
                {
                    IDiaSourceFile *pSource;
                    if (pLine->get_sourceFile(&pSource) == S_OK)
                    {
                        read_source_file_for_function(functionInfo, functionId, pSource);

                        pSource->Release();
                    }
                }
                read_line(functionInfo, pLine);
                pLine->Release();
            }

            pLines->Release();
        }
    }
}

void PdbReader::read_compiland_function_start(PdbFunctionInfo &functionInfo, IDiaSymbol *pSymbol)
{
    DWORD dwLocType;
    assert(pSymbol->get_locationType(&dwLocType) == S_OK);
    assert(dwLocType == LocIsStatic);

    ULONGLONG dwVA;
    DWORD dwRVA;
    DWORD dwSec;
    DWORD dwOff;

    if (pSymbol->get_virtualAddress(&dwVA) == S_OK)
    {
        functionInfo.debugStartAddress.absVirtual = dwVA;
    }
    if (pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK)
    {
        functionInfo.debugStartAddress.relVirtual = dwRVA;
    }
    if (pSymbol->get_addressSection(&dwSec) == S_OK)
    {
        functionInfo.debugStartAddress.section = dwSec;
    }
    if (pSymbol->get_addressOffset(&dwOff) == S_OK)
    {
        functionInfo.debugStartAddress.offset = dwOff;
    }
}

void PdbReader::read_compiland_function_end(PdbFunctionInfo &functionInfo, IDiaSymbol *pSymbol)
{
    DWORD dwLocType;
    assert(pSymbol->get_locationType(&dwLocType) == S_OK);
    assert(dwLocType == LocIsStatic);

    ULONGLONG dwVA;
    DWORD dwRVA;
    DWORD dwSec;
    DWORD dwOff;

    if (pSymbol->get_virtualAddress(&dwVA) == S_OK)
    {
        functionInfo.debugEndAddress.absVirtual = dwVA;
    }
    if (pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK)
    {
        functionInfo.debugEndAddress.relVirtual = dwRVA;
    }
    if (pSymbol->get_addressSection(&dwSec) == S_OK)
    {
        functionInfo.debugEndAddress.section = dwSec;
    }
    if (pSymbol->get_addressOffset(&dwOff) == S_OK)
    {
        functionInfo.debugEndAddress.offset = dwOff;
    }
}

void PdbReader::read_public_function(PdbFunctionInfo &functionInfo, IDiaSymbol *pSymbol)
{
    BSTR name;
    if (pSymbol->get_name(&name) == S_OK)
    {
        const std::string n = util::to_utf8(name);
        functionInfo.decoratedName = get_relevant_symbol_name(functionInfo.decoratedName, n);
        SysFreeString(name);
    }
}

void PdbReader::read_source_file_for_compiland(PdbCompilandInfo &compilandInfo, IDiaSourceFile *pSourceFile)
{
    BSTR name;
    if (pSourceFile->get_fileName(&name) == S_OK)
    {
        std::string n = util::to_utf8(name);
        StringToIndexMapT::const_iterator it = m_sourceFileNameToIndexMap.find(n);
        assert(it != m_sourceFileNameToIndexMap.end());

        compilandInfo.sourceFileIds.push_back(it->second);

        SysFreeString(name);
    }
}

void PdbReader::read_source_file_for_function(PdbFunctionInfo &functionInfo, IndexT functionId, IDiaSourceFile *pSourceFile)
{
    BSTR name;
    if (pSourceFile->get_fileName(&name) == S_OK)
    {
        std::string n = util::to_utf8(name);
        StringToIndexMapT::const_iterator it = m_sourceFileNameToIndexMap.find(n);
        assert(it != m_sourceFileNameToIndexMap.end());

        assert(functionInfo.sourceFileId == -1);
        functionInfo.sourceFileId = static_cast<IndexT>(it->second);
        m_sourceFiles[it->second].functionIds.push_back(functionId);

        SysFreeString(name);
    }
}

void PdbReader::read_line(PdbFunctionInfo &functionInfo, IDiaLineNumber *pLine)
{
    ULONGLONG dwVA;
    DWORD dwLinenum;
    DWORD dwLength;

    bool ok = true;

    ok = ok && pLine->get_virtualAddress(&dwVA) == S_OK;
    ok = ok && pLine->get_lineNumber(&dwLinenum) == S_OK;
    ok = ok && pLine->get_length(&dwLength) == S_OK;

    if (ok)
    {
        functionInfo.sourceLines.emplace_back();
        PdbSourceLineInfo &lineInfo = functionInfo.sourceLines.back();
        lineInfo.lineNumber = down_cast<uint16_t>(dwLinenum);
        lineInfo.offset = down_cast<uint16_t>(dwVA - functionInfo.address.absVirtual);
        lineInfo.length = down_cast<uint16_t>(dwLength);
    }
}

bool PdbReader::read_publics()
{
    IDiaEnumSymbols *pEnumSymbols;

    if (FAILED(m_pDiaSymbol->findChildren(SymTagPublicSymbol, NULL, nsNone, &pEnumSymbols)))
    {
        return false;
    }

    {
        LONG count = 0;
        pEnumSymbols->get_Count(&count);
        m_symbols.reserve(m_symbols.size() + count);
        m_symbolAddressToIndexMap.reserve(m_symbolAddressToIndexMap.size() + count);
    }

    IDiaSymbol *pSymbol;
    ULONG celt = 0;

    while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1))
    {
        PdbSymbolInfo symbolInfo;
        read_public_symbol(symbolInfo, pSymbol);
        add_or_update_symbol(std::move(symbolInfo));

        pSymbol->Release();
    }

    pEnumSymbols->Release();

    return true;
}

bool PdbReader::read_globals()
{
    constexpr size_t enumCount = 3;
    std::array<enum SymTagEnum, enumCount> dwSymTags = {SymTagFunction, SymTagThunk, SymTagData};
    std::array<IDiaEnumSymbols *, enumCount> enumSymbolsPtrs;
    bool ok = true;
    size_t combinedCount = 0;

    for (size_t i = 0; i < dwSymTags.size(); ++i)
    {
        enumSymbolsPtrs[i] = nullptr;
        ok = ok && SUCCEEDED(m_pDiaSymbol->findChildren(dwSymTags[i], NULL, nsNone, &enumSymbolsPtrs[i]));
        if (ok)
        {
            LONG count = 0;
            enumSymbolsPtrs[i]->get_Count(&count);
            combinedCount += count;
        }
    }

    if (ok)
    {
        m_symbols.reserve(m_symbols.size() + combinedCount);
        m_symbolAddressToIndexMap.reserve(m_symbolAddressToIndexMap.size() + combinedCount);

        for (size_t i = 0; i < enumSymbolsPtrs.size(); ++i)
        {
            IDiaSymbol *pSymbol;
            ULONG celt = 0;

            while (SUCCEEDED(enumSymbolsPtrs[i]->Next(1, &pSymbol, &celt)) && (celt == 1))
            {
                PdbSymbolInfo symbolInfo;
                read_global_symbol(symbolInfo, pSymbol);
                add_or_update_symbol(std::move(symbolInfo));

                pSymbol->Release();
            }
        }
    }

    for (size_t i = 0; i < enumSymbolsPtrs.size(); ++i)
    {
        if (enumSymbolsPtrs[i] != nullptr)
        {
            enumSymbolsPtrs[i]->Release();
            enumSymbolsPtrs[i] = nullptr;
        }
    }

    return ok;
}

void PdbReader::read_common_symbol(PdbSymbolInfo &symbolInfo, IDiaSymbol *pSymbol)
{
    ULONGLONG dwVA;
    DWORD dwRVA;
    DWORD dwSec;
    DWORD dwOff;
    ULONGLONG ulLen;

    if (pSymbol->get_virtualAddress(&dwVA) == S_OK)
    {
        symbolInfo.address.absVirtual = dwVA;
    }
    if (pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK)
    {
        symbolInfo.address.relVirtual = dwRVA;
    }
    if (pSymbol->get_addressSection(&dwSec) == S_OK)
    {
        symbolInfo.address.section = dwSec;
    }
    if (pSymbol->get_addressOffset(&dwOff) == S_OK)
    {
        symbolInfo.address.offset = dwOff;
    }
    if (pSymbol->get_length(&ulLen) == S_OK)
    {
        symbolInfo.length = down_cast<uint32_t>(ulLen);
    }

    {
        BSTR name;
        if (pSymbol->get_undecoratedName(&name) == S_OK)
        {
            symbolInfo.undecoratedName = util::to_utf8(name);
            SysFreeString(name);
        }
    }
}

void PdbReader::read_public_symbol(PdbSymbolInfo &symbolInfo, IDiaSymbol *pSymbol)
{
    read_common_symbol(symbolInfo, pSymbol);

    {
        BSTR name;
        if (pSymbol->get_name(&name) == S_OK)
        {
            symbolInfo.decoratedName = util::to_utf8(name);
            SysFreeString(name);
        }
    }
}

void PdbReader::read_global_symbol(PdbSymbolInfo &symbolInfo, IDiaSymbol *pSymbol)
{
    read_common_symbol(symbolInfo, pSymbol);

    {
        BSTR name;
        if (pSymbol->get_name(&name) == S_OK)
        {
            symbolInfo.globalName = util::to_utf8(name);
            SysFreeString(name);
        }
    }
}

const std::string &PdbReader::get_relevant_symbol_name(const std::string &name1, const std::string &name2)
{
    if (name1.empty())
    {
        return name2;
    }
    else if (name2.empty())
    {
        return name1;
    }
    else
    {
        if (name1.compare(name2) <= 0)
        {
            return name1;
        }
        else
        {
            return name2;
        }
    }
}

bool PdbReader::add_or_update_symbol(PdbSymbolInfo &&symbolInfo)
{
    const auto address = symbolInfo.address.absVirtual;

    if (address == ~decltype(address)(0))
    {
        return false;
    }

    Address64ToIndexMapT::iterator it = m_symbolAddressToIndexMap.find(address);

    if (it == m_symbolAddressToIndexMap.end())
    {
        const IndexT index = static_cast<IndexT>(m_symbols.size());
        m_symbols.emplace_back(std::move(symbolInfo));
        [[maybe_unused]] auto [_, added] = m_symbolAddressToIndexMap.try_emplace(address, index);
        assert(added);
    }
    else
    {
        // This update can hit in two ways:
        //
        // 1) a public symbol is also a global symbol
        //
        // 2) a symbol, such as an overloaded virtual destructor, has more than 1 symbol for its address:
        //    ??_EArmorStore@@UAEPAXI@Z  public: virtual void * __thiscall ArmorStore::`vector deleting destructor'...
        //    ??_GArmorStore@@UAEPAXI@Z  public: virtual void * __thiscall ArmorStore::`scalar deleting destructor'...

        PdbSymbolInfo &curSymbolInfo = m_symbols[it->second];

        if (symbolInfo.address.absVirtual != ~Address64T(0))
        {
            curSymbolInfo.address = symbolInfo.address;
        }
        if (symbolInfo.length != 0)
        {
            curSymbolInfo.length = symbolInfo.length;
        }
        curSymbolInfo.decoratedName = get_relevant_symbol_name(curSymbolInfo.decoratedName, symbolInfo.decoratedName);
        curSymbolInfo.undecoratedName = get_relevant_symbol_name(curSymbolInfo.undecoratedName, symbolInfo.undecoratedName);
        curSymbolInfo.globalName = get_relevant_symbol_name(curSymbolInfo.globalName, symbolInfo.globalName);
    }

    return true;
}

#endif

} // namespace unassemblize
