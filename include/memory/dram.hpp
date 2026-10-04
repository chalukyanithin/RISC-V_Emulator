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
    // Inside DRAM class:
    inline uint64_t load64(uint64_t addr) const {
        // Compose a 64-bit value from two 32-bit little-endian loads
        return static_cast<uint64_t>(load32(addr)) | 
              (static_cast<uint64_t>(load32(addr + 4)) << 32);
    }

    inline void store64(uint64_t addr, uint64_t val) {
        // Break the 64-bit value into bytes and store sequentially
        mem[addr]     = val & 0xFF;
        mem[addr + 1] = (val >> 8) & 0xFF;
        mem[addr + 2] = (val >> 16) & 0xFF;
        mem[addr + 3] = (val >> 24) & 0xFF;
        mem[addr + 4] = (val >> 32) & 0xFF;
        mem[addr + 5] = (val >> 40) & 0xFF;
        mem[addr + 6] = (val >> 48) & 0xFF;
        mem[addr + 7] = (val >> 56) & 0xFF;
    }
    // We will need a method to inject binary code (like our bootloader) 
    // directly into memory from the host system.
    void store8(uint64_t addr, uint8_t val) {
        mem[addr] = val;
    }
};