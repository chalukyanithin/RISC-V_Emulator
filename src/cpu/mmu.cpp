#include "cpu/mmu.hpp"

TrapCause MMU::get_fault_cause(AccessType type) const {
    switch (type) {
        case AccessType::InstructionFetch: return TrapCause::InstructionPageFault;
        case AccessType::Load:             return TrapCause::LoadPageFault;
        case AccessType::Store:            return TrapCause::StorePageFault;
    }
    return TrapCause::InstructionPageFault; // Fallback
}

TranslationResult MMU::page_walk(uint64_t vaddr, AccessType type, PrivilegeMode mode) {
    // 1. Machine mode always bypasses virtual memory
    if (mode == PrivilegeMode::Machine) {
        return {vaddr, true, TrapCause::InstructionPageFault};
    }

    // 2. Read the satp register (Address 0x180)
    uint64_t satp = csr->read(0x180);
    uint8_t mmu_mode = (satp >> 60) & 0xF;

    // Mode 0 = Bare (Virtual Memory is disabled)
    if (mmu_mode == 0) {
        return {vaddr, true, TrapCause::InstructionPageFault};
    }

    // If it's not Bare and not Sv39 (Mode 8), it's a critical hardware failure
    if (mmu_mode != 8) {
        return {0, false, get_fault_cause(type)};
    }

    // 3. Sv39 Virtual Address Layout
    // Extract the 3 Virtual Page Numbers (VPNs) and the 12-bit offset
    uint64_t vpn[3] = {
        (vaddr >> 12) & 0x1FF, // VPN[0] (Bits 12-20)
        (vaddr >> 21) & 0x1FF, // VPN[1] (Bits 21-29)
        (vaddr >> 30) & 0x1FF  // VPN[2] (Bits 30-38)
    };
    uint64_t offset = vaddr & 0xFFF;

    // Extract the Root Page Table Physical Address from satp
    uint64_t root_ppn = satp & 0xFFFFFFFFFFF;
    uint64_t table_addr = root_ppn * 4096; // Shift up by 12 bits (page size)

    // 4. The Hardware Page Walk
    int levels = 2; // Sv39 walks levels 2, 1, 0
    uint64_t pte = 0;
    
    for (int i = levels; i >= 0; --i) {
        // Calculate the physical address of the specific Page Table Entry (PTE)
        // Each entry is 8 bytes, so we multiply the VPN index by 8
        uint64_t pte_addr = table_addr + (vpn[i] * 8);
        
        // Emulate the hardware reading from RAM
        try {
            pte = bus->read64(pte_addr);
        } catch (...) {
            // If the page table itself is in invalid physical memory, hardware faults
            return {0, false, get_fault_cause(type)};
        }

        uint8_t v = pte & 0x1;          // Valid
        uint8_t r = (pte >> 1) & 0x1;   // Read
        uint8_t w = (pte >> 2) & 0x1;   // Write
        uint8_t x = (pte >> 3) & 0x1;   // Execute

        // If the PTE is not valid, or if it's Write-Only (which is illegal in RISC-V)
        if (v == 0 || (r == 0 && w == 1)) {
            return {0, false, get_fault_cause(type)};
        }

        // If R, W, and X are all 0, this is a pointer to the next level of the table
        if (r == 0 && w == 0 && x == 0) {
            if (i == 0) {
                // We reached the bottom level and still didn't find a leaf. Fault.
                return {0, false, get_fault_cause(type)};
            }
            // Update table_addr to point to the next level down
            table_addr = ((pte >> 10) & 0xFFFFFFFFFFF) * 4096;
            continue;
        }

        // --- We Found a Leaf PTE (R, W, or X is 1) ---
        
        // TODO: (Phase 3.5) Privilege checks against the 'U' bit go here

        // Extract the final Physical Page Number from the PTE
        uint64_t pte_ppn = (pte >> 10) & 0xFFFFFFFFFFF;
        uint64_t paddr = 0;

        if (i == 0) {
            // Standard 4KB Page
            paddr = (pte_ppn * 4096) + offset;
        } else {
            // Megapage (2MB) or Gigapage (1GB) Superpage handling
            // In a superpage, the lower bits of the physical address are taken 
            // directly from the virtual address, bypassing the lower tables.
            
            if (i == 1) { // 2MB Megapage
                paddr = (pte_ppn * 4096) + (vpn[0] * 4096) + offset;
            } else if (i == 2) { // 1GB Gigapage
                paddr = (pte_ppn * 4096) + (vpn[1] * 512 * 4096) + (vpn[0] * 4096) + offset;
            }
        }

        return {paddr, true, TrapCause::InstructionPageFault};
    }

    return {0, false, get_fault_cause(type)};
}

TranslationResult MMU::translate(uint64_t vaddr, AccessType type, PrivilegeMode mode) {
    // 1. Machine mode always bypasses virtual memory entirely
    if (mode == PrivilegeMode::Machine) {
        return {vaddr, true, TrapCause::InstructionPageFault};
    }

    uint64_t satp = csr->read(0x180);
    if (((satp >> 60) & 0xF) == 0) {
        return {vaddr, true, TrapCause::InstructionPageFault}; // Bare mode
    }

    // 2. Extract the Virtual Page Number (VPN)
    // The VPN is everything above the 12-bit offset
    uint64_t vpn = vaddr >> 12;

    // 3. Hash the VPN to get a TLB array index
    // Because TLB_SIZE is 1024, (1024 - 1) gives us a mask of 0x3FF
    size_t index = vpn & (TLB_SIZE - 1);
    
    TLBEntry& entry = tlb[index];

    // 4. TLB Hit Check
    if (entry.valid && entry.vpn == vpn) {
        // HIT! Check permissions (Simplified for now)
        // In a real implementation, you check entry.privileges against 'type' and 'mode' here
        
        uint64_t offset = vaddr & 0xFFF;
        uint64_t paddr = (entry.ppn * 4096) + offset;
        return {paddr, true, TrapCause::InstructionPageFault};
    }

    // 5. TLB Miss: Fall back to the slow hardware page walk
    TranslationResult result = page_walk(vaddr, type, mode);

    // 6. If the page walk succeeded, update the TLB cache
    if (result.success) {
        entry.valid = true;
        entry.vpn = vpn;
        entry.ppn = result.paddr >> 12; 
        // entry.privileges = ... (extracted from PTE during page_walk)
    }

    return result;
}
