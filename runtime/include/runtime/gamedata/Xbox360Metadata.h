#pragma once

#include <cstdint>
#include <istream>
#include <string>

namespace project_los_santos::runtime::gamedata {

struct StfsPackageMetadata {
    std::string magic;
    std::string contentId;
    uint32_t headerSize = 0;
    uint32_t contentType = 0;
    uint32_t metadataVersion = 0;
    uint32_t titleId = 0;
    uint16_t fileTableBlockCount = 0;
    uint32_t dataFileCount = 0;
    uint32_t volumeType = 0;
};

StfsPackageMetadata inspectStfsPackageHeader(std::istream& package, uint64_t packageSize);

}  // namespace project_los_santos::runtime::gamedata
