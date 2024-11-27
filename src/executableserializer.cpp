#include "executableserializer.h"
#include "executable.h"
#include <fstream>

namespace unassemblize
{

bool ExecutableSerializer::load_config(const std::string &filename, Executable &exe, bool overwrite_symbols)
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

    nlohmann::json js = nlohmann::json::parse(fs);
    load_json(js, exe, overwrite_symbols);
    return true;
}

bool ExecutableSerializer::save_config(const std::string &filename, const Executable &exe) const
{
    if (m_verbose)
    {
        printf("Saving config file '%s'...\n", filename.c_str());
    }

    nlohmann::json js;

    // Parse existing config file if it exists
    {
        std::ifstream fs(filename);
        if (!fs.fail())
        {
            js = nlohmann::json::parse(fs);
        }
    }

    save_json(js, exe);

    std::ofstream fs(filename);
    fs << std::setw(4) << js << std::endl;
    return !fs.fail();
}

void ExecutableSerializer::load_json(const nlohmann::json &js, Executable &exe, bool overwrite_symbols)
{
    if (js.contains(EXE_CONFIG_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading config section...\n");
        }
        js.at(EXE_CONFIG_SECTION).get_to(exe.m_imageData);
    }

    if (js.contains(EXE_SYMBOLS_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading symbols section...\n");
        }
        const auto &symbols = js.at(EXE_SYMBOLS_SECTION);
        for (const auto &symbol : symbols)
        {
            ExeSymbol exeSymbol;
            symbol.get_to(exeSymbol);
            if (!exeSymbol.name.empty() && exeSymbol.address != 0)
            {
                exe.add_symbol(exeSymbol, overwrite_symbols);
            }
        }
    }

    if (js.contains(EXE_SECTIONS_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading sections info...\n");
        }

        const auto &sections = js.at(EXE_SECTIONS_SECTION);
        for (const auto &section : sections)
        {
            std::string name = section.at("name").get<std::string>();
            if (name.empty())
                continue;

            ExeSectionInfo *sectionInfo = exe.find_section(name);
            if (!sectionInfo)
            {
                if (m_verbose)
                {
                    printf("Section '%s' not found in binary\n", name.c_str());
                }
                continue;
            }

            std::string type = section.at("type").get<std::string>();
            sectionInfo->type = to_section_type(type.c_str());

            if (section.contains("address"))
                section.at("address").get_to(sectionInfo->address);
            if (section.contains("size"))
                section.at("size").get_to(sectionInfo->size);
        }
    }

    if (js.contains(EXE_OBJECTS_SECTION))
    {
        if (m_verbose)
        {
            printf("Loading objects section...\n");
        }

        const auto &objects = js.at(EXE_OBJECTS_SECTION);
        for (const auto &object : objects)
        {
            ExeObject obj;
            object.at("name").get_to(obj.name);
            const auto &sections = object.at("sections");
            for (const auto &section : sections)
            {
                ExeObjectSection objSection;
                section.at("name").get_to(objSection.name);
                section.at("offset").get_to(objSection.offset);
                section.at("size").get_to(objSection.size);
                obj.sections.push_back(objSection);
            }
            exe.m_targetObjects.push_back(obj);
        }
    }
}

void ExecutableSerializer::save_json(nlohmann::json &js, const Executable &exe) const
{
    if (m_verbose)
    {
        printf("Saving config section...\n");
    }
    js[EXE_CONFIG_SECTION] = exe.m_imageData;

    if (m_verbose)
    {
        printf("Saving symbols section...\n");
    }
    js[EXE_SYMBOLS_SECTION] = exe.m_symbols;

    // Sections section
    auto &sections = js[EXE_SECTIONS_SECTION] = nlohmann::json::array();
    for (const auto &section : exe.m_sections)
    {
        sections.push_back(
            {{"name", section.name},
             {"type", to_string(section.type)},
             {"address", section.address},
             {"size", section.size}});
    }

    // Objects section
    auto &objects = js[EXE_OBJECTS_SECTION] = nlohmann::json::array();
    for (const auto &object : exe.m_targetObjects)
    {
        nlohmann::json obj = {{"name", object.name}, {"sections", nlohmann::json::array()}};
        for (const auto &section : object.sections)
        {
            obj["sections"].push_back({{"name", section.name}, {"offset", section.offset}, {"size", section.size}});
        }
        objects.push_back(obj);
    }
}

} // namespace unassemblize