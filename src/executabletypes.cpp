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
#include "executabletypes.h"
#include "util.h"
#include <nlohmann/json.hpp>

namespace unassemblize
{
ExeSectionType to_section_type(std::string_view str)
{
    if (util::equals_nocase(str, "code"))
    {
        return ExeSectionType::Code;
    }
    else if (util::equals_nocase(str, "data"))
    {
        return ExeSectionType::Data;
    }
    else
    {
        return ExeSectionType::Unknown;
    }
    static_assert(size_t(ExeSectionType::Unknown) == 2, "Enum was changed. Update conditions.");
}

const char *to_string(ExeSectionType type)
{
    switch (type)
    {
        case ExeSectionType::Code:
            return "code";
        case ExeSectionType::Data:
            return "data";
        case ExeSectionType::Unknown:
        default:
            return "unknown";
    }
    static_assert(size_t(ExeSectionType::Unknown) == 2, "Enum was changed. Update switch case.");
}

// #TODO: Verify and improve error handling when a json field does not exist.
// Currently js.at("") should crash if that named field does not exit.

void to_json(nlohmann::json &js, const ExeSymbol &d)
{
    js = nlohmann::json{{"name", d.name}, {"address", d.address}, {"size", d.size}};
}

void from_json(const nlohmann::json &js, ExeSymbol &d)
{
    js.at("name").get_to(d.name);
    js.at("address").get_to(d.address);
    js.at("size").get_to(d.size);
}

void to_json(nlohmann::json &js, const ExeObjectSection &d)
{
    js = nlohmann::json{{"name", d.name}, {"offset", d.offset}, {"size", d.size}};
}

void from_json(const nlohmann::json &js, ExeObjectSection &d)
{
    js.at("name").get_to(d.name);
    js.at("offset").get_to(d.offset);
    js.at("size").get_to(d.size);
}

void to_json(nlohmann::json &js, const ExeObject &d)
{
    js = nlohmann::json{{"name", d.name}, {"sections", d.sections}};
}

void from_json(const nlohmann::json &js, ExeObject &d)
{
    js.at("name").get_to(d.name);
    js.at("sections").get_to(d.sections);
}

void to_json(nlohmann::json &js, const ExeImageData &d)
{
    js = nlohmann::json{
        {"codealign", d.codeAlignment},
        {"dataalign", d.dataAlignment},
        {"codepadding", d.codePad},
        {"datapadding", d.dataPad}};
}

void from_json(const nlohmann::json &js, ExeImageData &d)
{
    js.at("codealign").get_to(d.codeAlignment);
    js.at("dataalign").get_to(d.dataAlignment);
    js.at("codepadding").get_to(d.codePad);
    js.at("datapadding").get_to(d.dataPad);
}

void to_json(nlohmann::json &js, const ExeSectionInfo &d)
{
    js = nlohmann::json{{"name", d.name}, {"type", to_string(d.type)}, {"address", d.address}, {"size", d.size}};
}

void from_json(const nlohmann::json &js, ExeSectionInfo &d)
{
    js.at("name").get_to(d.name);
    std::string type = js.at("type").get<std::string>();
    d.type = to_section_type(type.c_str());
    js.at("address").get_to(d.address);
    js.at("size").get_to(d.size);
}

} // namespace unassemblize
