#pragma once
#include <array>
#include <cstdint>
#include <iostream>

// Standard RISC-V Privilege Levels
enum class PrivilegeMode : uint8_t {
    User       = 0b00,
    Supervisor = 0b01,
    Machine    = 0b11
};

// Common CSR Addresses
// These are standard hardware addresses defined by the RISC-V manual
constexpr uint16_t MSTATUS = 0x300; // Machine Status
constexpr uint16_t MEDELEG = 0x302; // Machine Exception Delegation
constexpr uint16_t MIDELEG = 0x303; // Machine Interrupt Delegation
constexpr uint16_t MEPC    = 0x341; // Machine Exception Program Counter
constexpr uint16_t MCAUSE  = 0x342; // Machine Trap Cause

class CSR {
private:
    std::array<uint64_t, 4096> csrs{};

public:
    CSR() {
        csrs.fill(0);
    }

    // Hardware checks permissions before reading
    uint64_t read(uint16_t addr) const {
        // TODO: Privilege checking logic will go here
        return csrs[addr];
    }

    // Hardware checks permissions and triggers side-effects on write
    void write(uint16_t addr, uint64_t val) {
        // TODO: Privilege checking and read-only mask logic will go here
        csrs[addr] = val;
    }
};