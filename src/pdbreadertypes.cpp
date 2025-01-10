/**
 * @file
 *
 * @brief Types to store relevant symbols from PDB files
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "pdbreadertypes.h"
#include <nlohmann/json.hpp>

namespace unassemblize
{
// #TODO: Verify and improve error handling when a json field does not exist.
// Currently js.at("") should crash if that named field does not exit.

void to_json(nlohmann::json &js, const PdbAddress &d)
{
    js = nlohmann::json{
        {"virtual_abs", d.absVirtual},
        {"virtual_rel", d.relVirtual},
        {"section", d.section},
        {"offset", d.offset},
    };
}

void from_json(const nlohmann::json &js, PdbAddress &d)
{
    js.at("virtual_phy").get_to(d.absVirtual);
    js.at("virtual_rel").get_to(d.relVirtual);
    js.at("section").get_to(d.section);
    js.at("offset").get_to(d.offset);
}

void to_json(nlohmann::json &js, const PdbSourceLineInfo &d)
{
    js = nlohmann::json{
        {"line", d.lineNumber},
        {"off", d.offset},
        {"len", d.length},
    };
}

void from_json(const nlohmann::json &js, PdbSourceLineInfo &d)
{
    js.at("line").get_to(d.lineNumber);
    js.at("off").get_to(d.offset);
    js.at("len").get_to(d.length);
}

void to_json(nlohmann::json &js, const PdbCompilandInfo &d)
{
    js = nlohmann::json{
        {"name", d.name},
        {"source_file_ids", d.sourceFileIds},
        {"function_ids", d.functionIds},
    };
}

void from_json(const nlohmann::json &js, PdbCompilandInfo &d)
{
    js.at("name").get_to(d.name);
    js.at("source_file_ids").get_to(d.sourceFileIds);
    js.at("function_ids").get_to(d.functionIds);
}

void to_json(nlohmann::json &js, const PdbSourceFileInfo &d)
{
    js = nlohmann::json{
        {"name", d.name},
        {"chksum_type", d.checksumType},
        {"chksum", d.checksum},
        {"compiland_ids", d.compilandIds},
        {"function_ids", d.functionIds},
    };
}

void from_json(const nlohmann::json &js, PdbSourceFileInfo &d)
{
    js.at("name").get_to(d.name);
    js.at("chksum_type").get_to(d.checksumType);
    js.at("chksum").get_to(d.checksum);
    js.at("compiland_ids").get_to(d.compilandIds);
    js.at("function_ids").get_to(d.functionIds);
}

void to_json(nlohmann::json &js, const PdbSymbolInfo &d)
{
    js = nlohmann::json{
        {"address", d.address},
        {"len", d.length},
        {"name_decorated", d.decoratedName},
        {"name_undecorated", d.undecoratedName},
        {"name_global", d.globalName},
    };
}

void from_json(const nlohmann::json &js, PdbSymbolInfo &d)
{
    js.at("address").get_to(d.address);
    js.at("len").get_to(d.length);
    js.at("name_decorated").get_to(d.decoratedName);
    js.at("name_undecorated").get_to(d.undecoratedName);
    js.at("name_global").get_to(d.globalName);
}

void to_json(nlohmann::json &js, const PdbFunctionInfo &d)
{
    js = nlohmann::json{
        {"file_id", d.sourceFileId},
        {"compiland_id", d.compilandId},
        {"address", d.address},
        {"debug_start_address", d.debugStartAddress},
        {"debug_end_address", d.debugEndAddress},
        {"len", d.length},
        {"call", d.call},
        {"name_decorated", d.decoratedName},
        {"name_undecorated", d.undecoratedName},
        {"name_global", d.globalName},
        {"lines", d.sourceLines},
    };
}

void from_json(const nlohmann::json &js, PdbFunctionInfo &d)
{
    js.at("file_id").get_to(d.sourceFileId);
    js.at("compiland_id").get_to(d.compilandId);
    js.at("address").get_to(d.address);
    js.at("debug_start_address").get_to(d.debugStartAddress);
    js.at("debug_end_address").get_to(d.debugEndAddress);
    js.at("len").get_to(d.length);
    js.at("call").get_to(d.call);
    js.at("name_decorated").get_to(d.decoratedName);
    js.at("name_undecorated").get_to(d.undecoratedName);
    js.at("name_global").get_to(d.globalName);
    js.at("lines").get_to(d.sourceLines);
}

void to_json(nlohmann::json &js, const PdbExeInfo &d)
{
    js = nlohmann::json{
        {"exe_file_name", d.exeFileName},
        {"pdb_file_path", d.pdbFilePath},
    };
}

void from_json(const nlohmann::json &js, PdbExeInfo &d)
{
    js.at("exe_file_name").get_to(d.exeFileName);
    js.at("pdb_file_path").get_to(d.pdbFilePath);
}

} // namespace unassemblize
