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
const char *const s_symbolSection = "symbols";
const char *const s_sectionsSection = "sections";
const char *const s_configSection = "config";
const char *const s_objectSection = "objects";

Executable::Executable()
{
}

Executable::~Executable()
{
}

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
            const Address64T entrypoint = m_binary->entrypoint() - m_binary->imagebase();
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

const ExeSectionInfo *Executable::find_section(Address64T address) const
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

Address64T Executable::image_base() const
{
    return m_imageData.imageBase;
}

Address64T Executable::code_section_begin_from_image_base() const
{
    const ExeSectionInfo *section = get_code_section();
    assert(section != nullptr);
    return section->address + m_imageData.imageBase;
}

Address64T Executable::code_section_end_from_image_base() const
{
    const ExeSectionInfo *section = get_code_section();
    assert(section != nullptr);
    return section->address + section->size + m_imageData.imageBase;
}

Address64T Executable::all_sections_begin_from_image_base() const
{
    return m_imageData.sectionsBegin + m_imageData.imageBase;
}

Address64T Executable::all_sections_end_from_image_base() const
{
    return m_imageData.sectionsEnd + m_imageData.imageBase;
}

const ExeSymbols &Executable::get_symbols() const
{
    return m_symbols;
}

const ExeSymbol *Executable::get_symbol(Address64T address) const
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
    auto pair = m_symbolNameToIndexMap.equal_range(name);

    if (std::distance(pair.first, pair.second) == 1)
    {
        // No symbol or multiple symbols with this name. Skip.
        return nullptr;
    }

    return &m_symbols[pair.first->second];
}

const ExeSymbol *Executable::get_symbol_from_image_base(Address64T address) const
{
    return get_symbol(address - image_base());
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
        m_symbolNameToIndexMap.emplace(symbol.name, index);
    }
    else if (overwrite)
    {
        m_symbols[it->second] = symbol;
    }
}

bool Executable::load_config(const char *file_name, bool overwrite_symbols)
{
    if (m_verbose)
    {
        printf("Loading config file '%s'...\n", file_name);
    }

    std::ifstream fs(file_name);

    if (fs.fail())
    {
        return false;
    }

    nlohmann::json j = nlohmann::json::parse(fs);

    if (j.find(s_configSection) != j.end())
    {
        nlohmann::json &conf = j.at(s_configSection);
        conf.at("codealign").get_to(m_imageData.codeAlignment);
        conf.at("dataalign").get_to(m_imageData.dataAlignment);
        conf.at("codepadding").get_to(m_imageData.codePad);
        conf.at("datapadding").get_to(m_imageData.dataPad);
    }

    if (j.find(s_symbolSection) != j.end())
    {
        load_symbols(j.at(s_symbolSection), overwrite_symbols);
    }

    if (j.find(s_sectionsSection) != j.end())
    {
        load_sections(j.at(s_sectionsSection));
    }

    if (j.find(s_objectSection) != j.end())
    {
        load_objects(j.at(s_objectSection));
    }

    return true;
}

bool Executable::save_config(const char *file_name) const
{
    if (m_verbose)
    {
        printf("Saving config file '%s'...\n", file_name);
    }

    nlohmann::json j;

    // Parse the config file if it already exists and update it.
    {
        std::ifstream fs(file_name);

        if (!fs.fail())
        {
            j = nlohmann::json::parse(fs);
        }
    }

    if (j.find(s_configSection) == j.end())
    {
        j[s_configSection] = nlohmann::json();
    }

    nlohmann::json &conf = j.at(s_configSection);
    conf["codealign"] = m_imageData.codeAlignment;
    conf["dataalign"] = m_imageData.dataAlignment;
    conf["codepadding"] = m_imageData.codePad;
    conf["datapadding"] = m_imageData.dataPad;

    // Don't dump if we already have a sections for these.
    if (j.find(s_symbolSection) == j.end())
    {
        j[s_symbolSection] = nlohmann::json();
        dump_symbols(j.at(s_symbolSection));
    }

    if (j.find(s_sectionsSection) == j.end())
    {
        j[s_sectionsSection] = nlohmann::json();
        dump_sections(j.at(s_sectionsSection));
    }

    if (j.find(s_objectSection) == j.end())
    {
        j[s_objectSection] = nlohmann::json();
        dump_objects(j.at(s_objectSection));
    }

    std::ofstream fs(file_name);
    fs << std::setw(4) << j << std::endl;

    return !fs.fail();
}

void Executable::load_symbols(nlohmann::json &js, bool overwrite_symbols)
{
    if (m_verbose)
    {
        printf("Loading external symbols...\n");
    }

    size_t newSize = m_symbols.size() + js.size();
    m_symbols.reserve(newSize);
    m_symbolAddressToIndexMap.reserve(newSize);
    m_symbolNameToIndexMap.reserve(newSize);

    for (auto it = js.begin(); it != js.end(); ++it)
    {
        ExeSymbol symbol;

        it->at("name").get_to(symbol.name);
        if (symbol.name.empty())
        {
            continue;
        }

        it->at("address").get_to(symbol.address);
        if (symbol.address == 0)
        {
            continue;
        }

        it->at("size").get_to(symbol.size);

        add_symbol(symbol, overwrite_symbols);
    }
}

void Executable::dump_symbols(nlohmann::json &js) const
{
    if (m_verbose)
    {
        printf("Saving symbols...\n");
    }

    for (const ExeSymbol &symbol : m_symbols)
    {
        js.push_back({{"name", symbol.name}, {"address", symbol.address}, {"size", symbol.size}});
    }
}

void Executable::load_sections(nlohmann::json &js)
{
    if (m_verbose)
    {
        printf("Loading section info...\n");
    }

    for (auto it = js.begin(); it != js.end(); ++it)
    {
        std::string name;
        it->at("name").get_to(name);

        // Don't try and load an empty section.
        if (!name.empty())
        {
            ExeSectionInfo *section = find_section(name);

            if (section == nullptr)
            {
                if (m_verbose)
                {
                    printf("Tried to load section info for section not present in this binary!\n");
                    printf("Section '%s' info was ignored.\n", name.c_str());
                }
                continue;
            }

            std::string type;
            it->at("type").get_to(type);

            section->type = to_section_type(type.c_str());

            if (section->type == ExeSectionType::Unknown && m_verbose)
            {
                printf("Incorrect type specified for section '%s'.\n", name.c_str());
            }

            auto it_address = it->find("address");
            if (it_address != it->end())
            {
                it_address->get_to(section->address);
            }
            auto it_size = it->find("size");
            if (it_size != it->end())
            {
                it_size->get_to(section->size);
            }
        }
    }
}

void Executable::dump_sections(nlohmann::json &js) const
{
    if (m_verbose)
    {
        printf("Saving section info...\n");
    }

    for (const ExeSectionInfo &section : m_sections)
    {
        const char *type_str = to_string(section.type);
        js.push_back({{"name", section.name}, {"type", type_str}, {"address", section.address}, {"size", section.size}});
    }
}

void Executable::load_objects(nlohmann::json &js)
{
    if (m_verbose)
    {
        printf("Loading objects...\n");
    }

    for (auto js_object = js.begin(); js_object != js.end(); ++js_object)
    {
        std::string obj_name;
        js_object->at("name").get_to(obj_name);

        if (obj_name.empty())
        {
            continue;
        }

        {
            // Skip if entry already exists.
            ExeObjects::const_iterator it_object =
                std::find_if(m_targetObjects.begin(), m_targetObjects.end(), [&](const ExeObject &object) {
                    return object.name == obj_name;
                });
            if (it_object != m_targetObjects.end())
            {
                continue;
            }
        }

        m_targetObjects.push_back({obj_name, std::vector<ExeObjectSection>()});
        ExeObject &obj = m_targetObjects.back();
        const auto &js_sections = js.back().at("sections");

        for (auto js_section = js_sections.begin(); js_section != js_sections.end(); ++js_section)
        {
            obj.sections.emplace_back();
            ExeObjectSection &section = obj.sections.back();
            js_section->at("name").get_to(section.name);
            js_section->at("offset").get_to(section.offset);
            js_section->at("size").get_to(section.size);
        }
    }
}

void Executable::dump_objects(nlohmann::json &js) const
{
    if (m_verbose)
    {
        printf("Saving objects...\n");
    }

    for (const ExeObject &object : m_targetObjects)
    {
        js.push_back({{"name", object.name}, {"sections", nlohmann::json()}});
        auto &js_sections = js.back().at("sections");

        for (const ExeObjectSection &section : object.sections)
        {
            js_sections.push_back({{"name", section.name}, {"offset", section.offset}, {"size", section.size}});
        }
    }
}

} // namespace unassemblize
