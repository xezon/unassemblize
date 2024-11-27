#pragma once

#include "executabletypes.h"
#include <nlohmann/json.hpp>
#include <string>

namespace unassemblize
{

class Executable; // Forward declaration

class ExecutableSerializer
{
public:
    // Main interface
    bool load(const std::string &filename, Executable &exe, bool overwrite_symbols = false);
    bool save(const std::string &filename, const Executable &exe) const;

    void set_verbose(bool verbose) { m_verbose = verbose; }

private:
    // JSON section keys
    static constexpr const char *SYMBOL_SECTION = "symbols";
    static constexpr const char *SECTIONS_SECTION = "sections";
    static constexpr const char *CONFIG_SECTION = "config";
    static constexpr const char *OBJECT_SECTION = "objects";

    // Configuration loading methods
    void loadConfig(const nlohmann::json &js, Executable &exe);
    void loadSymbols(const nlohmann::json &js, Executable &exe, bool overwrite_symbols);
    void loadSections(const nlohmann::json &js, Executable &exe);
    void loadObjects(const nlohmann::json &js, Executable &exe);

    // Configuration saving methods
    void saveConfig(nlohmann::json &js, const Executable &exe) const;
    void saveSymbols(nlohmann::json &js, const Executable &exe) const;
    void saveSections(nlohmann::json &js, const Executable &exe) const;
    void saveObjects(nlohmann::json &js, const Executable &exe) const;

    bool m_verbose = false;
};

} // namespace unassemblize