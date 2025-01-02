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
#pragma once

#include "asmmatchertypes.h"
#include <string>
#include <string_view>

// When output is set to "auto", then output name is chosen for input file name.
inline constexpr char *const auto_str = "auto";
bool is_auto_str(std::string_view str);

enum class InputType
{
    Exe,
    Pdb,
    None,
};

inline constexpr char *const s_input_type_names[] = {"exe", "pdb", "none"};

std::string get_config_file_name(std::string_view input_file, std::string_view config_file);
std::string get_asm_output_file_name(std::string_view input_file, std::string_view output_file);
std::string get_cmp_output_file_name(
    std::string_view input_file0,
    std::string_view input_file1,
    std::string_view output_file);
InputType to_input_type(std::string_view str);
InputType get_input_type(std::string_view input_file, std::string_view input_type);

template<typename T>
struct CommandLineType
{
    CommandLineType() : v{} {}

    template<typename U>
    CommandLineType(const U &val) : v(val)
    {
    }

    template<typename U>
    void set_from_command_line(const U &val)
    {
        v = val;
        is_from_command_line = true;
    }

    operator T &() { return v; }
    operator const T &() const { return v; }

    T v;
    bool is_from_command_line = false;
};

struct CommandLineOptions
{
    static constexpr size_t MAX_INPUT_FILES = 2;

    CommandLineOptions()
    {
        std::fill_n(input_type, MAX_INPUT_FILES, auto_str);
        std::fill_n(config_file, MAX_INPUT_FILES, auto_str);
    }

    CommandLineType<std::string> input_file[MAX_INPUT_FILES];
    CommandLineType<std::string> input_type[MAX_INPUT_FILES];
    // When output_file is set to "auto", then output file name is chosen for input file name.
    CommandLineType<std::string> output_file = auto_str;
    CommandLineType<std::string> cmp_output_file = auto_str;
    CommandLineType<uint32_t> lookahead_limit = 20;
    CommandLineType<unassemblize::AsmMatchStrictness> match_strictness = unassemblize::AsmMatchStrictness::Undecided;
    CommandLineType<uint32_t> print_indent_len = 4;
    CommandLineType<uint32_t> print_asm_len = 80;
    CommandLineType<uint32_t> print_byte_count = 11;
    CommandLineType<uint32_t> print_sourcecode_len = 80;
    CommandLineType<uint32_t> print_sourceline_len = 5;
    CommandLineType<unassemblize::AsmFormat> format = unassemblize::AsmFormat::IGAS;
    CommandLineType<size_t> bundle_file_idx = 0;
    CommandLineType<unassemblize::MatchBundleType> bundle_type = unassemblize::MatchBundleType::SourceFile;
    // When config file is set to "auto", then config file name is chosen for input file name.
    CommandLineType<std::string> config_file[MAX_INPUT_FILES];
    CommandLineType<uint64_t> start_addr = 0x00000000;
    CommandLineType<uint64_t> end_addr = 0x00000000;
    CommandLineType<bool> print_secs = false;
    CommandLineType<bool> dump_syms = false;
    CommandLineType<bool> verbose = false;
    CommandLineType<bool> gui = false;
};
