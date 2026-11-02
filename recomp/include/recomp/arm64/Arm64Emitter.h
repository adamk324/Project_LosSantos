#pragma once

#include <cstdint>
#include <string>

namespace project_los_santos::recomp::arm64 {

struct EmittedInstruction {
    uint32_t encoding = 0;
    std::string mnemonic;
};

class Arm64Emitter {
public:
    static uint32_t emitNop();
    static uint32_t emitMovImmediate(uint8_t destinationRegister, uint32_t immediateValue);
    static EmittedInstruction emitBranchImmediate(uint8_t destinationRegister, uint32_t immediateValue);
};

}  // namespace project_los_santos::recomp::arm64
