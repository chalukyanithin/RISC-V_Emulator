#pragma once
#include <cstdint>
#include <vector>

class DRAM {
private:
    // Represents physical memory. 
    std::vector<uint8_t> mem;

public:
    // Default size is 128 MB (1024 * 1024 * 128 bytes)
    explicit DRAM(uint64_t size = 1024 * 1024 * 128) : mem(size, 0) {}

    // ---------------------------------------------------------
    // Memory Access Methods
    // ---------------------------------------------------------
    
    // RISC-V requires Little-Endian memory access.
    // We assemble the 32-bit value byte-by-byte. 
    // Optimization Note: On an x86-64 host (which is also Little-Endian), 
    // a modern C++ compiler like GCC or Clang will optimize these bitwise 
    // operations into a single, direct CPU 'mov' instruction. 
    // This provides 100% architectural safety with zero runtime cost.
    inline uint32_t load32(uint64_t addr) const {
        return static_cast<uint32_t>(mem[addr])
             | (static_cast<uint32_t>(mem[addr + 1]) << 8)
             | (static_cast<uint32_t>(mem[addr + 2]) << 16)
             | (static_cast<uint32_t>(mem[addr + 3]) << 24);
    }

    // We will need a method to inject binary code (like our bootloader) 
    // directly into memory from the host system.
    void store8(uint64_t addr, uint8_t val) {
        mem[addr] = val;
    }
};