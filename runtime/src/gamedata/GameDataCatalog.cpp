#include "runtime/gamedata/GameDataCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace project_los_santos::runtime::gamedata {
namespace fs = std::filesystem;

namespace {

constexpr std::array<uint32_t, 64> kSha256RoundConstants{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
    0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
    0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
    0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
    0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

uint32_t rotateRight(uint32_t value, uint32_t bits) {
    return (value >> bits) | (value << (32u - bits));
}

class Sha256 {
public:
    void update(const uint8_t* data, std::size_t size) {
        totalBytes_ += size;
        while (size > 0u) {
            const std::size_t copied = std::min(size, buffer_.size() - bufferedBytes_);
            std::copy_n(data, copied, buffer_.data() + bufferedBytes_);
            bufferedBytes_ += copied;
            data += copied;
            size -= copied;
            if (bufferedBytes_ == buffer_.size()) {
                transform(buffer_.data());
                bufferedBytes_ = 0u;
            }
        }
    }

    std::string finish() {
        const uint64_t bitLength = static_cast<uint64_t>(totalBytes_) * 8u;
        buffer_[bufferedBytes_++] = 0x80u;
        if (bufferedBytes_ > 56u) {
            std::fill(buffer_.begin() + static_cast<std::ptrdiff_t>(bufferedBytes_), buffer_.end(), 0u);
            transform(buffer_.data());
            bufferedBytes_ = 0u;
        }
        std::fill(buffer_.begin() + static_cast<std::ptrdiff_t>(bufferedBytes_), buffer_.begin() + 56, 0u);
        for (std::size_t i = 0; i < 8u; ++i) {
            buffer_[63u - i] = static_cast<uint8_t>(bitLength >> (i * 8u));
        }
        transform(buffer_.data());

        std::ostringstream result;
        result << std::hex << std::setfill('0');
        for (const uint32_t word : state_) {
            result << std::setw(8) << word;
        }
        return result.str();
    }

private:
    void transform(const uint8_t* block) {
        std::array<uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16u; ++i) {
            const std::size_t offset = i * 4u;
            words[i] = (static_cast<uint32_t>(block[offset]) << 24u) |
                       (static_cast<uint32_t>(block[offset + 1u]) << 16u) |
                       (static_cast<uint32_t>(block[offset + 2u]) << 8u) |
                       static_cast<uint32_t>(block[offset + 3u]);
        }
        for (std::size_t i = 16u; i < words.size(); ++i) {
            const uint32_t s0 = rotateRight(words[i - 15u], 7u) ^
                                rotateRight(words[i - 15u], 18u) ^ (words[i - 15u] >> 3u);
            const uint32_t s1 = rotateRight(words[i - 2u], 17u) ^
                                rotateRight(words[i - 2u], 19u) ^ (words[i - 2u] >> 10u);
            words[i] = words[i - 16u] + s0 + words[i - 7u] + s1;
        }

        uint32_t a = state_[0];
        uint32_t b = state_[1];
        uint32_t c = state_[2];
        uint32_t d = state_[3];
        uint32_t e = state_[4];
        uint32_t f = state_[5];
        uint32_t g = state_[6];
        uint32_t h = state_[7];
        for (std::size_t i = 0; i < words.size(); ++i) {
            const uint32_t sum1 = rotateRight(e, 6u) ^ rotateRight(e, 11u) ^ rotateRight(e, 25u);
            const uint32_t choice = (e & f) ^ (~e & g);
            const uint32_t temp1 = h + sum1 + choice + kSha256RoundConstants[i] + words[i];
            const uint32_t sum0 = rotateRight(a, 2u) ^ rotateRight(a, 13u) ^ rotateRight(a, 22u);
            const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t temp2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::array<uint32_t, 8> state_{
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    std::array<uint8_t, 64> buffer_{};
    std::size_t bufferedBytes_ = 0u;
    std::size_t totalBytes_ = 0u;
};

std::string hashFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to read game-data file: " + path.string());
    }

    Sha256 hash;
    std::array<uint8_t, 64u * 1024u> buffer{};
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            hash.update(buffer.data(), static_cast<std::size_t>(bytesRead));
        }
    }
    if (!file.eof()) {
        throw std::runtime_error("Failed while reading game-data file: " + path.string());
    }
    return hash.finish();
}

bool sameContents(const fs::path& leftPath, const fs::path& rightPath) {
    std::ifstream left(leftPath, std::ios::binary);
    std::ifstream right(rightPath, std::ios::binary);
    if (!left || !right) {
        throw std::runtime_error("Unable to compare game-data files.");
    }

    std::array<char, 64u * 1024u> leftBuffer{};
    std::array<char, 64u * 1024u> rightBuffer{};
    while (left && right) {
        left.read(leftBuffer.data(), static_cast<std::streamsize>(leftBuffer.size()));
        right.read(rightBuffer.data(), static_cast<std::streamsize>(rightBuffer.size()));
        const std::streamsize leftCount = left.gcount();
        const std::streamsize rightCount = right.gcount();
        if (leftCount != rightCount ||
            !std::equal(leftBuffer.begin(), leftBuffer.begin() + leftCount, rightBuffer.begin())) {
            return false;
        }
    }
    if ((!left.eof() && left.fail()) || (!right.eof() && right.fail())) {
        throw std::runtime_error("Failed while comparing game-data files.");
    }
    return left.eof() && right.eof();
}

std::string normalizeVirtualPath(std::string_view path) {
    std::string slashes(path);
    std::replace(slashes.begin(), slashes.end(), '\\', '/');
    const fs::path parsed(slashes);
    const bool hasDrivePrefix = slashes.size() >= 2u &&
                                std::isalpha(static_cast<unsigned char>(slashes[0])) != 0 &&
                                slashes[1] == ':';
    if (parsed.empty() || parsed.is_absolute() || parsed.has_root_name() || hasDrivePrefix) {
        throw std::invalid_argument("Virtual game-data paths must be non-empty relative paths.");
    }

    fs::path normalized;
    for (const fs::path& component : parsed) {
        if (component == "." || component.empty()) {
            continue;
        }
        if (component == "..") {
            throw std::invalid_argument("Virtual game-data paths cannot escape their source root.");
        }
        normalized /= component;
    }
    if (normalized.empty()) {
        throw std::invalid_argument("Virtual game-data paths must name a file.");
    }

    std::string key = normalized.generic_string();
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return key;
}

std::string jsonEscape(std::string_view value) {
    std::string escaped;
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (ch < 0x20u) {
                    constexpr char hex[] = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped += hex[ch >> 4u];
                    escaped += hex[ch & 0x0fu];
                } else {
                    escaped += static_cast<char>(ch);
                }
                break;
        }
    }
    return escaped;
}

std::string executableFormat(const fs::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (extension == ".xex") {
        return "xex-extension-unverified";
    }
    return {};
}

bool hasXex2Magic(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to inspect executable candidate: " + path.string());
    }
    std::array<char, 4> magic{};
    file.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (file.bad()) {
        throw std::runtime_error("Failed while inspecting executable candidate: " + path.string());
    }
    return file.gcount() == static_cast<std::streamsize>(magic.size()) &&
           magic == std::array<char, 4>{'X', 'E', 'X', '2'};
}

std::string lookupStatusName(LookupStatus status) {
    switch (status) {
        case LookupStatus::Found:
            return "found";
        case LookupStatus::Conflict:
            return "conflict";
        case LookupStatus::NotFound:
            return "not-found";
    }
    throw std::logic_error("Unknown game-data lookup status.");
}

}  // namespace

std::string toString(DiscKind disc) {
    switch (disc) {
        case DiscKind::Install:
            return "install";
        case DiscKind::Play:
            return "play";
    }
    throw std::logic_error("Unknown game disc kind.");
}

GameDataCatalog GameDataCatalog::scan(const std::vector<DiscSource>& sources) {
    GameDataCatalog catalog;
    for (const DiscSource& source : sources) {
        if (catalog.sourceRoots_.contains(source.disc)) {
            throw std::invalid_argument("Only one source root may be configured for each game disc.");
        }
        if (source.root.empty()) {
            throw std::invalid_argument("Game-data source roots must not be empty.");
        }

        std::error_code error;
        const fs::path root = fs::absolute(source.root, error).lexically_normal();
        if (error) {
            throw fs::filesystem_error("Unable to resolve game-data source path", source.root, error);
        }
        if (!fs::is_directory(root, error) || error) {
            throw fs::filesystem_error("Game-data source is not an accessible directory", root, error);
        }
        for (const auto& [existingDisc, existingRoot] : catalog.sourceRoots_) {
            (void)existingDisc;
            const bool sameRoot = fs::equivalent(existingRoot, root, error);
            if (error) {
                throw fs::filesystem_error("Unable to compare game-data source roots",
                                           existingRoot, root, error);
            }
            if (sameRoot) {
                throw std::invalid_argument("Install Disc and Play Disc must use separate source roots.");
            }
        }
        catalog.sourceRoots_.emplace(source.disc, root);

        fs::recursive_directory_iterator iterator(root, fs::directory_options::none, error);
        const fs::recursive_directory_iterator end;
        if (error) {
            throw fs::filesystem_error("Unable to inspect game-data source", root, error);
        }
        while (iterator != end) {
            const fs::directory_entry item = *iterator;
            const fs::path relativePath = item.path().lexically_relative(root);
            const std::string virtualPath = relativePath.generic_string();
            const std::string key = normalizeVirtualPath(virtualPath);

            const bool isSymlink = item.is_symlink(error);
            if (error) {
                throw fs::filesystem_error("Unable to inspect game-data entry", item.path(), error);
            }
            if (isSymlink) {
                iterator.disable_recursion_pending();
            } else {
                const bool regularFile = item.is_regular_file(error);
                if (error) {
                    throw fs::filesystem_error("Unable to inspect game-data entry", item.path(), error);
                }
                if (regularFile) {
                    const uint64_t size = item.file_size(error);
                    if (error) {
                        throw fs::filesystem_error("Unable to get game-data file size", item.path(), error);
                    }
                    std::string digest = hashFile(item.path());
                    std::string format = executableFormat(item.path());
                    if (hasXex2Magic(item.path())) {
                        format = "xex2";
                    }
                    const bool executable = !format.empty();
                    ++catalog.sourceFileCounts_[source.disc];

                    std::size_t entryIndex;
                    const auto found = catalog.entryIndices_.find(key);
                    if (found == catalog.entryIndices_.end()) {
                        entryIndex = catalog.entries_.size();
                        catalog.entryIndices_.emplace(key, entryIndex);
                        catalog.entries_.push_back(ManifestEntry{virtualPath, {}});
                    } else {
                        entryIndex = found->second;
                    }

                    ManifestEntry& entry = catalog.entries_[entryIndex];
                    auto variant = std::find_if(entry.variants.begin(), entry.variants.end(),
                                                [&](const ContentVariant& candidate) {
                        if (candidate.size != size || candidate.sha256 != digest) {
                            return false;
                        }
                        const FileOrigin& origin = candidate.origins.front();
                        const fs::path existing = catalog.sourceRoots_.at(origin.disc) /
                                                  fs::path(origin.sourceRelativePath);
                        return sameContents(existing, item.path());
                    });
                    if (variant == entry.variants.end()) {
                        ContentVariant content;
                        content.size = size;
                        content.sha256 = std::move(digest);
                        content.executableCandidate = executable;
                        content.executableFormat = std::move(format);
                        content.origins.push_back(FileOrigin{source.disc, virtualPath});
                        entry.variants.push_back(std::move(content));
                    } else {
                        variant->origins.push_back(FileOrigin{source.disc, virtualPath});
                        if (format == "xex2" ||
                            (variant->executableFormat.empty() && !format.empty())) {
                            variant->executableFormat = std::move(format);
                        }
                        variant->executableCandidate = variant->executableCandidate || executable;
                    }
                }
            }

            iterator.increment(error);
            if (error) {
                throw fs::filesystem_error("Failed while scanning game-data source", root, error);
            }
        }
    }

    for (ManifestEntry& entry : catalog.entries_) {
        for (const ContentVariant& variant : entry.variants) {
            for (const FileOrigin& origin : variant.origins) {
                if (origin.sourceRelativePath < entry.virtualPath) {
                    entry.virtualPath = origin.sourceRelativePath;
                }
            }
        }
    }

    std::sort(catalog.entries_.begin(), catalog.entries_.end(),
              [](const ManifestEntry& left, const ManifestEntry& right) {
                  return normalizeVirtualPath(left.virtualPath) < normalizeVirtualPath(right.virtualPath);
              });
    catalog.entryIndices_.clear();
    for (std::size_t i = 0; i < catalog.entries_.size(); ++i) {
        catalog.entryIndices_.emplace(normalizeVirtualPath(catalog.entries_[i].virtualPath), i);
        auto& variants = catalog.entries_[i].variants;
        std::sort(variants.begin(), variants.end(), [](const ContentVariant& left, const ContentVariant& right) {
            if (left.sha256 != right.sha256) {
                return left.sha256 < right.sha256;
            }
            return left.executableFormat < right.executableFormat;
        });
        for (ContentVariant& variant : variants) {
            std::sort(variant.origins.begin(), variant.origins.end(), [](const FileOrigin& left, const FileOrigin& right) {
                if (left.disc != right.disc) {
                    return left.disc < right.disc;
                }
                return left.sourceRelativePath < right.sourceRelativePath;
            });
        }
    }
    return catalog;
}

LookupResult GameDataCatalog::lookup(std::string_view virtualPath) const {
    const std::string key = normalizeVirtualPath(virtualPath);
    const auto found = entryIndices_.find(key);
    if (found == entryIndices_.end()) {
        return {};
    }
    const ManifestEntry& entry = entries_[found->second];
    return LookupResult{entry.variants.size() > 1u ? LookupStatus::Conflict : LookupStatus::Found, &entry};
}

fs::path GameDataCatalog::resolvePath(std::string_view virtualPath) const {
    const LookupResult result = lookup(virtualPath);
    if (result.status == LookupStatus::NotFound) {
        return {};
    }
    if (result.status == LookupStatus::Conflict) {
        throw std::runtime_error("Ambiguous game-data path has different contents: " + std::string(virtualPath));
    }
    const ContentVariant& variant = result.entry->variants.front();
    const FileOrigin& origin = variant.origins.front();
    return sourceRoots_.at(origin.disc) / fs::path(origin.sourceRelativePath);
}

std::ifstream GameDataCatalog::openFile(std::string_view virtualPath) const {
    const fs::path resolved = resolvePath(virtualPath);
    if (resolved.empty()) {
        throw std::runtime_error("Game-data resource was not found: " + std::string(virtualPath));
    }
    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open game-data resource: " + resolved.string());
    }
    return file;
}

std::vector<DiscKind> GameDataCatalog::missingDiscs() const {
    std::vector<DiscKind> missing;
    for (const DiscKind disc : {DiscKind::Install, DiscKind::Play}) {
        const auto fileCount = sourceFileCounts_.find(disc);
        if (!sourceRoots_.contains(disc) || fileCount == sourceFileCounts_.end() ||
            fileCount->second == 0u) {
            missing.push_back(disc);
        }
    }
    return missing;
}

std::vector<ResourceIssue> GameDataCatalog::validateRequiredResources(
    const std::vector<std::string>& virtualPaths) const {
    std::vector<ResourceIssue> issues;
    for (const std::string& path : virtualPaths) {
        const LookupStatus status = lookup(path).status;
        if (status != LookupStatus::Found) {
            issues.push_back(ResourceIssue{path, status});
        }
    }
    return issues;
}

std::vector<std::string> GameDataCatalog::executableCandidates() const {
    std::vector<std::string> candidates;
    for (const ManifestEntry& entry : entries_) {
        if (std::any_of(entry.variants.begin(), entry.variants.end(), [](const ContentVariant& variant) {
                return variant.executableCandidate;
            })) {
            candidates.push_back(entry.virtualPath);
        }
    }
    return candidates;
}

std::string GameDataCatalog::toJson() const {
    std::ostringstream output;
    output << "{\n  \"schema_version\": 1,\n  \"missing_discs\": [";
    const std::vector<DiscKind> missing = missingDiscs();
    for (std::size_t i = 0; i < missing.size(); ++i) {
        output << (i == 0u ? "\n    \"" : ",\n    \"") << toString(missing[i]) << '"';
    }
    if (!missing.empty()) {
        output << '\n' << "  ";
    }
    output << "],\n  \"files\": [";
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const ManifestEntry& entry = entries_[i];
        output << (i == 0u ? "\n" : ",\n") << "    {\"path\": \"" << jsonEscape(entry.virtualPath)
               << "\", \"status\": \"" << lookupStatusName(entry.variants.size() > 1u
                                                                 ? LookupStatus::Conflict
                                                                 : LookupStatus::Found)
               << "\", \"variants\": [";
        for (std::size_t variantIndex = 0; variantIndex < entry.variants.size(); ++variantIndex) {
            const ContentVariant& variant = entry.variants[variantIndex];
            output << (variantIndex == 0u ? "" : ",")
                   << "{\"size\": " << variant.size << ", \"sha256\": \"" << variant.sha256
                   << "\", \"executable_candidate\": " << (variant.executableCandidate ? "true" : "false")
                   << ", \"executable_format\": \"" << jsonEscape(variant.executableFormat)
                   << "\", \"origins\": [";
            for (std::size_t originIndex = 0; originIndex < variant.origins.size(); ++originIndex) {
                const FileOrigin& origin = variant.origins[originIndex];
                output << (originIndex == 0u ? "" : ",") << "{\"disc\": \"" << toString(origin.disc)
                       << "\", \"source_path\": \"" << jsonEscape(origin.sourceRelativePath) << "\"}";
            }
            output << "]}";
        }
        output << "]}";
    }
    if (!entries_.empty()) {
        output << '\n' << "  ";
    }
    output << "]\n}\n";
    return output.str();
}

}  // namespace project_los_santos::runtime::gamedata
