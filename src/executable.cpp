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
#include "executable.h"
#include "util.h"
#include <LIEF/LIEF.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace unassemblize
{

bool Executable::load(const std::string &exe_filename)
{
    unload();

    if (m_verbose)
    {
        printf("Loading section info...\n");
    }

    const std::string full_path = util::abs_path(exe_filename);

    m_binary = LIEF::Parser::parse(full_path);

    if (m_binary.get() == nullptr)
    {
        return false;
    }

    m_exeFilename = full_path;
    m_imageData.imageBase = m_binary->imagebase();

    bool checked_image_base = false;

    for (auto it = m_binary->sections().begin(); it != m_binary->sections().end(); ++it)
    {
        if (!it->name().empty() && it->size() != 0)
        {
            const IndexT section_idx = m_sections.size();
            m_sections.emplace_back();
            ExeSectionInfo &section = m_sections.back();

            section.name = it->name();
            section.data = it->content().data();

            // For PE format virtual_address appears to be an offset, in ELF/Mach-O it appears to be absolute.
            // #TODO: Check if ELF/Mach-O works correctly with this code - if necessary.
            section.address = it->virtual_address();
            section.size = it->size();

            m_imageData.sectionsBegin = std::min(m_imageData.sectionsBegin, section.address);
            m_imageData.sectionsEnd = std::max(m_imageData.sectionsEnd, section.address + section.size);

            // Naive split on whether section contains data or code... have entrypoint? Code, else data.
            // Needs to be refined by providing a config file with section types specified.
            const uint64_t entrypoint = m_binary->entrypoint() - m_binary->imagebase();
            if (section.address < entrypoint && section.address + section.size >= entrypoint)
            {
                section.type = ExeSectionType::Code;
                assert(m_codeSectionIdx == ~IndexT(0));
                m_codeSectionIdx = section_idx;
            }
            else
            {
                section.type = ExeSectionType::Data;
            }
        }
    }

    if (m_verbose)
    {
        printf("Indexing embedded symbols...\n");
    }

    auto exe_syms = m_binary->symbols();
    auto exe_imports = m_binary->imported_functions();

    {
        const size_t newSize = m_symbols.size() + exe_syms.size() + exe_imports.size();
        m_symbols.reserve(newSize);
        m_symbolAddressToIndexMap.reserve(newSize);
        m_symbolNameToIndexMap.reserve(newSize);
    }

    for (auto it = exe_syms.begin(); it != exe_syms.end(); ++it)
    {
        ExeSymbol symbol;
        symbol.name = it->name();
        symbol.address = it->value();
        symbol.size = it->size();

        add_symbol(symbol);
    }

    for (auto it = exe_imports.begin(); it != exe_imports.end(); ++it)
    {
        ExeSymbol symbol;
        symbol.name = it->name();
        symbol.address = it->value();
        symbol.size = it->size();

        add_symbol(symbol);
    }

    if (m_targetObjects.empty())
    {
        m_targetObjects.push_back(
            {m_binary->name().substr(m_binary->name().find_last_of("/\\") + 1), std::vector<ExeObjectSection>()});
        ExeObject &obj = m_targetObjects.back();

        for (auto it = m_binary->sections().begin(); it != m_binary->sections().end(); ++it)
        {
            if (it->name().empty() || it->size() == 0)
            {
                continue;
            }

            obj.sections.push_back({it->name(), it->offset(), it->size()});
        }
    }

    return true;
}

void Executable::unload()
{
    util::free_container(m_exeFilename);
    m_binary.reset();
    util::free_container(m_sections);
    m_codeSectionIdx = ~IndexT(0);
    util::free_container(m_symbols);
    util::free_container(m_symbolAddressToIndexMap);
    util::free_container(m_symbolNameToIndexMap);
    util::free_container(m_targetObjects);
    m_imageData = ExeImageData();
}

bool Executable::is_loaded() const
{
    return m_binary.get() != nullptr;
}

const std::string &Executable::get_filename() const
{
    return m_exeFilename;
}

const ExeSections &Executable::get_sections() const
{
    return m_sections;
}

const ExeSectionInfo *Executable::find_section(uint64_t address) const
{
    for (const ExeSectionInfo &section : m_sections)
    {
        if (address >= section.address && address < section.address + section.size)
            return &section;
    }
    return nullptr;
}

const ExeSectionInfo *Executable::find_section(const std::string &name) const
{
    for (const ExeSectionInfo &section : m_sections)
    {
        if (section.name == name)
            return &section;
    }
    return nullptr;
}

ExeSectionInfo *Executable::find_section(const std::string &name)
{
    for (ExeSectionInfo &section : m_sections)
    {
        if (section.name == name)
            return &section;
    }
    return nullptr;
}

const ExeSectionInfo *Executable::get_code_section() const
{
    if (m_codeSectionIdx < m_sections.size())
    {
        return &m_sections[m_codeSectionIdx];
    }
    return nullptr;
}

uint64_t Executable::image_base() const
{
    return m_imageData.imageBase;
}

uint64_t Executable::code_section_begin_from_image_base() const
{
    const ExeSectionInfo *section = get_code_section();
    assert(section != nullptr);
    return section->address + m_imageData.imageBase;
}

uint64_t Executable::code_section_end_from_image_base() const
{
    const ExeSectionInfo *section = get_code_section();
    assert(section != nullptr);
    return section->address + section->size + m_imageData.imageBase;
}

uint64_t Executable::all_sections_begin_from_image_base() const
{
    return m_imageData.sectionsBegin + m_imageData.imageBase;
}

uint64_t Executable::all_sections_end_from_image_base() const
{
    return m_imageData.sectionsEnd + m_imageData.imageBase;
}

const ExeSymbol *Executable::get_symbol(uint64_t address) const
{
    Address64ToIndexMapT::const_iterator it = m_symbolAddressToIndexMap.find(address);

    if (it != m_symbolAddressToIndexMap.end())
    {
        return &m_symbols[it->second];
    }
    return nullptr;
}

const ExeSymbol *Executable::get_symbol(const std::string &name) const
{
    StringToIndexMapT::const_iterator it = m_symbolNameToIndexMap.find(name);

    if (it != m_symbolNameToIndexMap.end())
    {
        return &m_symbols[it->second];
    }
    return nullptr;
}

const ExeSymbol *Executable::get_symbol_from_image_base(uint64_t address) const
{
    return get_symbol(address - image_base());
}

const ExeSymbols &Executable::get_symbols() const
{
    return m_symbols;
}

void Executable::add_symbols(const ExeSymbols &symbols, bool overwrite)
{
    const size_t size = m_symbols.size() + symbols.size();
    m_symbols.reserve(size);
    m_symbolAddressToIndexMap.reserve(size);
    m_symbolNameToIndexMap.reserve(size);

    for (const ExeSymbol &symbol : symbols)
    {
        add_symbol(symbol, overwrite);
    }
}

void Executable::add_symbols(const PdbSymbolInfoVector &symbols, bool overwrite)
{
    const size_t size = m_symbols.size() + symbols.size();
    m_symbols.reserve(size);
    m_symbolAddressToIndexMap.reserve(size);
    m_symbolNameToIndexMap.reserve(size);

    for (const PdbSymbolInfo &pdbSymbol : symbols)
    {
        add_symbol(to_exe_symbol(pdbSymbol), overwrite);
    }
}

void Executable::add_symbol(const ExeSymbol &symbol, bool overwrite)
{
    if (symbol.address == 0)
        return;

    Address64ToIndexMapT::iterator it = m_symbolAddressToIndexMap.find(symbol.address);

    if (it == m_symbolAddressToIndexMap.end())
    {
        const IndexT index = static_cast<IndexT>(m_symbols.size());
        m_symbols.push_back(symbol);
        [[maybe_unused]] auto [_, added] = m_symbolAddressToIndexMap.try_emplace(symbol.address, index);
        assert(added);
        // Symbol name can collide...
        if (overwrite)
            m_symbolNameToIndexMap.emplace(symbol.name, index);
        else
            m_symbolNameToIndexMap.try_emplace(symbol.name, index);
    }
    else if (overwrite)
    {
        m_symbols[it->second] = symbol;
    }
}

bool Executable::load_config(const char *filename, bool overwrite_symbols)
{
    if (m_verbose)
    {
        printf("Loading config file '%s'...\n", filename);
    }

    std::ifstream fs(filename);
    if (fs.fail())
    {
        return false;
    }

    nlohmann::json js = nlohmann::json::parse(fs);
    load_json(js, overwrite_symbols);
    return true;
}

bool Executable::save_config(const char *filename) const
{
    if (m_verbose)
    {
        printf("Saving config file '%s'...\n", filename);
    }

    nlohmann::json js;
    {
        std::ifstream fs(filename);
        if (!fs.fail())
        {
            js = nlohmann::json::parse(fs);
        }
    }

    save_json(js);

    std::ofstream fs(filename);
    fs << std::setw(4) << js << std::endl;
    return !fs.fail();
}

void Executable::load_json(const nlohmann::json &js, bool overwrite_symbols)
{
    if (js.contains(EXE_CONFIG_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading config section...\n");
        }
        js.at(EXE_CONFIG_SECTION).get_to(m_imageData);
    }

    if (js.contains(EXE_SYMBOLS_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading symbols section...\n");
        }
        std::vector<ExeSymbol> symbols;
        js.at(EXE_SYMBOLS_SECTION).get_to(symbols);
        add_symbols(symbols, overwrite_symbols);
    }

    if (js.contains(EXE_SECTIONS_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading sections info...\n");
        }
        std::vector<ExeSectionInfo> sections;
        js.at(EXE_SECTIONS_SECTION).get_to(sections);

        for (const auto &sectionInfo : sections)
        {
            if (sectionInfo.name.empty())
                continue;

            ExeSectionInfo *existingSection = find_section(sectionInfo.name);
            if (!existingSection)
            {
                if (m_verbose)
                {
                    printf("Section '%s' not found in binary\n", sectionInfo.name.c_str());
                }
                continue;
            }

            existingSection->type = sectionInfo.type;
            existingSection->address = sectionInfo.address;
            existingSection->size = sectionInfo.size;
        }
    }

    if (js.contains(EXE_OBJECTS_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading objects section...\n");
        }
        js.at(EXE_OBJECTS_SECTION).get_to(m_targetObjects);
    }
}

void Executable::save_json(nlohmann::json &js) const
{
    if (m_verbose)
    {
        printf("Saving config section...\n");
    }
    js[EXE_CONFIG_SECTION] = m_imageData;

    if (m_verbose)
    {
        printf("Saving symbols section...\n");
    }
    js[EXE_SYMBOLS_SECTION] = m_symbols;

    if (m_verbose)
    {
        printf("Saving sections section...\n");
    }
    js[EXE_SECTIONS_SECTION] = m_sections;

    if (m_verbose)
    {
        printf("Saving objects section...\n");
    }
    js[EXE_OBJECTS_SECTION] = m_targetObjects;
}

} // namespace unassemblize
