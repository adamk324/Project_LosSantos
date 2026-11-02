#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace project_los_santos::runtime::gamedata {

enum class DiscKind {
    Install,
    Play,
};

std::string toString(DiscKind disc);

struct DiscSource {
    DiscKind disc;
    std::filesystem::path root;
};

struct FileOrigin {
    DiscKind disc;
    std::string sourceRelativePath;
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
    [[nodiscard]] std::filesystem::path resolvePath(std::string_view virtualPath) const;
    [[nodiscard]] std::ifstream openFile(std::string_view virtualPath) const;
    [[nodiscard]] std::vector<DiscKind> missingDiscs() const;
    [[nodiscard]] std::vector<ResourceIssue> validateRequiredResources(
        const std::vector<std::string>& virtualPaths) const;
    [[nodiscard]] std::vector<std::string> executableCandidates() const;
    [[nodiscard]] const std::vector<ManifestEntry>& entries() const noexcept { return entries_; }
    [[nodiscard]] std::string toJson() const;

private:
    std::map<DiscKind, std::filesystem::path> sourceRoots_;
    std::map<DiscKind, std::size_t> sourceFileCounts_;
    std::vector<ManifestEntry> entries_;
    std::map<std::string, std::size_t> entryIndices_;
};

}  // namespace project_los_santos::runtime::gamedata
