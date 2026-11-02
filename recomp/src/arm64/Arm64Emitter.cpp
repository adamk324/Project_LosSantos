#include "recomp/arm64/Arm64Emitter.h"

namespace project_los_santos::recomp::arm64 {

uint32_t Arm64Emitter::emitNop() {
    return 0xD503201Fu;
}

uint32_t Arm64Emitter::emitMovImmediate(uint8_t destinationRegister, uint32_t immediateValue) {
    const uint32_t movBits = 0x52800000u;
    return movBits | (static_cast<uint32_t>(destinationRegister) << 0u) | (immediateValue & 0xFFFFu);
}

EmittedInstruction Arm64Emitter::emitBranchImmediate(uint8_t destinationRegister, uint32_t immediateValue) {
    EmittedInstruction instruction{};
    instruction.mnemonic = "b";
    instruction.encoding = 0x14000000u | (static_cast<uint32_t>(destinationRegister) << 5u) | (immediateValue & 0x7FFFFu);
    return instruction;
}

}  // namespace project_los_santos::recomp::arm64
