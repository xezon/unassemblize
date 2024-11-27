#include "executableserializer.h"
#include "executable.h"
#include <fstream>

namespace unassemblize
{

bool ExecutableSerializer::load(const std::string &filename, Executable &exe, bool overwrite_symbols)
{
    if (m_verbose)
    {
        printf("Loading config file '%s'...\n", filename.c_str());
    }

    std::ifstream fs(filename);
    if (fs.fail())
    {
        return false;
    }

    nlohmann::json j = nlohmann::json::parse(fs);

    if (j.contains(CONFIG_SECTION))
    {
        loadConfig(j.at(CONFIG_SECTION), exe);
    }

    if (j.contains(SYMBOL_SECTION))
    {
        loadSymbols(j.at(SYMBOL_SECTION), exe, overwrite_symbols);
    }

    if (j.contains(SECTIONS_SECTION))
    {
        loadSections(j.at(SECTIONS_SECTION), exe);
    }

    if (j.contains(OBJECT_SECTION))
    {
        loadObjects(j.at(OBJECT_SECTION), exe);
    }

    return true;
}

bool ExecutableSerializer::save(const std::string &filename, const Executable &exe) const
{
    if (m_verbose)
    {
        printf("Saving config file '%s'...\n", filename.c_str());
    }

    nlohmann::json j;

    // Parse existing config file if it exists
    {
        std::ifstream fs(filename);
        if (!fs.fail())
        {
            j = nlohmann::json::parse(fs);
        }
    }

    // Update or create config sections
    if (!j.contains(CONFIG_SECTION))
    {
        j[CONFIG_SECTION] = nlohmann::json();
    }
    saveConfig(j[CONFIG_SECTION], exe);

    if (!j.contains(SYMBOL_SECTION))
    {
        j[SYMBOL_SECTION] = nlohmann::json();
        saveSymbols(j[SYMBOL_SECTION], exe);
    }

    if (!j.contains(SECTIONS_SECTION))
    {
        j[SECTIONS_SECTION] = nlohmann::json();
        saveSections(j[SECTIONS_SECTION], exe);
    }

    if (!j.contains(OBJECT_SECTION))
    {
        j[OBJECT_SECTION] = nlohmann::json();
        saveObjects(j[OBJECT_SECTION], exe);
    }

    std::ofstream fs(filename);
    fs << std::setw(4) << j << std::endl;

    return !fs.fail();
}

void ExecutableSerializer::loadConfig(const nlohmann::json &js, Executable &exe)
{
    auto &imageData = exe.m_imageData;
    js.at("codealign").get_to(imageData.codeAlignment);
    js.at("dataalign").get_to(imageData.dataAlignment);
    js.at("codepadding").get_to(imageData.codePad);
    js.at("datapadding").get_to(imageData.dataPad);
}

void ExecutableSerializer::loadSymbols(const nlohmann::json &js, Executable &exe, bool overwrite_symbols)
{
    if (m_verbose)
    {
        printf("Loading external symbols...\n");
    }

    size_t newSize = exe.m_symbols.size() + js.size();
    exe.m_symbols.reserve(newSize);
    exe.m_symbolAddressToIndexMap.reserve(newSize);
    exe.m_symbolNameToIndexMap.reserve(newSize);

    for (const auto &symbol_json : js)
    {
        ExeSymbol symbol;

        symbol_json.at("name").get_to(symbol.name);
        if (symbol.name.empty())
        {
            continue;
        }

        symbol_json.at("address").get_to(symbol.address);
        if (symbol.address == 0)
        {
            continue;
        }

        symbol_json.at("size").get_to(symbol.size);

        exe.add_symbol(symbol, overwrite_symbols);
    }
}

void ExecutableSerializer::saveSymbols(nlohmann::json &js, const Executable &exe) const
{
    if (m_verbose)
    {
        printf("Saving symbols...\n");
    }

    for (const ExeSymbol &symbol : exe.m_symbols)
    {
        js.push_back({{"name", symbol.name}, {"address", symbol.address}, {"size", symbol.size}});
    }
}

void ExecutableSerializer::loadSections(const nlohmann::json &js, Executable &exe)
{
    if (m_verbose)
    {
        printf("Loading section info...\n");
    }

    for (const auto &section_json : js)
    {
        std::string name;
        section_json.at("name").get_to(name);

        // Don't try and load an empty section
        if (name.empty())
        {
            continue;
        }

        ExeSectionInfo *section = exe.find_section(name);
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
        section_json.at("type").get_to(type);
        section->type = to_section_type(type.c_str());

        if (section->type == ExeSectionType::Unknown && m_verbose)
        {
            printf("Incorrect type specified for section '%s'.\n", name.c_str());
        }

        auto it_address = section_json.find("address");
        if (it_address != section_json.end())
        {
            it_address->get_to(section->address);
        }

        auto it_size = section_json.find("size");
        if (it_size != section_json.end())
        {
            it_size->get_to(section->size);
        }
    }
}

void ExecutableSerializer::saveSections(nlohmann::json &js, const Executable &exe) const
{
    if (m_verbose)
    {
        printf("Saving section info...\n");
    }

    for (const ExeSectionInfo &section : exe.m_sections)
    {
        const char *type_str = to_string(section.type);
        js.push_back({{"name", section.name}, {"type", type_str}, {"address", section.address}, {"size", section.size}});
    }
}

void ExecutableSerializer::loadObjects(const nlohmann::json &js, Executable &exe)
{
    if (m_verbose)
    {
        printf("Loading objects...\n");
    }

    for (const auto &object_json : js)
    {
        std::string obj_name;
        object_json.at("name").get_to(obj_name);

        if (obj_name.empty())
        {
            continue;
        }

        // Skip if entry already exists
        auto it_object = std::find_if(exe.m_targetObjects.begin(), exe.m_targetObjects.end(), [&](const ExeObject &object) {
            return object.name == obj_name;
        });

        if (it_object != exe.m_targetObjects.end())
        {
            continue;
        }

        exe.m_targetObjects.push_back({obj_name, std::vector<ExeObjectSection>()});
        ExeObject &obj = exe.m_targetObjects.back();
        const auto &sections_json = object_json.at("sections");

        for (const auto &section_json : sections_json)
        {
            obj.sections.emplace_back();
            ExeObjectSection &section = obj.sections.back();
            section_json.at("name").get_to(section.name);
            section_json.at("offset").get_to(section.offset);
            section_json.at("size").get_to(section.size);
        }
    }
}

void ExecutableSerializer::saveObjects(nlohmann::json &js, const Executable &exe) const
{
    if (m_verbose)
    {
        printf("Saving objects...\n");
    }

    for (const ExeObject &object : exe.m_targetObjects)
    {
        js.push_back({{"name", object.name}, {"sections", nlohmann::json::array()}});
        auto &sections_json = js.back().at("sections");

        for (const ExeObjectSection &section : object.sections)
        {
            sections_json.push_back({{"name", section.name}, {"offset", section.offset}, {"size", section.size}});
        }
    }
}

void ExecutableSerializer::saveConfig(nlohmann::json &js, const Executable &exe) const
{
    js["codealign"] = exe.m_imageData.codeAlignment;
    js["dataalign"] = exe.m_imageData.dataAlignment;
    js["codepadding"] = exe.m_imageData.codePad;
    js["datapadding"] = exe.m_imageData.dataPad;
}

} // namespace unassemblize