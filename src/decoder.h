#pragma once
#include <cstdint>
#include <string>

enum class InstFormat {
    R, I, S, B, U, J, UNKNOWN
};

struct DecodedInst {
    uint32_t raw = 0;
    uint32_t opcode = 0;
    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct3 = 0;
    uint8_t funct7 = 0;
    int32_t imm = 0;
    InstFormat format = InstFormat::UNKNOWN;
};

// Sign-extends a `bits`-wide unsigned value (where `bits` is 1..31) into int32_t.
inline int32_t sign_extend(uint32_t value, int bits) {
    uint32_t sign_bit = 1u << (bits - 1);
    uint32_t mask = (bits == 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
    value &= mask;
    if (value & sign_bit) {
        value |= (~0u << bits);
    }
    return static_cast<int32_t>(value);
}

inline InstFormat get_format(uint32_t opcode) {
    switch (opcode) {
        case 0x33: return InstFormat::R; // OP (add, sub, xor, sll, etc.)
        case 0x13:
        case 0x03:
        case 0x67: return InstFormat::I; // OP-IMM, LOAD, JALR
        case 0x23: return InstFormat::S; // STORE (sb, sh, sw)
        case 0x63: return InstFormat::B; // BRANCH (beq, bne, blt, bge, etc.)
        case 0x37:
        case 0x17: return InstFormat::U; // LUI, AUIPC
        case 0x6F: return InstFormat::J; // JAL
        default:   return InstFormat::UNKNOWN;
    }
}

inline DecodedInst decode_rv32i(uint32_t insn) {
    DecodedInst d;
    d.raw    = insn;
    d.opcode = insn & 0x7Fu;
    d.rd     = (insn >> 7) & 0x1Fu;
    d.funct3 = (insn >> 12) & 0x7u;
    d.rs1    = (insn >> 15) & 0x1Fu;
    d.rs2    = (insn >> 20) & 0x1Fu;
    d.funct7 = (insn >> 25) & 0x7Fu;
    d.format = get_format(d.opcode);

    switch (d.format) {
        case InstFormat::R:
            d.imm = 0;
            break;

        case InstFormat::I: {
            // imm[11:0] = insn[31:20] (12 bits)
            uint32_t imm12 = (insn >> 20) & 0xFFFu;
            d.imm = sign_extend(imm12, 12);
            break;
        }

        case InstFormat::S: {
            // imm[11:5] = insn[31:25], imm[4:0] = insn[11:7] (12 bits)
            uint32_t imm11_5 = (insn >> 25) & 0x7Fu;
            uint32_t imm4_0  = (insn >> 7) & 0x1Fu;
            uint32_t imm12   = (imm11_5 << 5) | imm4_0;
            d.imm = sign_extend(imm12, 12);
            break;
        }

        case InstFormat::B: {
            // imm[12]   = insn[31]
            // imm[10:5] = insn[30:25]
            // imm[4:1]  = insn[11:8]
            // imm[11]   = insn[7]
            // imm[0]    = 0  -> 13-bit signed value total
            uint32_t bit12    = (insn >> 31) & 0x1u;
            uint32_t bits10_5 = (insn >> 25) & 0x3Fu;
            uint32_t bits4_1  = (insn >> 8) & 0xFu;
            uint32_t bit11    = (insn >> 7) & 0x1u;
            uint32_t imm13    = (bit12 << 12) |
                                (bit11 << 11) |
                                (bits10_5 << 5) |
                                (bits4_1 << 1);
            d.imm = sign_extend(imm13, 13);
            break;
        }

        case InstFormat::U: {
            // imm[31:12] = insn[31:12], lower 12 bits are zero
            d.imm = static_cast<int32_t>(insn & 0xFFFFF000u);
            break;
        }

        case InstFormat::J: {
            // imm[20]    = insn[31]
            // imm[10:1]  = insn[30:21]
            // imm[11]    = insn[20]
            // imm[19:12] = insn[19:12]
            // imm[0]     = 0 -> 21-bit signed value total
            uint32_t bit20     = (insn >> 31) & 0x1u;
            uint32_t bits10_1  = (insn >> 21) & 0x3FFu;
            uint32_t bit11     = (insn >> 20) & 0x1u;
            uint32_t bits19_12 = (insn >> 12) & 0xFFu;
            uint32_t imm21     = (bit20 << 20) |
                                 (bits19_12 << 12) |
                                 (bit11 << 11) |
                                 (bits10_1 << 1);
            d.imm = sign_extend(imm21, 21);
            break;
        }

        case InstFormat::UNKNOWN:
        default:
            d.imm = 0;
            break;
    }

    return d;
}