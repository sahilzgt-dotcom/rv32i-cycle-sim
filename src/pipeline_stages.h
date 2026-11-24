#pragma once
#include <cstdint>
#include "pipeline_latches.h"

inline void stage_wb(PipelineState& s) {
    const auto& wb = s.mem_wb;
    if (!wb.valid) return;

    if (wb.reg_write && wb.rd != 0) {
        uint32_t result = wb.mem_to_reg ? wb.mem_read_data : wb.alu_result;
        s.rf.write(wb.rd, result);
    }
}

inline void stage_mem(PipelineState& s) {
    const auto& mem_latch = s.ex_mem;
    auto& next_wb = s.next_mem_wb;

    if (!mem_latch.valid) {
        next_wb.valid = false;
        return;
    }

    next_wb.valid      = true;
    next_wb.pc         = mem_latch.pc;
    next_wb.inst       = mem_latch.inst;
    next_wb.alu_result = mem_latch.alu_result;
    next_wb.rd         = mem_latch.rd;
    next_wb.reg_write  = mem_latch.reg_write;
    next_wb.mem_to_reg = mem_latch.mem_to_reg;

    if (mem_latch.mem_read) {
        // Baseline 32-bit word load; extend with funct3 (LB, LH, LBU, LHU) as needed
        next_wb.mem_read_data = s.mem.read32(mem_latch.alu_result);
    }

    if (mem_latch.mem_write) {
        // Baseline 32-bit word store; extend with funct3 (SB, SH) as needed
        s.mem.write32(mem_latch.alu_result, mem_latch.rs2_val);
    }
}
inline void stage_ex(PipelineState& s) {
    const auto& ex = s.id_ex;
    auto& next_mem = s.next_ex_mem;

    if (!ex.valid) {
        next_mem.valid = false;
        return;
    }

    const auto& d = ex.inst;
    uint32_t op_a = ex.rs1_val;
    uint32_t op_b = ex.alu_src_imm ? static_cast<uint32_t>(d.imm) : ex.rs2_val;

    uint32_t alu_out = 0;
    bool take_branch = false;
    uint32_t target_pc = ex.pc + static_cast<uint32_t>(d.imm);

    switch (d.opcode) {
        case 0x33: { // R-type (OP)
            if (d.funct3 == 0x0 && d.funct7 == 0x00) {
                alu_out = op_a + op_b; // ADD
            } else if (d.funct3 == 0x0 && d.funct7 == 0x20) {
                alu_out = op_a - op_b; // SUB
            } else if (d.funct3 == 0x7) {
                alu_out = op_a & op_b; // AND
            } else if (d.funct3 == 0x6) {
                alu_out = op_a | op_b; // OR
            } else if (d.funct3 == 0x4) {
                alu_out = op_a ^ op_b; // XOR
            }
            break;
        }
        case 0x13: { // I-type arithmetic (OP-IMM)
            if (d.funct3 == 0x0) {
                alu_out = op_a + op_b; // ADDI
            } else if (d.funct3 == 0x7) {
                alu_out = op_a & op_b; // ANDI
            } else if (d.funct3 == 0x6) {
                alu_out = op_a | op_b; // ORI
            } else if (d.funct3 == 0x4) {
                alu_out = op_a ^ op_b; // XORI
            }
            break;
        }
        case 0x03: // LOAD (LW)
        case 0x23: { // STORE (SW)
            alu_out = static_cast<uint32_t>(static_cast<int32_t>(ex.rs1_val) + d.imm);
            break;
        }
        case 0x37: { // LUI
            alu_out = static_cast<uint32_t>(d.imm);
            break;
        }
        case 0x17: { // AUIPC
            alu_out = ex.pc + static_cast<uint32_t>(d.imm);
            break;
        }
        case 0x6F: { // JAL
            alu_out = ex.pc + 4u; // Return address (PC + 4)
            take_branch = true;
            break;
        }
        case 0x63: { // BRANCH (BEQ / BNE)
            if (d.funct3 == 0x0 && ex.rs1_val == ex.rs2_val) {
                take_branch = true; // BEQ
            } else if (d.funct3 == 0x1 && ex.rs1_val != ex.rs2_val) {
                take_branch = true; // BNE
            }
            break;
        }
        default:
            break;
    }

    next_mem.valid         = true;
    next_mem.pc            = ex.pc;
    next_mem.inst          = d;
    next_mem.alu_result    = alu_out;
    next_mem.rs2_val       = ex.rs2_val;
    next_mem.branch_target = target_pc;
    next_mem.branch_taken  = take_branch;
    next_mem.rd            = d.rd;
    next_mem.mem_read      = ex.mem_read;
    next_mem.mem_write     = ex.mem_write;
    next_mem.reg_write     = ex.reg_write;
    next_mem.mem_to_reg    = ex.mem_to_reg;

    if (take_branch) {
        s.next_pc = target_pc;
    }
}
inline void stage_id(PipelineState& s) {
    const auto& if_id = s.if_id;
    auto& next_ex = s.next_id_ex;

    if (!if_id.valid) {
        next_ex.valid = false;
        return;
    }

    DecodedInst d = decode_rv32i(if_id.instr);

    next_ex.valid   = true;
    next_ex.pc      = if_id.pc;
    next_ex.inst    = d;
    next_ex.rs1_val = s.rf.read(d.rs1);
    next_ex.rs2_val = s.rf.read(d.rs2);

    // Default control signals
    next_ex.alu_src_imm = false;
    next_ex.mem_read    = false;
    next_ex.mem_write   = false;
    next_ex.reg_write   = false;
    next_ex.mem_to_reg  = false;
    next_ex.branch      = false;
    next_ex.jump        = false;

    switch (d.opcode) {
        case 0x33: // R-type
            next_ex.reg_write = true;
            break;
        case 0x13: // I-type ALU
            next_ex.alu_src_imm = true;
            next_ex.reg_write   = true;
            break;
        case 0x03: // LOAD
            next_ex.alu_src_imm = true;
            next_ex.mem_read    = true;
            next_ex.reg_write   = true;
            next_ex.mem_to_reg  = true;
            break;
        case 0x23: // STORE
            next_ex.alu_src_imm = true;
            next_ex.mem_write   = true;
            break;
        case 0x37: // LUI
        case 0x17: // AUIPC
            next_ex.alu_src_imm = true;
            next_ex.reg_write   = true;
            break;
        case 0x6F: // JAL
            next_ex.reg_write = true;
            next_ex.jump      = true;
            break;
        case 0x63: // BRANCH
            next_ex.branch = true;
            break;
        default:
            break;
    }
}

inline void stage_if(PipelineState& s) {
    auto& next_if = s.next_if_id;

    // If EX did not redirect PC (e.g., taken branch/jump), advance sequentially
    if (!s.next_ex_mem.branch_taken) {
        s.next_pc = s.pc + 4u;
    }

    if (s.pc + 3u >= s.mem.bytes.size()) {
        next_if.valid = false;
        return;
    }

    next_if.valid = true;
    next_if.pc    = s.pc;
    next_if.instr = s.mem.read32(s.pc);
}
inline void tick(PipelineState& s) {
    s.next_pc = s.pc + 4u;
    stage_wb(s);
    stage_mem(s);
    stage_ex(s);
    stage_id(s);
    stage_if(s);
    commit_clock_edge(s);  
}