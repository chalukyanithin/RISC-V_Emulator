#pragma once
#include "memory/dram.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>

class Bus {
private:
    DRAM dram;
    
    // The standard base address for physical RAM in a RISC-V system
    static constexpr uint64_t DRAM_BASE = 0x80000000;
    static constexpr uint64_t DRAM_SIZE = 1024 * 1024 * 128; // 128 MB

public:
    Bus() : dram(DRAM_SIZE) {}

    // The CPU calls this to fetch instructions.
    inline uint32_t read32(uint64_t addr) const {
        
        // Check if the address falls within our physical RAM module
        if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
            
            // Subtract the base address to get the actual array index
            return dram.load32(addr - DRAM_BASE);
            
        }

        // If the address is below 0x80000000, it belongs to MMIO hardware.
        // We will implement hardware routing in Phase 4.
        std::cerr << "MMIO Trap: Unmapped memory read at 0x" << std::hex << addr << "\n";
        throw std::out_of_range("Memory access violation");
    }
    // Inside Bus class:
    inline uint64_t read64(uint64_t addr) const {
        if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
            return dram.load64(addr - DRAM_BASE);
        }
        throw std::out_of_range("MMIO Read64 out of bounds");
    }

    inline void write64(uint64_t addr, uint64_t val) {
        if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
            dram.store64(addr - DRAM_BASE, val);
            return;
        }
        throw std::out_of_range("MMIO Write64 out of bounds");
    }

    // Utility to load a raw binary program into the start of RAM
    void load_program(const std::vector<uint8_t>& code) {
        for (size_t i = 0; i < code.size(); ++i) {
            dram.store8(i, code[i]);
        }
    }
};