#include "cpu/cpu.hpp"
#include "memory/bus.hpp"
#include <iostream>
#include <fstream>
#include <vector>

// Helper to read a binary file into a vector
std::vector<uint8_t> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open " + filename);
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    throw std::runtime_error("Error reading " + filename);
}

int main() {
    std::cout << "Booting RV64 Emulator Engine...\n";
    Bus bus;
    CPU cpu(&bus);

    try {
        // 1. Load the payloads from disk
        auto opensbi = read_file("payloads/fw_jump.bin");
        auto kernel  = read_file("payloads/Image");
        auto dtb     = read_file("payloads/virt.dtb");

        // 2. Inject payloads into DRAM at the exact physical offsets
        // bus.load_program(data, physical_address) - You may need to update 
        // your bus.hpp to accept an address offset for this to work!
        bus.load_program(opensbi, 0x80000000); 
        bus.load_program(kernel,  0x80200000);
        bus.load_program(dtb,     0x87000000);

        std::cout << "Payloads loaded into RAM.\n";

        // 3. The RISC-V Boot Protocol
        // Register a0 (x10) = Hart ID (0)
        cpu.set_reg(10, 0); 
        // Register a1 (x11) = Physical Address of the DTB
        cpu.set_reg(11, 0x87000000); 

        std::cout << "Starting CPU clock... Handing control to OpenSBI.\n\n";
        
        // 4. Start the execution engine!
        cpu.execute_loop();

    } catch (const std::exception& e) {
        std::cerr << "\n[HARDWARE FAULT] " << e.what() << "\n";
        cpu.dump_registers();
    }

    return 0;
}