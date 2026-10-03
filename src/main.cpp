#include "cpu/cpu.hpp"
#include "memory/bus.hpp"
#include <iostream>
#include <vector>

int main() {
    std::cout << "Booting RV64 Emulator Engine...\n\n";

    // 1. Initialize the motherboard (The Bus)
    // This automatically provisions 128MB of virtual DRAM.
    Bus bus;

    // 2. Initialize the CPU and plug it into the Bus
    CPU cpu(&bus);

    // 3. Prepare a minimal machine-code payload
    // These bytes represent exactly what a C compiler would generate.
    // 
    // Instruction 1: LUI x5, 0x12345
    //   - Loads the value 0x12345000 into register 5 (x5)
    //   - Binary Encoding: 0x123452b7
    //   - Little-Endian Byte Order: 0xb7, 0x52, 0x34, 0x12
    //
    // Instruction 2: ADDI x6, x5, 0x001
    //   - We haven't implemented ADDI yet, but we will inject it to test 
    //     the CPU's "Illegal Instruction" crash handler.
    //   - Binary Encoding: 0x00128313
    //   - Little-Endian Byte Order: 0x13, 0x83, 0x12, 0x00
    
    std::vector<uint8_t> payload = {
        0xb7, 0x52, 0x34, 0x12, // LUI
        0x13, 0x83, 0x12, 0x00  // ADDI (Unimplemented)
    };

    // 4. Inject the payload directly into physical RAM at address 0x80000000
    bus.load_program(payload);

    std::cout << "Payload loaded into DRAM.\n";
    std::cout << "Starting CPU clock...\n\n";

    // 5. Run the CPU loop
    // Note: Since our execute_loop() in cpu.cpp currently has a temporary 
    // "break" statement at the end of it (from our previous step), it will 
    // only execute the first instruction and then exit.
    // 
    // You must go back to src/cpu/cpu.cpp and REMOVE the 'break;' 
    // at the end of the execute_loop() so it can process both instructions.
    
    cpu.execute_loop();

    std::cout << "\nCPU Halted. Final State:\n";
    
    // 6. Verify the math
    cpu.dump_registers();

    return 0;
}