#include "runtime/gamedata/Xbox360Metadata.h"

#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>

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
constexpr uint32_t kStfsVolumeType = 0u;
constexpr uint8_t kStfsDescriptorSize = 0x24u;

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
    metadata.fileTableBlockCount =
        readBigEndian16(header.data() + kStfsDescriptorOffset + 3u);
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

}  // namespace project_los_santos::runtime::gamedata
