#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace project_los_santos::runtime::gamedata {

struct StfsPackageMetadata {
    std::string magic;
    std::string contentId;
    uint32_t headerSize = 0;
    uint32_t contentType = 0;
    uint32_t metadataVersion = 0;
    uint32_t titleId = 0;
    uint8_t stfsFlags = 0;
    uint16_t fileTableBlockCount = 0;
    uint32_t fileTableBlockNumber = 0;
    uint32_t totalBlockCount = 0;
    uint32_t freeBlockCount = 0;
    uint32_t dataFileCount = 0;
    uint32_t volumeType = 0;
};

struct StfsDirectoryEntryMetadata {
    std::string name;
    bool isDirectory = false;
    bool contiguous = false;
    uint32_t validDataBlocks = 0;
    uint32_t allocatedDataBlocks = 0;
    uint32_t startBlockNumber = 0;
    uint16_t directoryIndex = 0;
    uint32_t size = 0;
};

StfsPackageMetadata inspectStfsPackageHeader(std::istream& package, uint64_t packageSize);
std::vector<StfsDirectoryEntryMetadata> inspectStfsPackageFileTable(
    std::istream& package, uint64_t packageSize);

}  // namespace project_los_santos::runtime::gamedata
