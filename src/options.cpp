/**
 * @file
 *
 * @brief Option types
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "options.h"
#include "util.h"
#include <filesystem>

bool is_auto_str(std::string_view str)
{
    return util::equals_nocase(str, auto_str);
}

std::string get_config_file_name(std::string_view input_file, std::string_view config_file)
{
    if (is_auto_str(config_file))
    {
        if (input_file.empty())
        {
            return {};
        }

        // path/program.config.json
        std::filesystem::path path = input_file;
        path.replace_extension("config.json");
        return path.string();
    }
    return std::string(config_file);
}

std::string get_asm_output_file_name(std::string_view input_file, std::string_view output_file)
{
    if (is_auto_str(output_file))
    {
        if (input_file.empty())
        {
            return {};
        }

        // path/program.S
        std::filesystem::path path = input_file;
        path.replace_extension("S");
        return path.string();
    }
    return std::string(output_file);
}

std::string get_cmp_output_file_name(
    std::string_view input_file0,
    std::string_view input_file1,
    std::string_view output_file)
{
    if (is_auto_str(output_file))
    {
        // path0/program0_program1_cmp.txt
        std::filesystem::path path0 = input_file0;
        std::filesystem::path path1 = input_file1;
        std::filesystem::path path = path0.parent_path();
        path /= path0.stem();
        path += "_";
        path += path1.stem();
        path += "_cmp.txt";
        return path.string();
    }
    return std::string(output_file);
}

InputType to_input_type(std::string_view str)
{
    if (util::equals_nocase(str, s_input_type_names[size_t(InputType::Pdb)]))
    {
        return InputType::Pdb;
    }
    else if (util::equals_nocase(str, s_input_type_names[size_t(InputType::Exe)]))
    {
        return InputType::Exe;
    }
    else
    {
        printf("Unrecognized input type '%.*s'. Defaulting to 'None'", PRINTF_STRING(str));
        return InputType::None;
    }
    static_assert(size_t(InputType::None) == 2, "Enum was changed. Update conditions.");
}

InputType get_input_type(std::string_view input_file, std::string_view input_type)
{
    InputType type = InputType::None;

    if (!input_file.empty())
    {
        if (is_auto_str(input_type))
        {
            std::string input_file_ext = util::get_file_ext(input_file);
            if (util::equals_nocase(input_file_ext.c_str(), "pdb"))
            {
                type = InputType::Pdb;
            }
            else
            {
                type = InputType::Exe;
            }
        }
        else
        {
            type = to_input_type(input_type);
        }
    }
    return type;
}
