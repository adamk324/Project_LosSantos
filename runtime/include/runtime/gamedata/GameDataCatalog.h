#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace project_los_santos::runtime::gamedata {

enum class DiscKind {
    Install,
    Play,
};

enum class SourceFormat {
    Auto,
    Directory,
    Xbox360Iso,
};

enum class ResolutionPolicy {
    RejectConflicts,
    SelectSource,
};

std::string toString(DiscKind disc);
std::string toString(SourceFormat format);

struct DiscSource {
    DiscKind disc;
    std::filesystem::path root;
    SourceFormat format = SourceFormat::Auto;
    std::string sourceId;
};

struct FileOrigin {
    DiscKind disc;
    std::string sourceRelativePath;
    std::string sourceId;
    uint64_t byteOffset = 0;
};

struct FileLocation {
    std::filesystem::path containerPath;
    std::string sourceId;
    uint64_t byteOffset = 0;
    uint64_t size = 0;
    SourceFormat format = SourceFormat::Directory;
};

struct ContentVariant {
    uint64_t size = 0;
    std::string sha256;
    bool executableCandidate = false;
    std::string executableFormat;
    std::vector<FileOrigin> origins;
};

struct ManifestEntry {
    std::string virtualPath;
    std::vector<ContentVariant> variants;
};

enum class LookupStatus {
    NotFound,
    Found,
    Conflict,
};

struct LookupResult {
    LookupStatus status = LookupStatus::NotFound;
    const ManifestEntry* entry = nullptr;
};

struct ResourceIssue {
    std::string virtualPath;
    LookupStatus status;
};

class GameDataCatalog {
public:
    static GameDataCatalog scan(const std::vector<DiscSource>& sources);

    [[nodiscard]] LookupResult lookup(std::string_view virtualPath) const;
    [[nodiscard]] FileLocation resolveFile(
        std::string_view virtualPath,
        ResolutionPolicy policy = ResolutionPolicy::RejectConflicts,
        std::string_view sourceId = {}) const;
    [[nodiscard]] std::filesystem::path resolvePath(
        std::string_view virtualPath,
        ResolutionPolicy policy = ResolutionPolicy::RejectConflicts,
        std::string_view sourceId = {}) const;
    [[nodiscard]] std::ifstream openFile(std::string_view virtualPath) const;
    [[nodiscard]] std::unique_ptr<std::istream> openResource(
        std::string_view virtualPath,
        ResolutionPolicy policy = ResolutionPolicy::RejectConflicts,
        std::string_view sourceId = {}) const;
    [[nodiscard]] std::vector<DiscKind> missingDiscs() const;
    [[nodiscard]] std::vector<ResourceIssue> validateRequiredResources(
        const std::vector<std::string>& virtualPaths) const;
    [[nodiscard]] std::vector<std::string> executableCandidates() const;
    [[nodiscard]] const std::vector<ManifestEntry>& entries() const noexcept { return entries_; }
    [[nodiscard]] std::string toJson() const;

private:
    struct SourceInfo {
        DiscKind disc;
        std::filesystem::path path;
        SourceFormat format;
    };

    std::map<std::string, SourceInfo> sources_;
    std::map<DiscKind, std::size_t> sourceFileCounts_;
    std::vector<ManifestEntry> entries_;
    std::map<std::string, std::size_t> entryIndices_;
};

}  // namespace project_los_santos::runtime::gamedata
