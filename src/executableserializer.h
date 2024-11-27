#pragma once

#include "executabletypes.h"
#include <string>

namespace unassemblize
{

class Executable;

class ExecutableSerializer
{
public:
    void set_verbose(bool verbose) { m_verbose = verbose; }

    // File I/O methods
    bool load_config(const std::string &filename, Executable &exe, bool overwrite_symbols = false);
    bool save_config(const std::string &filename, const Executable &exe) const;

    // JSON methods
    void load_json(const nlohmann::json &js, Executable &exe, bool overwrite_symbols = false);
    void save_json(nlohmann::json &js, const Executable &exe) const;

private:
    // JSON section keys
    static constexpr const char *EXE_CONFIG_SECTION = "exe_config";
    static constexpr const char *EXE_SYMBOLS_SECTION = "exe_symbols";
    static constexpr const char *EXE_SECTIONS_SECTION = "exe_sections";
    static constexpr const char *EXE_OBJECTS_SECTION = "exe_objects";

    bool m_verbose = false;
};

} // namespace unassemblize