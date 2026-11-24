#include <cstdint>
#include <iostream>
#include "pipeline_stages.h"

int main() {
    PipelineState s;

    // Sample RV32I machine instructions:
    // 1. addi x1, x0, 5    -> 0x00500093 (x1 = 5)
    // 2. addi x2, x0, 10   -> 0x00A00113 (x2 = 10)
    // 3. add  x3, x1, x2   -> 0x002081B3 (x3 = x1 + x2)
    s.mem.write32(0x00, 0x00500093u);
    s.mem.write32(0x04, 0x00A00113u);
    s.mem.write32(0x08, 0x002081B3u);

    // Run 10 clock cycles and print state after each tick
    for (int i = 0; i < 10; ++i) {
        tick(s);
        std::cout << "Cycle " << s.cycle
                  << " | PC=" << s.pc
                  << " | x1=" << s.rf.read(1)
                  << " | x2=" << s.rf.read(2)
                  << " | x3=" << s.rf.read(3) << "\n";
    }

    return 0;
}