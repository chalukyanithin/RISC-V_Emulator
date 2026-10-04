#pragma once

#include <array>
#include <cstdint>
#include <string>

class Bus;

class CPU
{
private:
    // ---------------------------------------------------------
    // Core State (Tightly Packed)
    // ---------------------------------------------------------

    // RV64 has 32 general-purpose registers (x0 - x31), each 64 bits wide.
    std::array<uint64_t, 32> regs{};

    // The Program Counter: Holds the memory address of the next instruction.
    // In our emulator, DRAM will start at physical address 0x80000000.
    uint64_t pc{0x80000000};

    // A pointer to the system bus for memory reads/writes.
    Bus *bus;

    // ---------------------------------------------------------
    // Helper Methods
    // ---------------------------------------------------------

    // RV64 requires that register x0 is hardwired to zero.
    // We enforce this by intercepting all register writes.
    // Marked 'inline' because this is called millions of times per second.
    inline void set_reg(uint8_t index, uint64_t val)
    {
        if (index != 0)
        {
            regs[index] = val;
        }
    }

    // Unsafe direct read for when we know the index is valid.
    inline uint64_t get_reg(uint8_t index) const
    {
        return regs[index];
    }

    inline uint8_t extract_opcode(uint32_t inst) const
    {
        return inst & 0x7F; // bottom 7 bits
    }

    inline uint8_t extract_rd(uint32_t inst) const
    {
        return (inst >> 7) & 0x1F; // 7-11
    }

    inline uint8_t extract_funct3(uint32_t inst) const
    {
        return (inst >> 12) & 0x7; // 12-14
    }

    inline uint8_t extract_rs1(uint32_t inst) const
    {
        return (inst >> 15) & 0x1F; // Bits 15-19
    }

    inline uint8_t extract_rs2(uint32_t inst) const
    {
        return (inst >> 20) & 0x1F; // Bits 20-24
    }

    inline uint8_t extract_funct7(uint32_t inst) const
    {
        return (inst >> 25) & 0x7F; // Bits 25-31
    }

    void exec_lui(uint32_t inst);
    void exec_addi(uint32_t inst);
    //void exec_lui(uint32_t inst);
    void exec_op_imm(uint32_t inst); // Handle all 0x13 instructions (ADDI, etc.)
    void exec_load(uint32_t inst);
    void exec_store(uint32_t inst);
public:
    explicit CPU(Bus *system_bus);
    ~CPU() = default;

    void execute_loop();

    void dump_registers() const;
};