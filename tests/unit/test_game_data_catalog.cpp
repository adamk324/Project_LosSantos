#include "runtime/gamedata/GameDataCatalog.h"
#include "runtime/gamedata/Xbox360Metadata.h"

#include <algorithm>
#include <cassert>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using project_los_santos::runtime::gamedata::DiscKind;
using project_los_santos::runtime::gamedata::DiscSource;
using project_los_santos::runtime::gamedata::GameDataCatalog;
using project_los_santos::runtime::gamedata::LookupStatus;
using project_los_santos::runtime::gamedata::ResolutionPolicy;
using project_los_santos::runtime::gamedata::SourceFormat;
using project_los_santos::runtime::gamedata::inspectStfsPackageHeader;

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = fs::temp_directory_path() /
                ("project-los-santos-game-data-" + std::to_string(timestamp));
        fs::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    const fs::path& path() const noexcept { return path_; }

private:
    fs::path path_;
};

void writeFile(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file || !file.write(contents.data(), static_cast<std::streamsize>(contents.size()))) {
        throw std::runtime_error("Unable to create synthetic game-data fixture.");
    }
}

void writeLittleEndian16(std::vector<uint8_t>& image, std::size_t offset, uint16_t value) {
    image[offset] = static_cast<uint8_t>(value);
    image[offset + 1u] = static_cast<uint8_t>(value >> 8u);
}

void writeLittleEndian32(std::vector<uint8_t>& image, std::size_t offset, uint32_t value) {
    for (std::size_t i = 0; i < 4u; ++i) {
        image[offset + i] = static_cast<uint8_t>(value >> (i * 8u));
    }
}

void overwriteLittleEndian16(const fs::path& path, std::streamoff offset, uint16_t value) {
    const std::array<char, 2> bytes{
        static_cast<char>(value),
        static_cast<char>(value >> 8u),
    };
    std::fstream image(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!image || !image.seekp(offset) || !image.write(bytes.data(), bytes.size())) {
        throw std::runtime_error("Unable to modify synthetic XDVDFS fixture.");
    }
}

void writeBigEndian16(std::vector<uint8_t>& bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value >> 8u);
    bytes[offset + 1u] = static_cast<uint8_t>(value);
}

void writeBigEndian32(std::vector<uint8_t>& bytes, std::size_t offset, uint32_t value) {
    for (std::size_t i = 0; i < 4u; ++i) {
        bytes[offset + i] = static_cast<uint8_t>(value >> ((3u - i) * 8u));
    }
}

std::vector<uint8_t> makeStfsPackageHeader() {
    std::vector<uint8_t> header(0x3ADu, 0u);
    std::copy_n("PIRS", 4u, header.begin());
    for (std::size_t i = 0; i < 0x14u; ++i) {
        header[0x32Cu + i] = static_cast<uint8_t>(i);
    }
    writeBigEndian32(header, 0x340u, 0x500u);
    writeBigEndian32(header, 0x344u, 2u);
    writeBigEndian32(header, 0x348u, 2u);
    writeBigEndian32(header, 0x360u, 0x545408A7u);
    header[0x379u] = 0x24u;
    writeBigEndian16(header, 0x37Cu, 1u);
    writeBigEndian32(header, 0x39Du, 0u);
    writeBigEndian32(header, 0x3A9u, 0u);
    return header;
}

bool rejectsStfsHeader(const std::vector<uint8_t>& bytes, uint64_t packageSize) {
    const std::string contents(bytes.begin(), bytes.end());
    std::istringstream package(contents, std::ios::in | std::ios::binary);
    try {
        (void)inspectStfsPackageHeader(package, packageSize);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

struct IsoFixtureFile {
    std::string name;
    std::string contents;
};

void writeXdvdfsIso(const fs::path& path, const std::vector<IsoFixtureFile>& files) {
    constexpr std::size_t sectorSize = 2048u;
    constexpr std::size_t descriptorOffset = 32u * sectorSize;
    constexpr std::size_t rootSector = 64u;
    constexpr std::size_t rootSize = 256u;
    constexpr std::size_t imageSectors = 160u;
    constexpr std::array<std::size_t, 3> nodeOffsets{0u, 16u, 32u};
    constexpr std::size_t dataSectorBase = 150u;
    assert(!files.empty() && files.size() <= nodeOffsets.size());

    std::vector<uint8_t> image(imageSectors * sectorSize, 0u);
    constexpr std::array<uint8_t, 20> magic{
        'M', 'I', 'C', 'R', 'O', 'S', 'O', 'F', 'T', '*',
        'X', 'B', 'O', 'X', '*', 'M', 'E', 'D', 'I', 'A',
    };
    std::copy(magic.begin(), magic.end(), image.begin() + descriptorOffset);
    std::copy(magic.begin(), magic.end(), image.begin() + descriptorOffset + 0x7ECu);
    writeLittleEndian32(image, descriptorOffset + 0x14u, static_cast<uint32_t>(rootSector));
    writeLittleEndian32(image, descriptorOffset + 0x18u, static_cast<uint32_t>(rootSize));

    for (std::size_t i = 0; i < files.size(); ++i) {
        assert(!files[i].name.empty() && files[i].name.size() <= 255u);
        const std::size_t entryOffset = nodeOffsets[i] * 4u;
        const std::size_t nextNode = i + 1u < files.size() ? nodeOffsets[i + 1u] : 0u;
        writeLittleEndian16(image, rootSector * sectorSize + entryOffset, 0u);
        writeLittleEndian16(image, rootSector * sectorSize + entryOffset + 2u,
                            static_cast<uint16_t>(nextNode));
        writeLittleEndian32(image, rootSector * sectorSize + entryOffset + 4u,
                            static_cast<uint32_t>(dataSectorBase + i));
        writeLittleEndian32(image, rootSector * sectorSize + entryOffset + 8u,
                            static_cast<uint32_t>(files[i].contents.size()));
        image[rootSector * sectorSize + entryOffset + 12u] = 0u;
        image[rootSector * sectorSize + entryOffset + 13u] =
            static_cast<uint8_t>(files[i].name.size());
        std::copy(files[i].name.begin(), files[i].name.end(),
                  image.begin() + rootSector * sectorSize + entryOffset + 14u);
        std::copy(files[i].contents.begin(), files[i].contents.end(),
                  image.begin() + (dataSectorBase + i) * sectorSize);
    }

    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output || !output.write(reinterpret_cast<const char*>(image.data()),
                                 static_cast<std::streamsize>(image.size()))) {
        throw std::runtime_error("Unable to create synthetic XDVDFS ISO fixture.");
    }
}

std::string readFile(std::ifstream file) {
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::string readResource(std::unique_ptr<std::istream> stream) {
    return std::string(std::istreambuf_iterator<char>(*stream), std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
    TemporaryDirectory temporary;
    const fs::path installRoot = temporary.path() / "install-disc";
    const fs::path playRoot = temporary.path() / "play-disc";
    writeFile(installRoot / "common" / "shared.dat", "same data");
    writeFile(playRoot / "common" / "shared.dat", "same data");
    writeFile(installRoot / "common" / "collision.dat", "install contents");
    writeFile(playRoot / "common" / "collision.dat", "play contents");
    writeFile(installRoot / "large.dat", std::string(1000u, 'a'));
    writeFile(installRoot / "install-only.dat", "install");
    writeFile(playRoot / "play-only.dat", "play");
    writeFile(playRoot / "somewhere" / "candidate.xex", "XEX2 synthetic fixture");

    const GameDataCatalog catalog = GameDataCatalog::scan({
        DiscSource{DiscKind::Install, installRoot},
        DiscSource{DiscKind::Play, playRoot},
    });

    assert(catalog.missingDiscs().empty());
    assert(catalog.entries().size() == 6u);

    const auto shared = catalog.lookup("COMMON\\SHARED.DAT");
    assert(shared.status == LookupStatus::Found);
    assert(shared.entry != nullptr);
    assert(shared.entry->variants.size() == 1u);
    assert(shared.entry->variants.front().origins.size() == 2u);
    assert(shared.entry->variants.front().sha256 ==
           "6d8596e17b628c605d4f4d885f7fced4f69c5caa7c4edd4ec215476c42992698");
    assert(readFile(catalog.openFile("common/shared.dat")) == "same data");
    assert(catalog.lookup("large.dat").entry->variants.front().sha256 ==
           "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");

    const auto collision = catalog.lookup("common/collision.dat");
    assert(collision.status == LookupStatus::Conflict);
    assert(collision.entry != nullptr);
    assert(collision.entry->variants.size() == 2u);
    assert(collision.entry->variants[0].origins.front().disc !=
           collision.entry->variants[1].origins.front().disc);
    bool conflictRefused = false;
    try {
        (void)catalog.resolvePath("common/collision.dat");
    } catch (const std::runtime_error&) {
        conflictRefused = true;
    }
    assert(conflictRefused);

    assert(catalog.lookup("install-only.dat").status == LookupStatus::Found);
    assert(catalog.lookup("play-only.dat").status == LookupStatus::Found);
    const auto candidates = catalog.executableCandidates();
    assert(candidates.size() == 1u);
    assert(candidates.front() == "somewhere/candidate.xex");
    const auto executable = catalog.lookup(candidates.front());
    assert(executable.entry->variants.front().executableFormat == "xex2");

    const auto issues = catalog.validateRequiredResources({
        "common/shared.dat",
        "common/collision.dat",
        "missing/resource.dat",
    });
    assert(issues.size() == 2u);
    assert(issues[0].status == LookupStatus::Conflict);
    assert(issues[1].status == LookupStatus::NotFound);

    const std::string manifest = catalog.toJson();
    assert(manifest.find("\"status\": \"conflict\"") != std::string::npos);
    assert(manifest.find("\"disc\": \"install\"") != std::string::npos);
    assert(manifest.find("\"disc\": \"play\"") != std::string::npos);
    assert(manifest.find("\"executable_candidate\": true") != std::string::npos);
    assert(readFile(std::ifstream(installRoot / "common" / "shared.dat", std::ios::binary)) == "same data");

    const GameDataCatalog installOnly = GameDataCatalog::scan({
        DiscSource{DiscKind::Install, installRoot},
    });
    assert(installOnly.missingDiscs() == std::vector<DiscKind>{DiscKind::Play});

    const GameDataCatalog playOnly = GameDataCatalog::scan({
        DiscSource{DiscKind::Play, playRoot},
    });
    assert(playOnly.missingDiscs() == std::vector<DiscKind>{DiscKind::Install});
    assert(installOnly.lookup("play-only.dat").status == LookupStatus::NotFound);
    assert(playOnly.lookup("install-only.dat").status == LookupStatus::NotFound);

    const fs::path emptyRoot = temporary.path() / "empty-disc";
    fs::create_directories(emptyRoot);
    const GameDataCatalog emptySource = GameDataCatalog::scan({
        DiscSource{DiscKind::Install, emptyRoot},
    });
    assert(emptySource.missingDiscs() ==
           (std::vector<DiscKind>{DiscKind::Install, DiscKind::Play}));

    bool traversalRejected = false;
    try {
        (void)catalog.lookup("../outside.dat");
    } catch (const std::invalid_argument&) {
        traversalRejected = true;
    }
    assert(traversalRejected);
    bool drivePathRejected = false;
    try {
        (void)catalog.lookup("C:\\outside.dat");
    } catch (const std::invalid_argument&) {
        drivePathRejected = true;
    }
    assert(drivePathRejected);

    bool sharedRootRejected = false;
    try {
        (void)GameDataCatalog::scan({
            DiscSource{DiscKind::Install, installRoot},
            DiscSource{DiscKind::Play, installRoot},
        });
    } catch (const std::invalid_argument&) {
        sharedRootRejected = true;
    }
    assert(sharedRootRejected);

    const fs::path installIso = temporary.path() / "install.iso";
    const fs::path playIso = temporary.path() / "play.iso";
    writeXdvdfsIso(installIso, {
        {"collision.dat", "install contents"},
        {"identical.dat", "same contents"},
        {"install-only.dat", "install only"},
    });
    writeXdvdfsIso(playIso, {
        {"collision.dat", "play contents"},
        {"identical.dat", "same contents"},
        {"play-only.dat", "play only"},
    });
    const GameDataCatalog isoCatalog = GameDataCatalog::scan({
        DiscSource{DiscKind::Install, installIso, SourceFormat::Xbox360Iso, "install-iso"},
        DiscSource{DiscKind::Play, playIso, SourceFormat::Xbox360Iso, "play-iso"},
    });
    assert(isoCatalog.missingDiscs().empty());
    assert(isoCatalog.lookup("identical.dat").status == LookupStatus::Found);
    assert(isoCatalog.lookup("identical.dat").entry->variants.front().origins.size() == 2u);
    assert(isoCatalog.lookup("install-only.dat").status == LookupStatus::Found);
    assert(isoCatalog.lookup("play-only.dat").status == LookupStatus::Found);
    assert(isoCatalog.lookup("collision.dat").status == LookupStatus::Conflict);
    assert(readResource(isoCatalog.openResource("identical.dat")) == "same contents");
    bool isoConflictRefused = false;
    try {
        (void)isoCatalog.openResource("collision.dat");
    } catch (const std::runtime_error&) {
        isoConflictRefused = true;
    }
    assert(isoConflictRefused);
    assert(readResource(isoCatalog.openResource("collision.dat",
                                                ResolutionPolicy::SelectSource, "install-iso")) ==
           "install contents");
    assert(readResource(isoCatalog.openResource("collision.dat",
                                                ResolutionPolicy::SelectSource, "play-iso")) ==
           "play contents");
    const auto selectedLocation = isoCatalog.resolveFile(
        "collision.dat", ResolutionPolicy::SelectSource, "play-iso");
    assert(selectedLocation.sourceId == "play-iso");
    assert(selectedLocation.format == SourceFormat::Xbox360Iso);
    assert(isoCatalog.validateRequiredResources({"missing.dat"}).front().status ==
           LookupStatus::NotFound);
    assert(isoCatalog.toJson().find("\"format\": \"xbox360-xdvdfs-iso\"") !=
           std::string::npos);

    const fs::path invalidIso = temporary.path() / "invalid.iso";
    writeFile(invalidIso, "not an ISO");
    bool invalidIsoRejected = false;
    try {
        (void)GameDataCatalog::scan({
            DiscSource{DiscKind::Install, invalidIso, SourceFormat::Xbox360Iso},
        });
    } catch (const std::runtime_error&) {
        invalidIsoRejected = true;
    }
    assert(invalidIsoRejected);

    const fs::path cyclicIso = temporary.path() / "cyclic.iso";
    writeXdvdfsIso(cyclicIso, {{"a.dat", "a"}, {"b.dat", "b"}});
    overwriteLittleEndian16(cyclicIso, 64 * 2048 + 64 + 2, 16u);
    bool cyclicTreeRejected = false;
    try {
        (void)GameDataCatalog::scan({
            DiscSource{DiscKind::Install, cyclicIso, SourceFormat::Xbox360Iso},
        });
    } catch (const std::runtime_error&) {
        cyclicTreeRejected = true;
    }
    assert(cyclicTreeRejected);

    const fs::path outOfBoundsIso = temporary.path() / "out-of-bounds.iso";
    writeXdvdfsIso(outOfBoundsIso, {{"a.dat", "a"}, {"b.dat", "b"}});
    overwriteLittleEndian16(outOfBoundsIso, 64 * 2048 + 2, 64u);
    bool invalidNodeRejected = false;
    try {
        (void)GameDataCatalog::scan({
            DiscSource{DiscKind::Install, outOfBoundsIso, SourceFormat::Xbox360Iso},
        });
    } catch (const std::runtime_error&) {
        invalidNodeRejected = true;
    }
    assert(invalidNodeRejected);

    std::vector<uint8_t> packageHeader = makeStfsPackageHeader();
    const std::string packageBytes(packageHeader.begin(), packageHeader.end());
    std::istringstream package(packageBytes, std::ios::in | std::ios::binary);
    const auto packageMetadata = inspectStfsPackageHeader(package, 0x500u);
    assert(packageMetadata.magic == "PIRS");
    assert(packageMetadata.contentId == "000102030405060708090A0B0C0D0E0F10111213");
    assert(packageMetadata.headerSize == 0x500u);
    assert(packageMetadata.titleId == 0x545408A7u);
    assert(packageMetadata.fileTableBlockCount == 1u);
    assert(packageMetadata.volumeType == 0u);

    std::vector<uint8_t> packageInIso = packageHeader;
    packageInIso.resize(0x500u, 0u);
    const std::string packageInIsoContents(packageInIso.begin(), packageInIso.end());
    const fs::path packageIso = temporary.path() / "package.iso";
    writeXdvdfsIso(packageIso, {{"content.pirs", packageInIsoContents}});
    const GameDataCatalog packageCatalog = GameDataCatalog::scan({
        DiscSource{DiscKind::Install, packageIso, SourceFormat::Xbox360Iso},
    });
    const auto packageLocation = packageCatalog.resolveFile("content.pirs");
    auto packageStream = packageCatalog.openResource("content.pirs");
    const auto packageFromIso = inspectStfsPackageHeader(*packageStream, packageLocation.size);
    assert(packageFromIso.contentId == packageMetadata.contentId);
    assert(packageFromIso.titleId == packageMetadata.titleId);

    auto invalidPackageHeader = packageHeader;
    invalidPackageHeader[0] = 'B';
    assert(rejectsStfsHeader(invalidPackageHeader, 0x500u));
    invalidPackageHeader = packageHeader;
    invalidPackageHeader[0x379u] = 0x23u;
    assert(rejectsStfsHeader(invalidPackageHeader, 0x500u));
    invalidPackageHeader = packageHeader;
    writeBigEndian32(invalidPackageHeader, 0x340u, 0x600u);
    assert(rejectsStfsHeader(invalidPackageHeader, 0x500u));
    assert(rejectsStfsHeader(packageHeader, 0x300u));
    assert(rejectsStfsHeader(std::vector<uint8_t>(16u, 0u), 0x500u));

    std::cout << "Two-disc game-data catalog synthetic validation passed.\n";
    return 0;
}
