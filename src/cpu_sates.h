#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "decoder.h" // Reuse DecodedInst from your decoder

struct RegisterFile {
    std::array<uint32_t, 32> regs{};

    uint32_t read(uint8_t idx) const {
        if (idx == 0) return 0;
        return regs.at(idx);
    }

    void write(uint8_t idx, uint32_t val) {
        if (idx == 0) return; // x0 is hardwired to zero
        regs.at(idx) = val;
    }
};

struct Memory {
    std::vector<uint8_t> bytes;

    explicit Memory(size_t size_in_bytes) : bytes(size_in_bytes, 0) {}

    uint8_t read8(uint32_t addr) const {
        if (addr >= bytes.size()) {
            throw std::out_of_range("Memory read8 out of bounds");
        }
        return bytes[addr];
    }

    void write8(uint32_t addr, uint8_t val) {
        if (addr >= bytes.size()) {
            throw std::out_of_range("Memory write8 out of bounds");
        }
        bytes[addr] = val;
    }

    // Little-endian 32-bit word access (for instruction fetch and LW/SW)
    uint32_t read32(uint32_t addr) const {
        return static_cast<uint32_t>(read8(addr)) |
               (static_cast<uint32_t>(read8(addr + 1)) << 8) |
               (static_cast<uint32_t>(read8(addr + 2)) << 16) |
               (static_cast<uint32_t>(read8(addr + 3)) << 24);
    }

    void write32(uint32_t addr, uint32_t val) {
        write8(addr,     static_cast<uint8_t>(val & 0xFFu));
        write8(addr + 1, static_cast<uint8_t>((val >> 8) & 0xFFu));
        write8(addr + 2, static_cast<uint8_t>((val >> 16) & 0xFFu));
        write8(addr + 3, static_cast<uint8_t>((val >> 24) & 0xFFu));
    }
};