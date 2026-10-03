Understanding Sign Extension (The LUI Instruction)
When dealing with low-level systems, how you handle negative numbers is critical. This is a common pitfall in emulator development.

The LUI instruction loads a 32-bit value into a 64-bit register.

If the 32-bit value is 0x00001000, putting it in a 64-bit register is easy: 0x0000000000001000.

But what if the 32-bit value represents a negative number, like -2 (which is 0xFFFFFFFE in hex)?

If you just copy 0xFFFFFFFE into a 64-bit register, you get 0x00000000FFFFFFFE. In 64-bit math, that is no longer -2; it's a massive positive number (4,294,967,294).

To preserve the negative value, RISC-V requires sign extension. If the highest bit (the "sign bit") of the 32-bit number is a 1, you must pad the upper 32 bits of the 64-bit register with 1s.

Correct sign extension: 0xFFFFFFFFFFFFFFFE (This remains -2 in 64-bit math).

In C++, casting an unsigned integer (uint32_t) to a larger size simply pads with zeroes. Casting a signed integer (int32_t) to a larger size tells the compiler to automatically perform sign extension. That is why we do static_cast<uint64_t>(static_cast<int32_t>(imm)) in the code above.