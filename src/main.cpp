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
        0x13, 0x02, 0xa0, 0x00, // ADDI x5, x0, 10
        0xef, 0x00, 0xc0, 0x00, // JAL x1, 12
        0x13, 0x03, 0x30, 0x06, // ADDI x6, x0, 99
        0x00, 0x00, 0x00, 0x00, // ILLEGAL (Forces emulator to halt)
        0x13, 0x02, 0x52, 0x00, // ADDI x5, x5, 5
        0x67, 0x80, 0x00, 0x00  // JALR x0, x1, 0
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