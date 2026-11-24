#pragma once
#include <cstdint>
#include "decoder.h"
#include "cpu_sates.h"

struct IF_ID_Latch {
    bool valid = false;
    uint32_t pc = 0;
    uint32_t instr = 0;
};

struct ID_EX_Latch {
    bool valid = false;
    uint32_t pc = 0;
    DecodedInst inst{};

    // Register values read during ID stage
    uint32_t rs1_val = 0;
    uint32_t rs2_val = 0;

    // Control signals decoded in ID (helpful as you wire EX/MEM/WB)
    bool alu_src_imm = false; // true -> use inst.imm, false -> use rs2_val
    bool mem_read = false;
    bool mem_write = false;
    bool reg_write = false;
    bool mem_to_reg = false;  // true -> WB takes memory data, false -> ALU result
    bool branch = false;
    bool jump = false;
};

struct EX_MEM_Latch {
    bool valid = false;
    uint32_t pc = 0;
    DecodedInst inst{};

    uint32_t alu_result = 0;  // Computed ALU output or target/effective address
    uint32_t rs2_val = 0;     // Store data (for SW/SH/SB)
    uint32_t branch_target = 0;
    bool branch_taken = false;

    uint8_t rd = 0;
    bool mem_read = false;
    bool mem_write = false;
    bool reg_write = false;
    bool mem_to_reg = false;
};

struct MEM_WB_Latch {
    bool valid = false;
    uint32_t pc = 0;
    DecodedInst inst{};

    uint32_t alu_result = 0;
    uint32_t mem_read_data = 0;
    uint8_t rd = 0;
    bool reg_write = false;
    bool mem_to_reg = false;
};

struct PipelineState {
    uint64_t cycle = 0;
    uint32_t pc = 0;
    RegisterFile rf{};
    Memory mem{65536}; // 64 KB default memory

    // Current cycle latches (inputs to combinational stage logic)
    IF_ID_Latch  if_id{};
    ID_EX_Latch  id_ex{};
    EX_MEM_Latch ex_mem{};
    MEM_WB_Latch mem_wb{};

    // Next cycle latches (computed during the current tick)
    IF_ID_Latch  next_if_id{};
    ID_EX_Latch  next_id_ex{};
    EX_MEM_Latch next_ex_mem{};
    MEM_WB_Latch next_mem_wb{};
    uint32_t     next_pc = 0;
};
inline void commit_clock_edge(PipelineState& s) {
    s.pc     = s.next_pc;
    s.if_id  = s.next_if_id;
    s.id_ex  = s.next_id_ex;
    s.ex_mem = s.next_ex_mem;
    s.mem_wb = s.next_mem_wb;
    s.cycle += 1;

    // Reset next latches for the upcoming cycle
    s.next_if_id  = IF_ID_Latch{};
    s.next_id_ex  = ID_EX_Latch{};
    s.next_ex_mem = EX_MEM_Latch{};
    s.next_mem_wb = MEM_WB_Latch{};
}