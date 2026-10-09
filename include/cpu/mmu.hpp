#pragma once
#include <cstdint>
#include <optional>
#include "memory/bus.hpp"
#include "cpu/csr.hpp" // For PrivilegeMode and TrapCause definitions
#include "cpu/cpu.hpp"

// The type of memory access being requested
enum class AccessType {
    InstructionFetch,
    Load,
    Store
};

// Zero-overhead return type for the MMU
struct TranslationResult {
    uint64_t paddr;
    bool success;
    TrapCause fault_cause;
};

// A single entry in the TLB cache
struct TLBEntry {
    uint64_t vpn;       // The Virtual Page Number (The "Tag")
    uint64_t ppn;       // The Physical Page Number (The "Target")
    bool valid;         // Is this entry currently in use?
    uint8_t privileges; // Cached Read/Write/Execute/User permissions
};

class MMU {
private:
    Bus* bus;
    CSR* csr;

    // The TLB Cache Size (Must be a power of 2 for fast bitwise masking)
    static constexpr size_t TLB_SIZE = 1024;
    
    // The flat array representing the TLB hardware
    std::array<TLBEntry, TLB_SIZE> tlb{};

    // Helper to throw the correct trap based on the access type
    TrapCause get_fault_cause(AccessType type) const;

    // The slow hardware page walk (we will rename your old translate() to this)
    TranslationResult page_walk(uint64_t vaddr, AccessType type, PrivilegeMode mode);

public:
    MMU(Bus* system_bus, CSR* system_csr) : bus(system_bus), csr(system_csr) {
        flush_tlb();
    }

    // The new, fast translation front-end
    TranslationResult translate(uint64_t vaddr, AccessType type, PrivilegeMode mode);

    // Clears the cache. Called when the OS changes processes.
    void flush_tlb() {
        for (auto& entry : tlb) {
            entry.valid = false;
        }
    }

    // Translates a virtual address to a physical address using Sv39
    TranslationResult translate(uint64_t vaddr, AccessType type, PrivilegeMode mode);
};