#pragma once

#include <cstdint>
#include <string>

namespace project_los_santos::recomp::powerpc {

struct DecodedInstruction {
    uint32_t raw = 0;
    uint8_t opcode = 0;
    uint8_t primary = 0;
    uint8_t secondary = 0;
    bool isBranch = false;
    bool isConditionalBranch = false;
    bool hasLink = false;
    uint32_t target = 0;
    std::string mnemonic = "unknown";
};

DecodedInstruction decodeInstruction(uint32_t word);
std::string formatInstruction(const DecodedInstruction& instruction);

}  // namespace project_los_santos::recomp::powerpc
