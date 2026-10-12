#pragma once
#include <cstdint>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>

class UART {
private:
    std::mutex mtx;
    std::queue<uint8_t> rx_buffer; // Buffer for incoming keyboard strokes

public:
    UART() {
        // In a full implementation, you would start a background C++ thread here 
        // to listen for std::cin without blocking the emulator. 
        // For now, we will focus purely on the Transmit (output) side.
    }

    // Called by the Bus when the CPU executes a Load instruction to the UART space
    uint8_t load(uint64_t addr) {
        std::lock_guard<std::mutex> lock(mtx);
        
        switch (addr) {
            case 0x00: { // RHR: Receive Holding Register
                if (rx_buffer.empty()) return 0;
                uint8_t c = rx_buffer.front();
                rx_buffer.pop();
                return c;
            }
            case 0x05: { // LSR: Line Status Register
                // Bit 5 (0x20): Transmit Holding Register Empty (We are always ready to print)
                // Bit 0 (0x01): Data Ready (Is there a keystroke in the buffer?)
                uint8_t lsr = 0x20; 
                if (!rx_buffer.empty()) {
                    lsr |= 0x01;
                }
                return lsr;
            }
            default:
                return 0; // Other registers (Interrupt Enables, FIFOs) are ignored for basic boot
        }
    }

    // Called by the Bus when the CPU executes a Store instruction to the UART space
    void store(uint64_t addr, uint8_t value) {
        std::lock_guard<std::mutex> lock(mtx);
        
        switch (addr) {
            case 0x00: { // THR: Transmit Holding Register
                // Flush the character directly to the host C++ terminal
                std::cout << static_cast<char>(value);
                std::cout.flush();
                break;
            }
            default:
                // Ignore writes to configuration registers during boot
                break;
        }
    }
};