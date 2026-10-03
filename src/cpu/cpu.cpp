#include "cpu/cpu.hpp"
#include "memory/bus.hpp" // We will uncomment this when we build the Bus
#include <iostream>
#include <iomanip>

// ---------------------------------------------------------
// Constructor
// ---------------------------------------------------------
CPU::CPU(Bus* system_bus) : bus(system_bus) {
    // The Program Counter is initialized to 0x80000000 in the header.
    // Register x0 is hardwired to 0. We ensure the array starts at 0.
    regs.fill(0);
}

// ---------------------------------------------------------
// Debugging Utility
// ---------------------------------------------------------
// Prints the current state of all 32 registers in hex format.
// Essential for verifying your math against a known good compiler output.
void CPU::dump_registers() const {
    std::cout << "--- CPU Register State ---\n";
    for (size_t i = 0; i < 32; i += 4) {
        std::cout << std::dec << "x" << std::setfill('0') << std::setw(2) << i << ": "
                  << std::hex << "0x" << std::setfill('0') << std::setw(16) << regs[i] << "  "
                  
                  << std::dec << "x" << std::setfill('0') << std::setw(2) << i+1 << ": "
                  << std::hex << "0x" << std::setfill('0') << std::setw(16) << regs[i+1] << "  "
                  
                  << std::dec << "x" << std::setfill('0') << std::setw(2) << i+2 << ": "
                  << std::hex << "0x" << std::setfill('0') << std::setw(16) << regs[i+2] << "  "
                  
                  << std::dec << "x" << std::setfill('0') << std::setw(2) << i+3 << ": "
                  << std::hex << "0x" << std::setfill('0') << std::setw(16) << regs[i+3] << "\n";
    }
    std::cout << "PC : " << std::hex << "0x" << std::setfill('0') << std::setw(16) << pc << "\n";
    std::cout << "--------------------------\n";
}

// ---------------------------------------------------------
// Instruction Execution
// ---------------------------------------------------------

// LUI: Load Upper Immediate
// This instruction takes a 20-bit immediate value, shifts it left by 12 bits,
// and stores it in the destination register (rd).
void CPU::exec_lui(uint32_t inst) {
    // 1. Extract the destination register (rd) using our inline helper
    uint8_t rd = extract_rd(inst);

    // 2. Extract the 20-bit immediate value.
    // In the RISC-V LUI format, the immediate is stored in bits 12-31.
    // We extract it by masking the bottom 12 bits to 0.
    uint32_t imm = inst & 0xFFFFF000;

    // 3. RISC-V requires that 32-bit values loaded into 64-bit registers
    // are "sign-extended". If the highest bit (bit 31) is a 1, we must fill 
    // the upper 32 bits of our 64-bit register with 1s.
    // Casting to int32_t first forces the C++ compiler to perform an arithmetic 
    // (sign-extending) cast when converting up to uint64_t.
    uint64_t sign_extended_imm = static_cast<uint64_t>(static_cast<int32_t>(imm));

    // 4. Save the result
    set_reg(rd, sign_extended_imm);
}

// ---------------------------------------------------------
// The Core Loop
// ---------------------------------------------------------
void CPU::execute_loop() {
    // For now, we will break the loop manually during testing.
    // Later, this runs infinitely until the OS halts.
    while (true) {
        
        // 1. FETCH
        // We ask the Bus to read 4 bytes (32 bits) from memory at the current PC.
        uint32_t inst = bus->read32(pc); 
        
        // STUB: Since the Bus isn't built yet, we will simulate fetching an instruction.
        // Let's pretend we fetched: LUI x5, 0x12345 
        // (Loads the value 0x12345000 into register 5)
        // The binary encoding for this specific instruction is: 0x123452b7
        uint32_t inst = 0x123452b7; 

        // 2. DECODE
        uint8_t opcode = extract_opcode(inst);

        // 3. EXECUTE
        switch (opcode) {
            case 0x37: // 0x37 is the official RISC-V opcode for LUI
                exec_lui(inst);
                break;
            default:
                std::cerr << "TRAP: Illegal Instruction executed at PC: 0x" 
                          << std::hex << pc << "\n";
                return; // Stop the emulator on a crash
        }

        // 4. ADVANCE
        pc += 4; // Move to the next 32-bit instruction

        // Temporary break for our stub test so it doesn't loop forever
        break; 
    }
}