#include "runtime/gamedata/Xbox360Metadata.h"

#include <array>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace project_los_santos::runtime::gamedata {
namespace {

constexpr std::size_t kMinimumHeaderBytes = 0x3ADu;
constexpr std::size_t kContentIdOffset = 0x32Cu;
constexpr std::size_t kHeaderSizeOffset = 0x340u;
constexpr std::size_t kMetadataOffset = 0x344u;
constexpr std::size_t kTitleIdOffset = 0x360u;
constexpr std::size_t kStfsDescriptorOffset = 0x379u;
constexpr std::size_t kDataFileCountOffset = 0x39Du;
constexpr std::size_t kVolumeTypeOffset = 0x3A9u;
constexpr std::size_t kStfsBlockSize = 0x1000u;
constexpr std::size_t kStfsBlocksPerHashTable = 170u;
constexpr std::size_t kStfsDirectoryEntrySize = 0x40u;
constexpr std::size_t kStfsDirectoryEntriesPerBlock = 64u;
constexpr std::size_t kStfsDirectoryBlockSize =
    kStfsDirectoryEntrySize * kStfsDirectoryEntriesPerBlock;
constexpr uint32_t kStfsVolumeType = 0u;
constexpr uint8_t kStfsDescriptorSize = 0x24u;

uint16_t readLittleEndian16(const uint8_t* bytes) {
    return static_cast<uint16_t>(bytes[0]) |
           (static_cast<uint16_t>(bytes[1]) << 8u);
}

uint32_t readLittleEndian24(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8u) |
           (static_cast<uint32_t>(bytes[2]) << 16u);
}

uint16_t readBigEndian16(const uint8_t* bytes) {
    return static_cast<uint16_t>((static_cast<uint16_t>(bytes[0]) << 8u) |
                                 static_cast<uint16_t>(bytes[1]));
}

uint32_t readBigEndian32(const uint8_t* bytes) {
    return (static_cast<uint32_t>(bytes[0]) << 24u) |
           (static_cast<uint32_t>(bytes[1]) << 16u) |
           (static_cast<uint32_t>(bytes[2]) << 8u) |
           static_cast<uint32_t>(bytes[3]);
}

}  // namespace

StfsPackageMetadata inspectStfsPackageHeader(std::istream& package, uint64_t packageSize) {
    if (packageSize < kMinimumHeaderBytes) {
        throw std::runtime_error("STFS package is shorter than its required metadata header.");
    }

    std::array<uint8_t, kMinimumHeaderBytes> header{};
    if (!package.read(reinterpret_cast<char*>(header.data()),
                      static_cast<std::streamsize>(header.size()))) {
        throw std::runtime_error("Unable to read complete STFS package metadata header.");
    }

    StfsPackageMetadata metadata;
    metadata.magic.assign(reinterpret_cast<const char*>(header.data()), 4u);
    if (metadata.magic != "PIRS" && metadata.magic != "LIVE" && metadata.magic != "CON ") {
        throw std::runtime_error("Unrecognized Xbox content package signature.");
    }

    metadata.headerSize = readBigEndian32(header.data() + kHeaderSizeOffset);
    if (metadata.headerSize < kMinimumHeaderBytes || metadata.headerSize > packageSize) {
        throw std::runtime_error("Xbox content package header size is outside the package.");
    }
    metadata.contentType = readBigEndian32(header.data() + kMetadataOffset);
    metadata.metadataVersion = readBigEndian32(header.data() + kMetadataOffset + 4u);
    metadata.titleId = readBigEndian32(header.data() + kTitleIdOffset);
    metadata.stfsFlags = header[kStfsDescriptorOffset + 2u];
    metadata.fileTableBlockCount =
        readLittleEndian16(header.data() + kStfsDescriptorOffset + 3u);
    metadata.fileTableBlockNumber =
        readLittleEndian24(header.data() + kStfsDescriptorOffset + 5u);
    metadata.totalBlockCount =
        readBigEndian32(header.data() + kStfsDescriptorOffset + 0x1Cu);
    metadata.freeBlockCount =
        readBigEndian32(header.data() + kStfsDescriptorOffset + 0x20u);
    metadata.dataFileCount = readBigEndian32(header.data() + kDataFileCountOffset);
    metadata.volumeType = readBigEndian32(header.data() + kVolumeTypeOffset);

    if (header[kStfsDescriptorOffset] != kStfsDescriptorSize ||
        metadata.volumeType != kStfsVolumeType) {
        throw std::runtime_error("Xbox content package does not contain a valid STFS descriptor.");
    }

    std::ostringstream contentId;
    contentId << std::uppercase << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < 0x14u; ++i) {
        contentId << std::setw(2) << static_cast<unsigned int>(header[kContentIdOffset + i]);
    }
    metadata.contentId = contentId.str();
    return metadata;
}

std::vector<StfsDirectoryEntryMetadata> inspectStfsPackageFileTable(
    std::istream& package, uint64_t packageSize) {
    package.clear();
    package.seekg(0, std::ios::beg);
    if (!package) {
        throw std::runtime_error("Unable to seek to STFS package start.");
    }

    const StfsPackageMetadata metadata = inspectStfsPackageHeader(package, packageSize);
    if (metadata.fileTableBlockCount != 1u) {
        throw std::runtime_error(
            "STFS metadata inspection currently supports exactly one file-table block.");
    }
    if (metadata.totalBlockCount == 0u ||
        metadata.freeBlockCount > metadata.totalBlockCount) {
        throw std::runtime_error("STFS volume block counts are invalid.");
    }
    if (metadata.fileTableBlockNumber >= metadata.totalBlockCount) {
        throw std::runtime_error("STFS file-table block number is outside the volume.");
    }

    const uint64_t alignedHeaderSize =
        (static_cast<uint64_t>(metadata.headerSize) + (kStfsBlockSize - 1u)) &
        ~static_cast<uint64_t>(kStfsBlockSize - 1u);
    uint64_t mappedBlock = metadata.fileTableBlockNumber;
    uint64_t blocksPerLevel = kStfsBlocksPerHashTable;
    const uint64_t hashTableCopies = (metadata.stfsFlags & 1u) != 0u ? 1u : 2u;
    for (std::size_t level = 0; level < 3u; ++level) {
        const uint64_t tablesBeforeBlock =
            (static_cast<uint64_t>(metadata.fileTableBlockNumber) + blocksPerLevel) /
            blocksPerLevel;
        if (tablesBeforeBlock >
            (std::numeric_limits<uint64_t>::max() - mappedBlock) / hashTableCopies) {
            throw std::runtime_error("STFS file-table block mapping overflows.");
        }
        mappedBlock += tablesBeforeBlock * hashTableCopies;
        if (metadata.fileTableBlockNumber < blocksPerLevel) {
            break;
        }
        if (blocksPerLevel >
            std::numeric_limits<uint64_t>::max() / kStfsBlocksPerHashTable) {
            throw std::runtime_error("STFS hash-table mapping overflows.");
        }
        blocksPerLevel *= kStfsBlocksPerHashTable;
    }
    if (mappedBlock >
        (std::numeric_limits<uint64_t>::max() - alignedHeaderSize) / kStfsBlockSize) {
        throw std::runtime_error("STFS file-table byte offset overflows.");
    }
    const uint64_t tableOffset =
        alignedHeaderSize + mappedBlock * kStfsBlockSize;
    if (tableOffset > packageSize ||
        kStfsDirectoryBlockSize > packageSize - tableOffset ||
        tableOffset > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        throw std::runtime_error("STFS file-table block lies outside the package.");
    }

    package.clear();
    package.seekg(static_cast<std::streamoff>(tableOffset), std::ios::beg);
    std::array<uint8_t, kStfsDirectoryBlockSize> table{};
    if (!package || !package.read(reinterpret_cast<char*>(table.data()),
                                  static_cast<std::streamsize>(table.size()))) {
        throw std::runtime_error("Unable to read complete STFS file-table block.");
    }

    std::vector<StfsDirectoryEntryMetadata> entries;
    entries.reserve(kStfsDirectoryEntriesPerBlock);
    for (std::size_t i = 0; i < kStfsDirectoryEntriesPerBlock; ++i) {
        const uint8_t* const entry = table.data() + i * kStfsDirectoryEntrySize;
        if (entry[0] == 0u) {
            break;
        }

        const uint8_t flags = entry[40u];
        const uint8_t nameLength = flags & 0x3Fu;
        if (nameLength == 0u || nameLength > 40u) {
            throw std::runtime_error("STFS file-table entry has an invalid name length.");
        }
        const std::string_view name(reinterpret_cast<const char*>(entry), nameLength);
        if (name.find_first_of("/\\") != std::string_view::npos) {
            throw std::runtime_error("STFS file-table entry contains a path separator.");
        }
        const uint32_t validDataBlocks = readLittleEndian24(entry + 41u);
        const uint32_t allocatedDataBlocks = readLittleEndian24(entry + 44u);
        const uint32_t startBlockNumber = readLittleEndian24(entry + 47u);
        const uint16_t directoryIndex = readBigEndian16(entry + 50u);
        const bool isDirectory = (flags & 0x80u) != 0u;
        if (validDataBlocks > allocatedDataBlocks ||
            allocatedDataBlocks > metadata.totalBlockCount ||
            (directoryIndex != 0xFFFFu &&
             (directoryIndex >= entries.size() ||
              !entries[directoryIndex].isDirectory)) ||
            (allocatedDataBlocks > 0u &&
             startBlockNumber >= metadata.totalBlockCount)) {
            throw std::runtime_error("STFS file-table entry contains invalid block metadata.");
        }

        StfsDirectoryEntryMetadata metadataEntry;
        metadataEntry.name.assign(name);
        metadataEntry.isDirectory = isDirectory;
        metadataEntry.contiguous = (flags & 0x40u) != 0u;
        metadataEntry.validDataBlocks = validDataBlocks;
        metadataEntry.allocatedDataBlocks = allocatedDataBlocks;
        metadataEntry.startBlockNumber = startBlockNumber;
        metadataEntry.directoryIndex = directoryIndex;
        metadataEntry.size = readBigEndian32(entry + 52u);
        entries.push_back(std::move(metadataEntry));
    }
    return entries;
}

}  // namespace project_los_santos::runtime::gamedata
