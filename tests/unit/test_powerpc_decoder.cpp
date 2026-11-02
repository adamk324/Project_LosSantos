#include "recomp/powerpc/PowerPCDecoder.h"

#include <cassert>
#include <iostream>

using project_los_santos::recomp::powerpc::DecodedInstruction;
using project_los_santos::recomp::powerpc::decodeInstruction;

int main() {
    const uint32_t unconditionalBranch = 0x48000010u;
    const uint32_t conditionalBranch = 0x41800004u;
    const uint32_t loadWord = 0x8C010020u;

    const DecodedInstruction unconditional = decodeInstruction(unconditionalBranch);
    assert(unconditional.isBranch);
    assert(unconditional.mnemonic == "b");

    const DecodedInstruction conditional = decodeInstruction(conditionalBranch);
    assert(conditional.isBranch);
    assert(conditional.isConditionalBranch);
    assert(conditional.mnemonic == "bc");

    const DecodedInstruction loadedWord = decodeInstruction(loadWord);
    assert(loadedWord.mnemonic == "lwz");

    std::cout << "PowerPC decoder synthetic validation passed.\n";
    return 0;
}
