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
#include <Zydis/Decoder.h>
#include <Zydis/Formatter.h>
#include <Zydis/SharedTypes.h>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

struct ZydisDisassembledInstruction_;
using ZydisDisassembledInstruction = ZydisDisassembledInstruction_;

namespace unassemblize
{
/*
 * Intermediate instruction data between Zydis disassemble and final text generation.
 */
struct InstructionData
{
    Address64T address; // Position of the instruction within the executable.
    bool isJump : 1; // Instruction is a jump.
    bool isInvalid : 1; // Instruction was not read or formatted correctly.
    union
    {
        int16_t jumpLen; // Jump length in bytes.
    };
    std::string instruction; // Instruction mnemonics and operands with address symbol substitution.
    std::string label; // Function or Jump label before this instruction.
};
using InstructionDataVector = std::vector<InstructionData>;

/*
 * Generate pure text from an instruction data vector.
 */
void append_as_text(std::string &str, const InstructionDataVector &instructions);

enum class AsmFormat
{
    DEFAULT,
    IGAS,
    AGAS,
    MASM,
};

class Function;

/*
 * Function disassemble setup class. Can be passed to multiple Function instances.
 */
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

/*
 * Function disassemble class.
 */
class Function
{
    friend class FunctionSetup;

    using Address64ToIndexMap = std::map<Address64T, IndexT>;

public:
    Function() = default;

    /*
     * Disassemble a function from begin to end with the given setup. The address range is free to choose, but it is best
     * used for a single function only. When complete, instruction data will be available.
     */
    void disassemble(const FunctionSetup *setup, Address64T begin_address, Address64T end_address);

    const InstructionDataVector &get_instructions() const;
    Address64T get_begin_address() const;
    Address64T get_end_address() const;

private:
    const Executable &get_executable() const;
    ZydisFormatterFunc get_default_print_address_absolute() const;
    ZydisFormatterFunc get_default_print_address_relative() const;
    ZydisFormatterFunc get_default_print_displacement() const;
    ZydisFormatterFunc get_default_print_immediate() const;
    ZydisFormatterFunc get_default_format_operand_mem() const;
    ZydisFormatterFunc get_default_format_operand_ptr() const;
    ZydisFormatterRegisterFunc get_default_print_register() const;

    void add_pseudo_symbol(Address64T address);
    const ExeSymbol &get_symbol(Address64T address) const;
    const ExeSymbol &get_symbol_from_image_base(Address64T address) const;
    const ExeSymbol &get_nearest_symbol(Address64T address) const; // TODO: investigate

    // Zydis formatter callbacks
    static ZyanStatus UnasmFormatterPrintAddressAbsolute(
        const ZydisFormatter *formatter, ZydisFormatterBuffer *buffer, ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintAddressRelative(
        const ZydisFormatter *formatter, ZydisFormatterBuffer *buffer, ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintDISP(
        const ZydisFormatter *formatter, ZydisFormatterBuffer *buffer, ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintIMM(
        const ZydisFormatter *formatter, ZydisFormatterBuffer *buffer, ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterFormatOperandPTR(
        const ZydisFormatter *formatter, ZydisFormatterBuffer *buffer, ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterFormatOperandMEM(
        const ZydisFormatter *formatter, ZydisFormatterBuffer *buffer, ZydisFormatterContext *context);
    static ZyanStatus UnasmFormatterPrintRegister(
        const ZydisFormatter *formatter, ZydisFormatterBuffer *buffer, ZydisFormatterContext *context, ZydisRegister reg);

    static ZyanStatus UnasmDisassembleNoFormat(const ZydisDecoder &decoder, ZyanU64 runtime_address, const void *buffer,
        ZyanUSize length, ZydisDisassembledInstruction &instruction);
    static ZyanStatus UnasmDisassembleCustom(const ZydisFormatter &formatter, const ZydisDecoder &decoder,
        ZyanU64 runtime_address, const void *buffer, ZyanUSize length, ZydisDisassembledInstruction &instruction,
        std::string &instruction_buffer, void *user_data);

private:
    const FunctionSetup *m_setup = nullptr;
    Address64T m_beginAddress = 0;
    Address64T m_endAddress = 0;

    // Symbols used within disassemble step. Is cleared at the end of it.
    ExeSymbols m_pseudoSymbols;
    Address64ToIndexMap m_pseudoSymbolAddressToIndexMap;

    InstructionDataVector m_instructions;
};
} // namespace unassemblize
