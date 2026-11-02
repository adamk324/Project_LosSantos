#include "recomp/powerpc/PowerPCDecoder.h"

#include <sstream>

namespace project_los_santos::recomp::powerpc {

DecodedInstruction decodeInstruction(uint32_t word) {
    DecodedInstruction instruction;
    instruction.raw = word;
    instruction.opcode = static_cast<uint8_t>((word >> 26) & 0x3Fu);
    instruction.primary = static_cast<uint8_t>((word >> 25) & 0x7Fu);
    instruction.secondary = static_cast<uint8_t>(word & 0x7Fu);

    switch (instruction.opcode) {
        case 0x10u: {
            instruction.isBranch = true;
            instruction.isConditionalBranch = true;
            instruction.mnemonic = "bc";
            instruction.target = word & 0x03FF'FFFCu;
            break;
        }
        case 0x12u: {
            instruction.isBranch = true;
            instruction.mnemonic = "b";
            instruction.target = word & 0x03FF'FFFCu;
            break;
        }
        case 0x13u: {
            instruction.isBranch = true;
            instruction.hasLink = true;
            instruction.mnemonic = "bl";
            instruction.target = word & 0x03FF'FFFCu;
            break;
        }
        case 0x20u:
            instruction.mnemonic = "lbz";
            break;
        case 0x21u:
            instruction.mnemonic = "lhz";
            break;
        case 0x23u:
            instruction.mnemonic = "lwz";
            break;
        case 0x2Fu:
            instruction.mnemonic = "lwa";
            break;
        default:
            instruction.mnemonic = "unknown";
            break;
    }

    if (instruction.opcode == 0x1Fu) {
        // PowerPC helper decode for OPD-style instructions.
        switch (instruction.secondary) {
            case 0x00u:
                instruction.mnemonic = "cmp";
                break;
            case 0x13u:
                instruction.mnemonic = "add";
                break;
            default:
                instruction.mnemonic = "opd";
                break;
        }
    }

    return instruction;
}

std::string formatInstruction(const DecodedInstruction& instruction) {
    std::ostringstream stream;
    stream << instruction.mnemonic << " raw=0x" << std::hex << instruction.raw << std::dec;
    if (instruction.isBranch) {
        stream << " target=0x" << std::hex << instruction.target << std::dec;
    }
    return stream.str();
}

}  // namespace project_los_santos::recomp::powerpc
