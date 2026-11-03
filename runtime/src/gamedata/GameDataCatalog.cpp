#include "runtime/gamedata/GameDataCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <system_error>
#include <tuple>
#include <utility>

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

class FileRegionBuffer final : public std::streambuf {
public:
    FileRegionBuffer(const fs::path& path, uint64_t offset, uint64_t size)
        : file_(path, std::ios::binary), offset_(offset), size_(size) {
        setg(buffer_.data(), buffer_.data(), buffer_.data());
    }

    [[nodiscard]] bool isOpen() const noexcept { return file_.is_open(); }

protected:
    int_type underflow() override {
        if (gptr() != nullptr && gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }
        if (nextReadOffset_ >= size_) {
            return traits_type::eof();
        }

        const uint64_t remaining = size_ - nextReadOffset_;
        const auto readSize = static_cast<std::streamsize>(
            std::min<uint64_t>(remaining, static_cast<uint64_t>(buffer_.size())));
        file_.clear();
        file_.seekg(static_cast<std::streamoff>(offset_ + nextReadOffset_));
        if (!file_) {
            return traits_type::eof();
        }
        file_.read(buffer_.data(), readSize);
        const std::streamsize bytesRead = file_.gcount();
        if (bytesRead <= 0) {
            return traits_type::eof();
        }
        nextReadOffset_ += static_cast<uint64_t>(bytesRead);
        setg(buffer_.data(), buffer_.data(), buffer_.data() + bytesRead);
        return traits_type::to_int_type(*gptr());
    }

    pos_type seekoff(off_type offset, std::ios_base::seekdir direction,
                     std::ios_base::openmode mode) override {
        if ((mode & std::ios_base::in) == 0) {
            return pos_type(off_type(-1));
        }
        const uint64_t current = nextReadOffset_ -
                                 static_cast<uint64_t>(egptr() - gptr());
        int64_t base = 0;
        if (direction == std::ios_base::cur) {
            if (current > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                return pos_type(off_type(-1));
            }
            base = static_cast<int64_t>(current);
        } else if (direction == std::ios_base::end) {
            if (size_ > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                return pos_type(off_type(-1));
            }
            base = static_cast<int64_t>(size_);
        } else if (direction != std::ios_base::beg) {
            return pos_type(off_type(-1));
        }

        if ((offset < 0 && base < -static_cast<int64_t>(offset)) ||
            (offset > 0 && base > std::numeric_limits<int64_t>::max() - offset)) {
            return pos_type(off_type(-1));
        }
        const int64_t target = base + offset;
        if (target < 0 || static_cast<uint64_t>(target) > size_) {
            return pos_type(off_type(-1));
        }

        nextReadOffset_ = static_cast<uint64_t>(target);
        setg(buffer_.data(), buffer_.data(), buffer_.data());
        return pos_type(static_cast<off_type>(target));
    }

    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override {
        return seekoff(static_cast<off_type>(position), std::ios_base::beg, mode);
    }

private:
    std::ifstream file_;
    uint64_t offset_ = 0;
    uint64_t size_ = 0;
    uint64_t nextReadOffset_ = 0;
    std::array<char, 64u * 1024u> buffer_{};
};

class FileRegionStream final : public std::istream {
public:
    FileRegionStream(const fs::path& path, uint64_t offset, uint64_t size)
        : std::istream(nullptr), buffer_(path, offset, size) {
        rdbuf(&buffer_);
        if (!buffer_.isOpen()) {
            setstate(std::ios_base::failbit);
        }
    }

private:
    FileRegionBuffer buffer_;
};

std::unique_ptr<std::istream> openRegion(const fs::path& path, uint64_t offset, uint64_t size) {
    auto stream = std::make_unique<FileRegionStream>(path, offset, size);
    if (!*stream) {
        throw std::runtime_error("Unable to read game-data source: " + path.string());
    }
    return stream;
}

std::string hashRegion(const fs::path& path, uint64_t offset, uint64_t size) {
    std::unique_ptr<std::istream> file = openRegion(path, offset, size);
    Sha256 hash;
    std::array<uint8_t, 64u * 1024u> buffer{};
    while (*file) {
        file->read(reinterpret_cast<char*>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytesRead = file->gcount();
        if (bytesRead > 0) {
            hash.update(buffer.data(), static_cast<std::size_t>(bytesRead));
        }
    }
    if (!file->eof()) {
        throw std::runtime_error("Failed while reading game-data source: " + path.string());
    }
    return hash.finish();
}

bool sameContents(const fs::path& leftPath, uint64_t leftOffset,
                  const fs::path& rightPath, uint64_t rightOffset, uint64_t size) {
    std::unique_ptr<std::istream> left = openRegion(leftPath, leftOffset, size);
    std::unique_ptr<std::istream> right = openRegion(rightPath, rightOffset, size);
    std::array<char, 64u * 1024u> leftBuffer{};
    std::array<char, 64u * 1024u> rightBuffer{};
    uint64_t remaining = size;
    while (remaining > 0u) {
        const auto chunk = static_cast<std::streamsize>(
            std::min<uint64_t>(remaining, static_cast<uint64_t>(leftBuffer.size())));
        left->read(leftBuffer.data(), chunk);
        right->read(rightBuffer.data(), chunk);
        const std::streamsize leftCount = left->gcount();
        const std::streamsize rightCount = right->gcount();
        if (leftCount != rightCount ||
            !std::equal(leftBuffer.begin(), leftBuffer.begin() + leftCount, rightBuffer.begin())) {
            return false;
        }
        if (leftCount != chunk) {
            throw std::runtime_error("Unexpected end of game-data source while comparing files.");
        }
        remaining -= static_cast<uint64_t>(leftCount);
    }
    return true;
}

struct IndexedImageFile {
    std::string path;
    uint64_t offset = 0;
    uint64_t size = 0;
};

struct XdvdfsVolume {
    uint64_t imageOffset = 0;
    uint32_t rootSector = 0;
    uint32_t rootSize = 0;
};

constexpr uint64_t kXdvdfsSectorSize = 2048u;
constexpr std::array<uint8_t, 20> kXdvdfsMagic{
    'M', 'I', 'C', 'R', 'O', 'S', 'O', 'F', 'T', '*',
    'X', 'B', 'O', 'X', '*', 'M', 'E', 'D', 'I', 'A',
};

uint32_t readLittleEndian32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8u) |
           (static_cast<uint32_t>(bytes[2]) << 16u) |
           (static_cast<uint32_t>(bytes[3]) << 24u);
}

uint16_t readLittleEndian16(const uint8_t* bytes) {
    return static_cast<uint16_t>(bytes[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(bytes[1]) << 8u);
}

void readAt(std::ifstream& file, uint64_t imageSize, uint64_t offset,
            uint8_t* destination, std::size_t size, const fs::path& path) {
    if (offset > imageSize || size > imageSize - offset ||
        offset > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        throw std::runtime_error("XDVDFS metadata points outside the image: " + path.string());
    }
    file.clear();
    file.seekg(static_cast<std::streamoff>(offset));
    if (!file || !file.read(reinterpret_cast<char*>(destination),
                            static_cast<std::streamsize>(size))) {
        throw std::runtime_error("Unable to read XDVDFS metadata: " + path.string());
    }
}

std::string decodeWindows1252(const uint8_t* bytes, std::size_t size) {
    constexpr std::array<uint16_t, 32> kControlCodepoints{
        0x20ACu, 0x0081u, 0x201Au, 0x0192u, 0x201Eu, 0x2026u, 0x2020u, 0x2021u,
        0x02C6u, 0x2030u, 0x0160u, 0x2039u, 0x0152u, 0x008Du, 0x017Du, 0x008Fu,
        0x0090u, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2013u, 0x2014u,
        0x02DCu, 0x2122u, 0x0161u, 0x203Au, 0x0153u, 0x009Du, 0x017Eu, 0x0178u,
    };

    std::string result;
    for (std::size_t i = 0; i < size; ++i) {
        const uint8_t byte = bytes[i];
        uint32_t codepoint = byte;
        if (byte >= 0x80u && byte <= 0x9Fu) {
            codepoint = kControlCodepoints[byte - 0x80u];
        }
        if (codepoint < 0x80u) {
            result.push_back(static_cast<char>(codepoint));
        } else if (codepoint < 0x800u) {
            result.push_back(static_cast<char>(0xC0u | (codepoint >> 6u)));
            result.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        } else {
            result.push_back(static_cast<char>(0xE0u | (codepoint >> 12u)));
            result.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
            result.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
        }
    }
    return result;
}

XdvdfsVolume readXdvdfsVolume(std::ifstream& image, uint64_t imageSize,
                              const fs::path& path) {
    constexpr std::array<uint64_t, 4> kImageOffsets{
        0u, 0x02080000u, 0x0FD90000u, 0x18300000u,
    };
    constexpr uint64_t kDescriptorOffset = 0x10000u;
    constexpr uint64_t kDescriptorSize = 0x800u;

    for (const uint64_t imageOffset : kImageOffsets) {
        if (imageOffset > imageSize ||
            kDescriptorOffset + kDescriptorSize > imageSize - imageOffset) {
            continue;
        }
        std::array<uint8_t, kDescriptorSize> descriptor{};
        readAt(image, imageSize, imageOffset + kDescriptorOffset, descriptor.data(),
               descriptor.size(), path);
        if (!std::equal(kXdvdfsMagic.begin(), kXdvdfsMagic.end(), descriptor.begin()) ||
            !std::equal(kXdvdfsMagic.begin(), kXdvdfsMagic.end(),
                        descriptor.begin() + 0x7EC)) {
            continue;
        }

        XdvdfsVolume volume;
        volume.imageOffset = imageOffset;
        volume.rootSector = readLittleEndian32(descriptor.data() + 0x14);
        volume.rootSize = readLittleEndian32(descriptor.data() + 0x18);
        const uint64_t rootOffset =
            imageOffset + static_cast<uint64_t>(volume.rootSector) * kXdvdfsSectorSize;
        if (volume.rootSize < 14u || rootOffset > imageSize ||
            volume.rootSize > imageSize - rootOffset) {
            throw std::runtime_error("XDVDFS root directory region is invalid: " + path.string());
        }
        return volume;
    }
    throw std::runtime_error("No supported XDVDFS volume descriptor found: " + path.string());
}

std::vector<IndexedImageFile> scanXdvdfsImage(const fs::path& path, uint64_t imageSize) {
    std::ifstream image(path, std::ios::binary);
    if (!image) {
        throw std::runtime_error("Unable to open Xbox 360 disc image: " + path.string());
    }

    const XdvdfsVolume volume = readXdvdfsVolume(image, imageSize, path);
    std::vector<IndexedImageFile> files;
    std::set<std::tuple<uint64_t, uint64_t, std::string>> activeDirectories;
    constexpr std::size_t kMaximumFiles = 1000000u;
    constexpr std::size_t kMaximumDepth = 128u;

    std::function<void(uint64_t, uint32_t, const std::string&, std::size_t)> scanDirectory;
    scanDirectory = [&](uint64_t sector, uint32_t tableSize, const std::string& parent,
                        std::size_t depth) {
        if (depth > kMaximumDepth) {
            throw std::runtime_error("XDVDFS directory nesting exceeds the safety limit.");
        }
        const uint64_t tableOffset =
            volume.imageOffset + sector * kXdvdfsSectorSize;
        if (tableOffset > imageSize || tableSize > imageSize - tableOffset) {
            throw std::runtime_error("XDVDFS directory table lies outside the image.");
        }
        const auto key = std::make_tuple(sector, tableSize, parent);
        if (!activeDirectories.insert(key).second) {
            throw std::runtime_error("XDVDFS directory cycle detected.");
        }

        std::set<uint64_t> visitedNodes;
        std::vector<std::pair<uint64_t, bool>> pendingNodes{{0u, true}};
        while (!pendingNodes.empty()) {
            const auto [nodeOffset, isRoot] = pendingNodes.back();
            pendingNodes.pop_back();
            if (!isRoot &&
                (nodeOffset == 0u || nodeOffset == 0xFFu || nodeOffset == 0xFFFFu)) {
                continue;
            }
            if (!visitedNodes.insert(nodeOffset).second) {
                throw std::runtime_error("XDVDFS directory tree contains a cycle.");
            }
            if (visitedNodes.size() > kMaximumFiles) {
                throw std::runtime_error("XDVDFS directory node count exceeds the safety limit.");
            }
            const uint64_t entryOffset = nodeOffset * 4u;
            if (entryOffset > tableSize || 14u > tableSize - entryOffset) {
                throw std::runtime_error("XDVDFS directory node lies outside its table.");
            }
            std::array<uint8_t, 14> entry{};
            readAt(image, imageSize, tableOffset + entryOffset, entry.data(), entry.size(), path);
            const bool emptyZero = std::all_of(entry.begin(), entry.end(),
                                                [](uint8_t value) { return value == 0u; });
            const bool emptyOnes = std::all_of(entry.begin(), entry.end(),
                                                [](uint8_t value) { return value == 0xFFu; });
            if (emptyZero || emptyOnes) {
                continue;
            }

            const uint16_t left = readLittleEndian16(entry.data());
            const uint16_t right = readLittleEndian16(entry.data() + 2u);
            const uint32_t fileSector = readLittleEndian32(entry.data() + 4u);
            const uint32_t fileSize = readLittleEndian32(entry.data() + 8u);
            const uint8_t attributes = entry[12];
            const uint8_t nameLength = entry[13];
            if (nameLength == 0u || nameLength > tableSize - entryOffset - 14u) {
                throw std::runtime_error("XDVDFS directory entry has an invalid filename.");
            }

            std::vector<uint8_t> nameBytes(nameLength);
            readAt(image, imageSize, tableOffset + entryOffset + 14u, nameBytes.data(),
                   nameBytes.size(), path);
            const std::string name = decodeWindows1252(nameBytes.data(), nameBytes.size());
            if (name.empty() || name == "." || name == ".." ||
                name.find_first_of("/\\") != std::string::npos) {
                throw std::runtime_error("XDVDFS directory entry contains an unsafe filename.");
            }
            const std::string virtualPath = parent.empty() ? name : parent + "/" + name;
            const uint64_t dataOffset =
                volume.imageOffset + static_cast<uint64_t>(fileSector) * kXdvdfsSectorSize;
            if (dataOffset > imageSize || fileSize > imageSize - dataOffset) {
                throw std::runtime_error("XDVDFS file region lies outside the image.");
            }

            if ((attributes & 0x10u) != 0u) {
                if (fileSize != 0u) {
                    scanDirectory(fileSector, fileSize, virtualPath, depth + 1u);
                }
            } else {
                files.push_back(IndexedImageFile{virtualPath, dataOffset, fileSize});
                if (files.size() > kMaximumFiles) {
                    throw std::runtime_error("XDVDFS file count exceeds the safety limit.");
                }
            }
            pendingNodes.emplace_back(right, false);
            pendingNodes.emplace_back(left, false);
        }
        activeDirectories.erase(key);
    };

    scanDirectory(volume.rootSector, volume.rootSize, {}, 0u);
    return files;
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

std::string toString(SourceFormat format) {
    switch (format) {
        case SourceFormat::Auto:
            return "auto";
        case SourceFormat::Directory:
            return "directory";
        case SourceFormat::Xbox360Iso:
            return "xbox360-xdvdfs-iso";
    }
    throw std::logic_error("Unknown game-data source format.");
}

GameDataCatalog GameDataCatalog::scan(const std::vector<DiscSource>& sources) {
    GameDataCatalog catalog;
    std::map<DiscKind, std::size_t> sourceOrdinals;
    for (const DiscSource& source : sources) {
        if (source.root.empty()) {
            throw std::invalid_argument("Game-data source roots must not be empty.");
        }

        std::error_code error;
        const fs::path sourcePath = fs::absolute(source.root, error).lexically_normal();
        if (error) {
            throw fs::filesystem_error("Unable to resolve game-data source path", source.root, error);
        }

        SourceFormat format = source.format;
        if (format == SourceFormat::Auto) {
            if (fs::is_directory(sourcePath, error) && !error) {
                format = SourceFormat::Directory;
            } else {
                error.clear();
                if (fs::is_regular_file(sourcePath, error) && !error) {
                    std::string extension = sourcePath.extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(),
                                   [](unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                    });
                    if (extension == ".iso") {
                        format = SourceFormat::Xbox360Iso;
                    }
                }
            }
        }

        if (format == SourceFormat::Directory) {
            if (!fs::is_directory(sourcePath, error) || error) {
                throw fs::filesystem_error("Game-data source is not an accessible directory",
                                           sourcePath, error);
            }
        } else if (format == SourceFormat::Xbox360Iso) {
            if (!fs::is_regular_file(sourcePath, error) || error) {
                throw fs::filesystem_error("Xbox 360 ISO source is not an accessible file",
                                           sourcePath, error);
            }
        } else {
            throw std::invalid_argument("Unable to infer game-data source format: " +
                                        sourcePath.string());
        }

        std::string sourceId = source.sourceId;
        if (sourceId.empty()) {
            sourceId = toString(source.disc) + "-" +
                       std::to_string(sourceOrdinals[source.disc]);
        }
        ++sourceOrdinals[source.disc];
        if (catalog.sources_.contains(sourceId)) {
            throw std::invalid_argument("Game-data source IDs must be unique: " + sourceId);
        }
        for (const auto& [existingId, existingSource] : catalog.sources_) {
            (void)existingId;
            const bool sameRoot = fs::equivalent(existingSource.path, sourcePath, error);
            if (error) {
                throw fs::filesystem_error("Unable to compare game-data source roots",
                                           existingSource.path, sourcePath, error);
            }
            if (sameRoot) {
                throw std::invalid_argument("A game-data source cannot be registered more than once.");
            }
        }

        SourceInfo sourceInfo{source.disc, sourcePath, format};
        std::vector<IndexedImageFile> imageFiles;
        if (format == SourceFormat::Xbox360Iso) {
            const uint64_t imageSize = fs::file_size(sourcePath, error);
            if (error) {
                throw fs::filesystem_error("Unable to determine Xbox 360 ISO size", sourcePath, error);
            }
            imageFiles = scanXdvdfsImage(sourcePath, imageSize);
        }
        catalog.sources_.emplace(sourceId, sourceInfo);

        const auto addFile = [&](const std::string& virtualPath, const fs::path& filePath,
                                 uint64_t offset, uint64_t size) {
            const std::string key = normalizeVirtualPath(virtualPath);
            const std::string digest = hashRegion(filePath, offset, size);
            std::string formatName = fs::path(virtualPath).extension().empty()
                                         ? std::string{}
                                         : executableFormat(fs::path(virtualPath));
            const auto prefix = openRegion(filePath, offset,
                                           std::min<uint64_t>(size, 4u));
            std::array<char, 4> magic{};
            prefix->read(magic.data(), static_cast<std::streamsize>(magic.size()));
            if (prefix->gcount() == static_cast<std::streamsize>(magic.size()) &&
                magic == std::array<char, 4>{'X', 'E', 'X', '2'}) {
                formatName = "xex2";
            }
            const bool executable = !formatName.empty();
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
                const SourceInfo& existingSource = catalog.sources_.at(origin.sourceId);
                const fs::path existingPath = existingSource.format == SourceFormat::Directory
                                                  ? existingSource.path /
                                                        fs::path(origin.sourceRelativePath)
                                                  : existingSource.path;
                return sameContents(existingPath, origin.byteOffset, filePath, offset, size);
            });
            FileOrigin origin{source.disc, virtualPath, sourceId, offset};
            if (variant == entry.variants.end()) {
                ContentVariant content;
                content.size = size;
                content.sha256 = digest;
                content.executableCandidate = executable;
                content.executableFormat = formatName;
                content.origins.push_back(std::move(origin));
                entry.variants.push_back(std::move(content));
            } else {
                variant->origins.push_back(std::move(origin));
                if (formatName == "xex2" ||
                    (variant->executableFormat.empty() && !formatName.empty())) {
                    variant->executableFormat = formatName;
                }
                variant->executableCandidate = variant->executableCandidate || executable;
            }
        };

        if (format == SourceFormat::Directory) {
            fs::recursive_directory_iterator iterator(sourcePath, fs::directory_options::none, error);
            const fs::recursive_directory_iterator end;
            if (error) {
                throw fs::filesystem_error("Unable to inspect game-data source", sourcePath, error);
            }
            while (iterator != end) {
                const fs::directory_entry item = *iterator;
                const fs::path relativePath = item.path().lexically_relative(sourcePath);
                const std::string virtualPath = relativePath.generic_string();

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
                            throw fs::filesystem_error("Unable to get game-data file size",
                                                       item.path(), error);
                        }
                        addFile(virtualPath, item.path(), 0u, size);
                    }
                }

                iterator.increment(error);
                if (error) {
                    throw fs::filesystem_error("Failed while scanning game-data source",
                                               sourcePath, error);
                }
            }
        } else {
            for (const IndexedImageFile& file : imageFiles) {
                addFile(file.path, sourcePath, file.offset, file.size);
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
                if (left.sourceId != right.sourceId) {
                    return left.sourceId < right.sourceId;
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

FileLocation GameDataCatalog::resolveFile(std::string_view virtualPath, ResolutionPolicy policy,
                                          std::string_view sourceId) const {
    if (policy == ResolutionPolicy::SelectSource && sourceId.empty()) {
        throw std::invalid_argument("SelectSource resolution requires a source ID.");
    }
    if (policy == ResolutionPolicy::RejectConflicts && !sourceId.empty()) {
        throw std::invalid_argument("A source ID is valid only with SelectSource resolution.");
    }

    const LookupResult result = lookup(virtualPath);
    if (result.status == LookupStatus::NotFound) {
        return {};
    }
    const ContentVariant* selectedVariant = nullptr;
    const FileOrigin* selectedOrigin = nullptr;
    if (policy == ResolutionPolicy::RejectConflicts) {
        if (result.status == LookupStatus::Conflict) {
            throw std::runtime_error("Ambiguous game-data path has different contents: " +
                                     std::string(virtualPath));
        }
        selectedVariant = &result.entry->variants.front();
        selectedOrigin = &selectedVariant->origins.front();
    } else {
        for (const ContentVariant& variant : result.entry->variants) {
            const auto origin = std::find_if(
                variant.origins.begin(), variant.origins.end(),
                [&](const FileOrigin& candidate) { return candidate.sourceId == sourceId; });
            if (origin != variant.origins.end()) {
                selectedVariant = &variant;
                selectedOrigin = &*origin;
                break;
            }
        }
        if (selectedOrigin == nullptr) {
            throw std::runtime_error("Requested source does not contain game-data path: " +
                                     std::string(virtualPath));
        }
    }

    const SourceInfo& source = sources_.at(selectedOrigin->sourceId);
    const fs::path containerPath =
        source.format == SourceFormat::Directory
            ? source.path / fs::path(selectedOrigin->sourceRelativePath)
            : source.path;
    return FileLocation{containerPath, selectedOrigin->sourceId, selectedOrigin->byteOffset,
                        selectedVariant->size, source.format};
}

fs::path GameDataCatalog::resolvePath(std::string_view virtualPath, ResolutionPolicy policy,
                                      std::string_view sourceId) const {
    const FileLocation location = resolveFile(virtualPath, policy, sourceId);
    if (location.containerPath.empty()) {
        return {};
    }
    if (location.format != SourceFormat::Directory) {
        throw std::runtime_error("The resource is inside an ISO image; use openResource instead.");
    }
    return location.containerPath;
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

std::unique_ptr<std::istream> GameDataCatalog::openResource(std::string_view virtualPath,
                                                            ResolutionPolicy policy,
                                                            std::string_view sourceId) const {
    const FileLocation location = resolveFile(virtualPath, policy, sourceId);
    if (location.containerPath.empty()) {
        throw std::runtime_error("Game-data resource was not found: " + std::string(virtualPath));
    }
    return openRegion(location.containerPath, location.byteOffset, location.size);
}

std::vector<DiscKind> GameDataCatalog::missingDiscs() const {
    std::vector<DiscKind> missing;
    for (const DiscKind disc : {DiscKind::Install, DiscKind::Play}) {
        const auto fileCount = sourceFileCounts_.find(disc);
        const bool hasSource = std::any_of(sources_.begin(), sources_.end(),
                                           [disc](const auto& source) {
            return source.second.disc == disc;
        });
        if (!hasSource || fileCount == sourceFileCounts_.end() || fileCount->second == 0u) {
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
    output << "{\n  \"schema_version\": 2,\n  \"missing_discs\": [";
    const std::vector<DiscKind> missing = missingDiscs();
    for (std::size_t i = 0; i < missing.size(); ++i) {
        output << (i == 0u ? "\n    \"" : ",\n    \"") << toString(missing[i]) << '"';
    }
    if (!missing.empty()) {
        output << '\n' << "  ";
    }
    output << "],\n  \"sources\": [";
    std::size_t sourceIndex = 0;
    for (const auto& [id, source] : sources_) {
        output << (sourceIndex++ == 0u ? "\n" : ",\n")
               << "    {\"source_id\": \"" << jsonEscape(id) << "\", \"disc\": \""
               << toString(source.disc) << "\", \"format\": \"" << toString(source.format)
               << "\"}";
    }
    if (!sources_.empty()) {
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
                       << "\", \"source_id\": \"" << jsonEscape(origin.sourceId)
                       << "\", \"source_path\": \"" << jsonEscape(origin.sourceRelativePath)
                       << "\", \"byte_offset\": " << origin.byteOffset << "}";
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
