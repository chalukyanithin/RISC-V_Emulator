#include "cpu/cpu.hpp"
#include "cpu/csr.hpp"
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
        uint64_t inst_pc = pc;
        
        // 1. FETCH
        // We ask the Bus to read 4 bytes (32 bits) from memory at the current PC.
        uint32_t inst = bus->read32(pc); 
        
        // STUB: Since the Bus isn't built yet, we will simulate fetching an instruction.
        // Let's pretend we fetched: LUI x5, 0x12345 
        // (Loads the value 0x12345000 into register 5)
        // The binary encoding for this specific instruction is: 0x123452b7
        //uint32_t inst = 0x123452b7; 

        // 2. DECODE
        uint8_t opcode = extract_opcode(inst);
        pc += 4;

        // 3. EXECUTE
        // ... inside CPU::execute_loop() ...

        // 3. EXECUTE
        switch (opcode) {
            case 0x3:
                exec_load(inst);
                break;
            case 0x13:
                exec_op_imm(inst);
                break;
            case 0x23:
                exec_store(inst);
                break;
            case 0x37: // LUI
                exec_lui(inst);
                break;
            case 0x67: // JALR
                exec_jalr(inst, inst_pc);
                break;
            case 0x6F: // JAL
                exec_jal(inst, inst_pc);
                break;
            case 0x73: // SYSTEM (CSR manipulation, ECALL, EBREAK)
                exec_system(inst, inst_pc);
                break;
            default:
                std::cerr << "TRAP: Illegal Instruction executed at PC: 0x" 
                          << std::hex << pc << "\n";
                return; 
        }

         
    }
}

// ---------------------------------------------------------
// Instruction Execution
// ---------------------------------------------------------

void CPU::exec_op_imm(uint32_t inst) {
    uint8_t rd = extract_rd(inst);
    uint8_t rs1 = extract_rs1(inst);
    uint8_t funct3 = extract_funct3(inst);

    // The C++ Sign-Extension Trick:
    // By casting the 32-bit unsigned instruction to a SIGNED 32-bit integer,
    // and then right-shifting by 20, the C++ compiler automatically performs 
    // an "Arithmetic Shift". It shifts the top 12 bits down and fills the 
    // empty upper bits with the sign bit (1s if negative, 0s if positive).
    // We then cast it to 64-bit to match our registers.
    int64_t imm = static_cast<int64_t>(static_cast<int32_t>(inst) >> 20);

    switch (funct3) {
        case 0x0: { // 0x0 is ADDI (Add Immediate)
            // regs[rd] = regs[rs1] + imm
            uint64_t val1 = get_reg(rs1);
            set_reg(rd, val1 + imm);
            break;
        }
        default:
            std::cerr << "TRAP: Unimplemented OP-IMM funct3: " << (int)funct3 << "\n";
            // We will implement proper hardware traps in Phase 2
            break; 
    }
}

// Load Instruction (I-Type format)
void CPU::exec_load(uint32_t inst) {
    uint8_t rd = extract_rd(inst);
    uint8_t rs1 = extract_rs1(inst);
    uint8_t funct3 = extract_funct3(inst);
    
    // I-Type sign-extension (same as ADDI)
    int64_t imm = static_cast<int64_t>(static_cast<int32_t>(inst) >> 20);
    uint64_t addr = get_reg(rs1) + imm;

    switch (funct3) {
        case 0x3: // LD (Load Doubleword - 64 bit)
            set_reg(rd, bus->read64(addr));
            break;
        default:
            std::cerr << "TRAP: Unimplemented LOAD funct3: " << (int)funct3 << "\n";
            break;
    }
}

// Store Instruction (S-Type format)
void CPU::exec_store(uint32_t inst) {
    uint8_t rs1 = extract_rs1(inst);
    uint8_t rs2 = extract_rs2(inst);
    uint8_t funct3 = extract_funct3(inst);

    // The S-Type Reassembly Trick:
    // 1. (inst & 0xFE000000) isolates the top 7 bits. We cast to signed int32 
    //    and shift right by 20 to move them to positions 11-5, sign-extending automatically.
    // 2. ((inst >> 7) & 0x1F) extracts the bottom 5 bits.
    // 3. We bitwise OR them together to recreate the 12-bit signed offset.
    int64_t imm = static_cast<int64_t>(static_cast<int32_t>(inst & 0xFE000000) >> 20) 
                  | ((inst >> 7) & 0x1F);
                  
    uint64_t addr = get_reg(rs1) + imm;

    switch (funct3) {
        case 0x3: // SD (Store Doubleword - 64 bit)
            bus->write64(addr, get_reg(rs2));
            break;
        default:
            std::cerr << "TRAP: Unimplemented STORE funct3: " << (int)funct3 << "\n";
            break;
    }
}

void CPU::exec_branch(uint32_t inst, uint64_t inst_pc) {
    uint8_t rs1 = extract_rs1(inst);
    uint8_t rs2 = extract_rs2(inst);
    uint8_t funct3 = extract_funct3(inst);

    // Reassemble the scrambled B-Type immediate
    // 1. Bit 31 -> 12 (Sign extended via arithmetic right shift)
    // 2. Bit 7 -> 11
    // 3. Bits 30:25 -> 10:5
    // 4. Bits 11:8 -> 4:1
    int64_t imm = static_cast<int64_t>(static_cast<int32_t>(inst & 0x80000000) >> 19)
                | ((inst & 0x80) << 4)
                | ((inst >> 20) & 0x7E0)
                | ((inst >> 7) & 0x1E);

    uint64_t val1 = get_reg(rs1);
    uint64_t val2 = get_reg(rs2);
    bool take_branch = false;

    switch (funct3) {
        case 0x0: // BEQ (Branch if Equal)
            take_branch = (val1 == val2);
            break;
        case 0x1: // BNE (Branch if Not Equal)
            take_branch = (val1 != val2);
            break;
        // Other branches (BLT, BGE) will go here later
        default:
            std::cerr << "TRAP: Unimplemented BRANCH funct3: " << (int)funct3 << "\n";
            break;
    }

    if (take_branch) {
        pc = inst_pc + imm; // Update the Program Counter
    }
}

// JAL (Jump and Link) - J-Type Format
void CPU::exec_jal(uint32_t inst, uint64_t inst_pc) {
    uint8_t rd = extract_rd(inst);

    // Reassemble the scrambled J-Type 20-bit immediate
    // 1. Bit 31 -> 20 (Sign extended)
    // 2. Bits 30:21 -> 10:1
    // 3. Bit 20 -> 11
    // 4. Bits 19:12 -> 19:12
    int64_t imm = static_cast<int64_t>(static_cast<int32_t>(inst & 0x80000000) >> 11)
                | (inst & 0xFF000)
                | ((inst >> 9) & 0x800)
                | ((inst >> 20) & 0x7FE);

    // 1. Link: Save the return address (PC + 4) into rd.
    // Note: inst_pc is the PC of this JAL instruction.
    set_reg(rd, inst_pc + 4);

    // 2. Jump: Update the PC.
    pc = inst_pc + imm;
}

// JALR (Jump and Link Register) - I-Type Format
void CPU::exec_jalr(uint32_t inst, uint64_t inst_pc) {
    uint8_t rd = extract_rd(inst);
    uint8_t rs1 = extract_rs1(inst);
    
    // Standard I-Type sign-extension
    int64_t imm = static_cast<int64_t>(static_cast<int32_t>(inst) >> 20);

    // 1. Calculate target: rs1 + imm. 
    // RISC-V requires clearing the lowest bit to ensure 2-byte alignment.
    uint64_t target = (get_reg(rs1) + imm) & ~1ULL;

    // 2. Link: Save return address.
    set_reg(rd, inst_pc + 4);

    // 3. Jump: Update the PC.
    pc = target;
}

void CPU::exec_system(uint32_t inst, uint64_t inst_pc) {
    uint8_t rd = extract_rd(inst);
    uint8_t rs1 = extract_rs1(inst);
    uint8_t funct3 = extract_funct3(inst);
    
    // The CSR address is stored in the upper 12 bits of the instruction
    uint16_t csr_addr = (inst >> 20) & 0xFFF;

    if (funct3 == 0x0) {
        // These are not CSR instructions; they are environment calls/breaks.
        switch (csr_addr) {
            case 0x000: { // ECALL
                // Determine cause based on current privilege mode
                TrapCause cause;
                if (mode == PrivilegeMode::User) cause = TrapCause::EnvironmentCallFromUMode;
                else if (mode == PrivilegeMode::Supervisor) cause = TrapCause::EnvironmentCallFromSMode;
                else cause = TrapCause::EnvironmentCallFromMMode;
                
                // Trigger the trap. 
                // The PC passed is the PC of the ECALL instruction itself.
                trap(cause, inst_pc);
                return; // Stop execution of this instruction and immediately fetch from the trap vector
            }
            case 0x001: // EBREAK
                trap(TrapCause::Breakpoint, inst_pc);
                return;
            // (MRET and SRET will go here later to return from traps)
            default:
                std::cerr << "TRAP: Illegal SYSTEM instruction at PC: 0x" << std::hex << inst_pc << "\n";
                trap(TrapCause::IllegalInstruction, inst_pc, inst);
                return;
        }
    }

    switch (funct3) {
        case 0x1: { // CSRRW (Read / Write)
            uint64_t old_val = csr.read(csr_addr);
            csr.write(csr_addr, get_reg(rs1));
            // Only update destination if rd is not x0 (hardwired to 0)
            if (rd != 0) {
                set_reg(rd, old_val);
            }
            break;
        }
        case 0x2: { // CSRRS (Read / Set)
            uint64_t old_val = csr.read(csr_addr);
            // If rs1 is x0, this is a pure read, no write occurs.
            if (rs1 != 0) {
                csr.write(csr_addr, old_val | get_reg(rs1));
            }
            if (rd != 0) {
                set_reg(rd, old_val);
            }
            break;
        }
        case 0x3: { // CSRRC (Read / Clear)
            uint64_t old_val = csr.read(csr_addr);
            if (rs1 != 0) {
                // Bitwise AND with the bitwise NOT of rs1 clears the bits
                csr.write(csr_addr, old_val & ~get_reg(rs1));
            }
            if (rd != 0) {
                set_reg(rd, old_val);
            }
            break;
        }
        // ... (ECALL and EBREAK will go here)
        default:
            std::cerr << "TRAP: Unimplemented SYSTEM funct3: " << (int)funct3 << "\n";
            break;
    }

    

    
}

void CPU::trap(TrapCause cause, uint64_t epc, uint64_t tval) {
    // 1. Save the Program Counter where the exception occurred
    csr.write(MEPC, epc);

    // 2. Save the cause of the trap
    csr.write(MCAUSE, static_cast<uint64_t>(cause));

    // 3. Save any trap-specific value (e.g., the bad memory address for a page fault)
    // For ECALLs, this is usually 0.
    csr.write(0x343, tval); // 0x343 is MTVAL (Machine Trap Value)

    // 4. Save previous privilege mode and disable interrupts (simplified for Phase 2)
    // Real hardware manipulates specific bits in the 'mstatus' CSR here to preserve
    // the previous state so the OS can execute an MRET instruction to return later.
    
    // 5. Elevate privilege to Machine Mode
    mode = PrivilegeMode::Machine;

    // 6. Jump to the Kernel's trap handler
    // The OS sets up MTVEC (0x305) during boot. We read it, mask out the 
    // lowest 2 bits (which dictate vectoring modes), and jump there.
    uint64_t trap_vector = csr.read(0x305) & ~0x3ULL;
    pc = trap_vector;
}