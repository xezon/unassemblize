/**
 * @file
 *
 * @brief Class encapsulating a single function disassembly.
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "executable.h"
#include "functiontypes.h"
#include <Zydis/Decoder.h>
#include <Zydis/Formatter.h>
#include <Zydis/SharedTypes.h>
#include <map>
#include <stdint.h>

struct ZydisDisassembledInstruction_;
using ZydisDisassembledInstruction = ZydisDisassembledInstruction_;

struct PdbSourceFileInfo;
struct PdbSourceLineInfoVector;

namespace unassemblize
{
class Function;

// Function disassemble setup class. Can be passed to multiple Function instances.
class FunctionSetup
{
    friend class Function;

public:
    explicit FunctionSetup(const Executable &executable, AsmFormat format = AsmFormat::DEFAULT);

private:
    ZyanStatus initialize();

    const Executable &m_executable;
    const AsmFormat m_format;
    ZydisStackWidth m_stackWidth;
    ZydisFormatterStyle m_style;
    ZydisDecoder m_decoder;
    ZydisFormatter m_formatter;

    ZydisFormatterFunc m_default_print_address_absolute;
    ZydisFormatterFunc m_default_print_address_relative;
    ZydisFormatterFunc m_default_print_displacement;
    ZydisFormatterFunc m_default_print_immediate;
    ZydisFormatterFunc m_default_format_operand_mem;
    ZydisFormatterFunc m_default_format_operand_ptr;
    ZydisFormatterRegisterFunc m_default_print_register;
};

// Function disassemble class.
class Function
{
    friend class FunctionSetup;

    using Address64ToIndexMap = std::map<Address64T, IndexT>;

public:
    Function() = default;

    // Set address range. Must not be called after disassemble, but can be called before.
    void set_address_range(Address64T begin_address, Address64T end_address);

    // Set source file info. Must not be called before disassembler, but can be called after.
    void set_source_file(const PdbSourceFileInfo &source_file, const PdbSourceLineInfoVector &source_lines);

    // Disassemble a function from begin to end with the given setup. The address range is free to choose, but it is best
    // used for a single function only. When complete, instruction data will be available.
    void disassemble(const FunctionSetup &setup, Address64T begin_address, Address64T end_address);
    void disassemble(const FunctionSetup &setup);

    Address64T get_begin_address() const { return m_beginAddress; }
    Address64T get_end_address() const { return m_endAddress; }

    const std::string &get_source_file_name() const { return m_sourceFileName; }

    const AsmInstructions &get_instructions() const { return m_instructions; }

    // The number of instruction addresses that refer to a symbol or pseudo symbol.
    uint32_t get_symbol_count() const { return m_symbolCount; }

    const AsmJumpDestinationInfo *get_jump_destination_info(Address64T address) const;

    const ExeSymbol *get_pseudo_symbol(Address64T address) const;

private:
    const FunctionSetup &get_setup() const;
    const Executable &get_executable() const;
    ZydisFormatterFunc get_default_print_address_absolute() const;
    ZydisFormatterFunc get_default_print_address_relative() const;
    ZydisFormatterFunc get_default_print_displacement() const;
    ZydisFormatterFunc get_default_print_immediate() const;
    ZydisFormatterFunc get_default_format_operand_mem() const;
    ZydisFormatterFunc get_default_format_operand_ptr() const;
    ZydisFormatterRegisterFunc get_default_print_register() const;

    void add_jump_destination(Address64T jumpDestination, Address64T jumpOrigin);

    bool add_pseudo_symbol(Address64T address, std::string_view prefix);
    const ExeSymbol *get_symbol(Address64T address) const;
    const ExeSymbol *get_symbol_from_image_base(Address64T address) const;

    // Zydis formatter callbacks
    static ZyanStatus UnasmFormatterPrintAddressAbsolute(
        const ZydisFormatter *formatter,
        ZydisFormatterBuffer *buffer,
        ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintAddressRelative(
        const ZydisFormatter *formatter,
        ZydisFormatterBuffer *buffer,
        ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintDISP(
        const ZydisFormatter *formatter,
        ZydisFormatterBuffer *buffer,
        ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintIMM(
        const ZydisFormatter *formatter,
        ZydisFormatterBuffer *buffer,
        ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterFormatOperandPTR(
        const ZydisFormatter *formatter,
        ZydisFormatterBuffer *buffer,
        ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterFormatOperandMEM(
        const ZydisFormatter *formatter,
        ZydisFormatterBuffer *buffer,
        ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintRegister(
        const ZydisFormatter *formatter,
        ZydisFormatterBuffer *buffer,
        ZydisFormatterContext *context,
        ZydisRegister reg);

    static ZyanStatus UnasmDisassembleNoFormat(
        const ZydisDecoder &decoder,
        ZyanU64 runtime_address,
        const void *buffer,
        ZyanUSize length,
        ZydisDisassembledInstruction &instruction);

    static ZyanStatus UnasmDisassembleCustom(
        const ZydisFormatter &formatter,
        const ZydisDecoder &decoder,
        ZyanU64 runtime_address,
        const void *buffer,
        ZyanUSize length,
        ZydisDisassembledInstruction &instruction,
        span<char> instruction_buffer,
        void *user_data);

private:
    // Setup used during disassemble step. Is nulled at the end of it.
    const FunctionSetup *m_setup = nullptr;

    Address64T m_beginAddress = 0;
    Address64T m_endAddress = 0;
    std::string m_sourceFileName;
    AsmInstructions m_instructions;

    AsmJumpDestinationInfos m_jumpDestinationInfos;
    Address64ToIndexMap m_jumpDestinationAddressToIndexMap;

    ExeSymbols m_pseudoSymbols;
    Address64ToIndexMap m_pseudoSymbolAddressToIndexMap;

    uint32_t m_symbolCount = 0;
};

const ExeSymbol *get_symbol_or_pseudo_symbol(Address64T address, const Executable &executable, const Function &function);

} // namespace unassemblize
