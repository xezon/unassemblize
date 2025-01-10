/**
 * @file
 *
 * @brief main function and option handling.
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "executable.h"
#include "pdbreader.h"
#include "runner.h"
#include "version.h"
#include <assert.h>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <inttypes.h>
#include <iostream>
#include <stdio.h>

#define BS_THREAD_POOL_DISABLE_EXCEPTION_HANDLING
#include <BS_thread_pool.hpp>

#ifdef _WIN32
#include "imguiclient/imguiwin32.h"
#include <Windows.h>
#else
#include "imguiclient/imguiglfw.h"
#endif

void CreateConsole()
{
#ifdef _WIN32
    if (::AllocConsole() == FALSE)
    {
        return;
    }

    // std::cout, std::clog, std::cerr, std::cin
    FILE *fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    // std::wcout, std::wclog, std::wcerr, std::wcin
    HANDLE hConOut = ::CreateFileA(
        "CONOUT$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    HANDLE hConIn = ::CreateFileA(
        "CONIN$",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    ::SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    ::SetStdHandle(STD_ERROR_HANDLE, hConOut);
    ::SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
#endif
}

static CommandLineOptions g_options;
static std::string g_parseError;
static std::string g_helpString;
static std::unique_ptr<BS::thread_pool> g_runnerThreadPool;
static std::unique_ptr<BS::thread_pool> g_workQueueThreadPool;

void parse_options(int argc, char **argv)
{
    cxxopts::Options options("unassemblize", "x86 Unassembly tool");

#define OPT_INPUT "input"
#define OPT_INPUT_2 "input2"
#define OPT_INPUTTYPE "input-type"
#define OPT_INPUTTYPE_2 "input2-type"
#define OPT_ASM_OUTPUT "output"
#define OPT_CMP_OUTPUT "cmp-output"
#define OPT_LOOKAHEAD_LIMIT "lookahead-limit"
#define OPT_MATCH_STRICTNESS "match-strictness"
#define OPT_PRINT_INDENT_LEN "print-indent-len"
#define OPT_PRINT_ASM_LEN "print-asm-len"
#define OPT_PRINT_BYTE_COUNT "print-byte-count"
#define OPT_PRINT_SOURCECODE_LEN "print-sourcecode-len"
#define OPT_PRINT_SOURCELINE_LEN "print-sourceline-len"
#define OPT_FORMAT "format"
#define OPT_BUNDLE_FILE_ID "bundle-file-id"
#define OPT_BUNDLE_TYPE "bundle-type"
#define OPT_CONFIG "config"
#define OPT_CONFIG_2 "config2"
#define OPT_START "start"
#define OPT_END "end"
#define OPT_LISTSECTIONS "list-sections"
#define OPT_DUMPSYMS "dumpsyms"
#define OPT_VERBOSE "verbose"
#define OPT_HELP "help"
#define OPT_GUI "gui"

    // clang-format off
    options.add_options("main", {
        cxxopts::Option{"i," OPT_INPUT, "Input file", cxxopts::value<std::string>()},
        cxxopts::Option{OPT_INPUT_2, "Input file 2", cxxopts::value<std::string>()},
        cxxopts::Option{OPT_INPUTTYPE, "Input file type. Default is 'auto'", cxxopts::value<std::string>(), R"(["auto", "exe", "pdb"])"},
        cxxopts::Option{OPT_INPUTTYPE_2, "Input file 2 type. Default is 'auto'", cxxopts::value<std::string>(), R"(["auto", "exe", "pdb"])"},
        cxxopts::Option{"o," OPT_ASM_OUTPUT, "Filename for single file output. Default is 'auto'", cxxopts::value<std::string>()},
        cxxopts::Option{OPT_CMP_OUTPUT, "Filename for comparison file output. Default is 'auto'", cxxopts::value<std::string>()},
        cxxopts::Option{OPT_LOOKAHEAD_LIMIT, "Max instruction count for trying to find a matching assembler line ahead. Default is 20.", cxxopts::value<uint32_t>()},
        cxxopts::Option{OPT_MATCH_STRICTNESS, "Assembler matching strictness. If 'lenient', then unknown to known/unknown symbol pairs are treated as match. If 'strict', then unknown to known/unknown symbol pairs are treated as mismatch. Default is 'undecided'.", cxxopts::value<std::string>(), R"(["lenient", "undecided", "strict"])"},
        cxxopts::Option{OPT_PRINT_INDENT_LEN, "Character count for indentation of assembler instructions in output report(s). Default is 4.", cxxopts::value<uint32_t>()},
        cxxopts::Option{OPT_PRINT_ASM_LEN, "Max character count for assembler instructions in comparison report(s). Text is truncated if longer. Default is 80.", cxxopts::value<uint32_t>()},
        cxxopts::Option{OPT_PRINT_BYTE_COUNT, "Max byte count in comparison report(s). Text is truncated if longer. Default is 11.", cxxopts::value<uint32_t>()},
        cxxopts::Option{OPT_PRINT_SOURCECODE_LEN, "Max character count for source code in comparison report(s). Text is truncated if longer. Default is 80.", cxxopts::value<uint32_t>()},
        cxxopts::Option{OPT_PRINT_SOURCELINE_LEN, "Max character count for source line in comparison report(s). Text is truncated if longer. Default is 5.", cxxopts::value<uint32_t>()},
        cxxopts::Option{"f," OPT_FORMAT, "Assembly output format. Default is 'igas'", cxxopts::value<std::string>(), R"(["igas", "agas", "masm", "default"])"},
        cxxopts::Option{OPT_BUNDLE_FILE_ID, "Input file used to bundle match results with. Default is 1.", cxxopts::value<size_t>(), "[1: 1st input file, 2: 2nd input file]"},
        cxxopts::Option{OPT_BUNDLE_TYPE, "Method used to bundle match results with. Default is 'sourcefile'.", cxxopts::value<std::string>(), R"(["none", "sourcefile", "compiland"])"},
        cxxopts::Option{"c," OPT_CONFIG, "Configuration file describing how to disassemble the input file and containing extra symbol info. Default is 'auto'", cxxopts::value<std::string>()},
        cxxopts::Option{OPT_CONFIG_2, "Configuration file describing how to disassemble the input file and containing extra symbol info. Default is 'auto'", cxxopts::value<std::string>()},
        cxxopts::Option{"s," OPT_START, "Starting address of a single function to disassemble in hexadecimal notation.", cxxopts::value<std::string>()},
        cxxopts::Option{"e," OPT_END, "Ending address of a single function to disassemble in hexadecimal notation.", cxxopts::value<std::string>()},
        cxxopts::Option{OPT_LISTSECTIONS, "Prints a list of sections in the executable then exits."},
        cxxopts::Option{"d," OPT_DUMPSYMS, "Dumps symbols stored in a executable or pdb to the config file."},
        cxxopts::Option{"v," OPT_VERBOSE, "Verbose output on current state of the program."},
        cxxopts::Option{"h," OPT_HELP, "Displays this help."},
        cxxopts::Option{"g," OPT_GUI, "Open with graphical user interface"},
        });
    // clang-format on
    static_assert(size_t(unassemblize::AsmFormat::DEFAULT) == 3, "Enum was changed. Update command line options.");
    static_assert(size_t(unassemblize::MatchBundleType::Count) == 3, "Enum was changed. Update command line options.");

    options.parse_positional({"input", "input2"});

    cxxopts::ParseResult result;

    try
    {
        result = options.parse(argc, argv);
    }
    catch (const std::exception &e)
    {
        g_parseError = fmt::format("Error parsing options: {:s}", e.what());
        return;
    }

    if (result.count("help") != 0)
    {
        g_helpString = options.help();
        return;
    }

    for (const cxxopts::KeyValue &kv : result.arguments())
    {
        const std::string &v = kv.key();
        if (v == OPT_INPUT)
        {
            g_options.input_file[0].set_from_command_line(kv.value());
        }
        else if (v == OPT_INPUT_2)
        {
            g_options.input_file[1].set_from_command_line(kv.value());
        }
        else if (v == OPT_INPUTTYPE)
        {
            g_options.input_type[0].set_from_command_line(kv.value());
        }
        else if (v == OPT_INPUTTYPE_2)
        {
            g_options.input_type[1].set_from_command_line(kv.value());
        }
        else if (v == OPT_ASM_OUTPUT)
        {
            g_options.output_file.set_from_command_line(kv.value());
        }
        else if (v == OPT_CMP_OUTPUT)
        {
            g_options.cmp_output_file.set_from_command_line(kv.value());
        }
        else if (v == OPT_LOOKAHEAD_LIMIT)
        {
            g_options.lookahead_limit.set_from_command_line(kv.as<uint32_t>());
        }
        else if (v == OPT_MATCH_STRICTNESS)
        {
            const auto type = unassemblize::to_asm_match_strictness(kv.value().c_str());
            g_options.match_strictness.set_from_command_line(type);
        }
        else if (v == OPT_PRINT_INDENT_LEN)
        {
            g_options.print_indent_len.set_from_command_line(kv.as<uint32_t>());
        }
        else if (v == OPT_PRINT_ASM_LEN)
        {
            g_options.print_asm_len.set_from_command_line(kv.as<uint32_t>());
        }
        else if (v == OPT_PRINT_BYTE_COUNT)
        {
            g_options.print_byte_count.set_from_command_line(kv.as<uint32_t>());
        }
        else if (v == OPT_PRINT_SOURCECODE_LEN)
        {
            g_options.print_sourcecode_len.set_from_command_line(kv.as<uint32_t>());
        }
        else if (v == OPT_PRINT_SOURCELINE_LEN)
        {
            g_options.print_sourceline_len.set_from_command_line(kv.as<uint32_t>());
        }
        else if (v == OPT_FORMAT)
        {
            const auto type = unassemblize::to_asm_format(kv.value().c_str());
            g_options.format.set_from_command_line(type);
        }
        else if (v == OPT_BUNDLE_FILE_ID)
        {
            g_options.bundle_file_idx.set_from_command_line(kv.as<size_t>() - 1);
        }
        else if (v == OPT_BUNDLE_TYPE)
        {
            const auto type = unassemblize::to_match_bundle_type(kv.value().c_str());
            g_options.bundle_type.set_from_command_line(type);
        }
        else if (v == OPT_CONFIG)
        {
            g_options.config_file[0].set_from_command_line(kv.value());
        }
        else if (v == OPT_CONFIG_2)
        {
            g_options.config_file[1].set_from_command_line(kv.value());
        }
        else if (v == OPT_START)
        {
            g_options.start_addr.set_from_command_line(strtoull(kv.value().c_str(), nullptr, 16));
        }
        else if (v == OPT_END)
        {
            g_options.end_addr.set_from_command_line(strtoull(kv.value().c_str(), nullptr, 16));
        }
        else if (v == OPT_LISTSECTIONS)
        {
            g_options.print_secs.set_from_command_line(kv.as<bool>());
        }
        else if (v == OPT_DUMPSYMS)
        {
            g_options.dump_syms.set_from_command_line(kv.as<bool>());
        }
        else if (v == OPT_VERBOSE)
        {
            g_options.verbose.set_from_command_line(kv.as<bool>());
        }
        else if (v == OPT_GUI)
        {
            g_options.gui.set_from_command_line(kv.as<bool>());
        }
    }
}

// Overwrite thread counts for testing.
//#define WORKQUEUE_THREADPOOL_COUNT 0
//#define RUNNER_THREADPOOL_COUNT 0

void initialize_threadpools(bool gui)
{
    const BS::concurrency_t threadCount = std::max<BS::concurrency_t>(std::thread::hardware_concurrency(), 1);
    BS::concurrency_t workQueueThreadCount;
    BS::concurrency_t runnerThreadCount;
    if (gui)
    {
        workQueueThreadCount = std::max<BS::concurrency_t>(threadCount / 4, 1);
        // -2 for main thread and work queue thread.
        runnerThreadCount = std::max<BS::concurrency_t>(threadCount - workQueueThreadCount - 2, 1);
    }
    else
    {
        workQueueThreadCount = 0;
        // -1 for main thread.
        runnerThreadCount = std::max<BS::concurrency_t>(threadCount - 1, 1);
    }

#if defined(WORKQUEUE_THREADPOOL_COUNT)
    workQueueThreadCount = WORKQUEUE_THREADPOOL_COUNT;
#endif
#if defined(RUNNER_THREADPOOL_COUNT)
    runnerThreadCount = RUNNER_THREADPOOL_COUNT;
#endif

    if (workQueueThreadCount > 0)
    {
        g_workQueueThreadPool = std::make_unique<BS::thread_pool>(workQueueThreadCount);
    }

    if (runnerThreadCount > 0)
    {
        g_runnerThreadPool = std::make_unique<BS::thread_pool>(runnerThreadCount);
        unassemblize::Runner::set_thread_pool(g_runnerThreadPool.get());
    }
}

std::unique_ptr<unassemblize::Executable> load_and_process_exe(
    std::string_view input_file,
    std::string_view config_file,
    const unassemblize::PdbReader *pdb_reader = nullptr)
{
    const std::string evaluated_config_file = get_config_file_name(input_file, config_file);
    std::unique_ptr<unassemblize::Executable> executable;

    {
        unassemblize::LoadExeOptions o(input_file);
        o.config_file = evaluated_config_file;
        o.pdb_reader = pdb_reader;
        o.verbose = g_options.verbose;
        executable = unassemblize::Runner::load_exe(o);
    }

    if (executable != nullptr)
    {
        if (g_options.dump_syms)
        {
            unassemblize::SaveExeConfigOptions o(*executable, evaluated_config_file);
            unassemblize::Runner::save_exe_config(o);
        }
        if (g_options.print_secs)
        {
            const unassemblize::ExeSections &sections = executable->get_sections();
            for (const unassemblize::ExeSectionInfo &section : sections)
            {
                printf(
                    "Name: %s, Address: 0x%" PRIx64 " Size: %" PRIu64 "\n",
                    section.name.c_str(),
                    section.address,
                    section.size);
            }
        }
    }

    return executable;
}

std::unique_ptr<unassemblize::PdbReader> load_and_process_pdb(std::string_view input_file, std::string_view config_file)
{
    std::unique_ptr<unassemblize::PdbReader> pdb_reader;

    {
        unassemblize::LoadPdbOptions o(input_file);
        o.verbose = g_options.verbose;
        pdb_reader = unassemblize::Runner::load_pdb(o);
    }

    if (pdb_reader != nullptr)
    {
        if (g_options.dump_syms)
        {
            const std::string evaluated_config_file = get_config_file_name(input_file, config_file);
            unassemblize::SavePdbConfigOptions o(*pdb_reader, evaluated_config_file);
            unassemblize::Runner::save_pdb_config(o);
        }
    }

    return pdb_reader;
}

int main(int argc, char **argv)
{
    parse_options(argc, argv);

    initialize_threadpools(g_options.gui);

    bool gui_error = false;

    if (g_options.gui)
    {
#if defined(_WIN32)
        // Windows GUI implementation
        unassemblize::gui::ImGuiWin32 gui;
#elif defined(__APPLE__) || defined(__linux__)
        // macOS and Linux GUI implementation using GLFW
        unassemblize::gui::ImGuiGLFW gui;
#else
        // Unsupported platform
        gui_error = true;
#endif
        unassemblize::gui::ImGuiStatus status = gui.run(g_options, g_workQueueThreadPool.get());
        return int(status);
    }
    else
    {
        CreateConsole();
    }

    std::cout << create_version_string() << std::endl;

    if (!g_parseError.empty())
    {
        std::cerr << g_parseError << std::endl;
        return 1;
    }

    if (!g_helpString.empty())
    {
        std::cout << g_helpString << std::endl;
        return 0;
    }

    if (gui_error)
    {
        std::cerr << "Gui not implemented. Exiting..." << std::endl;
        return 1;
    }

    if (g_options.input_file[0].v.empty())
    {
        std::cerr << "Missing input file argument. Exiting..." << std::endl;
        return 1;
    }

    std::array<std::unique_ptr<unassemblize::Executable>, 2> executable_pair;
    std::array<std::unique_ptr<unassemblize::PdbReader>, 2> pdb_reader_pair;

    bool ok = true;

    for (size_t idx = 0; idx < executable_pair.size() && ok; ++idx)
    {
        const InputType type = get_input_type(g_options.input_file[idx].v, g_options.input_type[idx].v);

        if (InputType::Exe == type)
        {
            executable_pair[idx] = load_and_process_exe(g_options.input_file[idx].v, g_options.config_file[idx].v);
            ok &= executable_pair[idx] != nullptr;
        }
        else if (InputType::Pdb == type)
        {
            pdb_reader_pair[idx] = load_and_process_pdb(g_options.input_file[idx].v, g_options.config_file[idx].v);
            ok &= pdb_reader_pair[idx] != nullptr;

            if (ok)
            {
                const unassemblize::PdbExeInfo &exe_info = pdb_reader_pair[idx]->get_exe_info();
                const std::string input_file = unassemblize::Runner::create_exe_filename(exe_info);
                executable_pair[idx] =
                    load_and_process_exe(input_file, g_options.config_file[idx].v, pdb_reader_pair[idx].get());
                ok &= executable_pair[idx] != nullptr;
            }
        }
        else if (idx == 0)
        {
            std::string text = fmt::format(
                "Unrecognized input type '{:s}' for input file '{:s}'. Exiting...",
                g_options.input_type[idx].v,
                g_options.input_file[idx].v);
            std::cerr << text << std::endl;
            return 1;
        }
    }

    if (ok)
    {
        const unassemblize::Executable *executable0 = executable_pair[0].get();
        const unassemblize::Executable *executable1 = executable_pair[1].get();

        if (executable0 != nullptr && !g_options.output_file.v.empty())
        {
            const std::string output_file = get_asm_output_file_name(executable0->get_filename(), g_options.output_file.v);
            unassemblize::AsmOutputOptions o(*executable0, output_file, g_options.start_addr, g_options.end_addr);
            o.format = g_options.format;
            o.print_indent_len = g_options.print_indent_len;
            ok &= unassemblize::Runner::process_asm_output(o);
        }

        if (executable0 != nullptr && executable1 != nullptr)
        {
            assert(executable0->is_loaded());
            assert(executable1->is_loaded());

            const std::string output_file =
                get_cmp_output_file_name(executable0->get_filename(), executable1->get_filename(), g_options.output_file.v);

            unassemblize::ConstExecutablePair executable_pair2 = {executable0, executable1};
            unassemblize::ConstPdbReaderPair pdb_reader_pair2 = {pdb_reader_pair[0].get(), pdb_reader_pair[1].get()};

            unassemblize::AsmComparisonOptions o(executable_pair2, pdb_reader_pair2, output_file);
            o.format = g_options.format;
            if (g_options.bundle_file_idx < 2 && pdb_reader_pair[g_options.bundle_file_idx] != nullptr)
            {
                o.bundle_file_idx = g_options.bundle_file_idx;
                o.bundle_type = g_options.bundle_type;
            }
            o.print_indent_len = g_options.print_indent_len;
            o.print_asm_len = g_options.print_asm_len;
            o.print_byte_count = g_options.print_byte_count;
            o.print_sourcecode_len = g_options.print_sourcecode_len;
            o.print_sourceline_len = g_options.print_sourceline_len;
            o.lookahead_limit = g_options.lookahead_limit;
            o.match_strictness = g_options.match_strictness;
            ok &= unassemblize::Runner::process_asm_comparison(o);
        }
    }
    return ok ? 0 : 1;
}
