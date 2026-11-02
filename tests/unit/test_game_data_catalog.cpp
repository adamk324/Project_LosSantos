#include "runtime/gamedata/GameDataCatalog.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using project_los_santos::runtime::gamedata::DiscKind;
using project_los_santos::runtime::gamedata::DiscSource;
using project_los_santos::runtime::gamedata::GameDataCatalog;
using project_los_santos::runtime::gamedata::LookupStatus;

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

std::string readFile(std::ifstream file) {
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
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

    std::cout << "Two-disc game-data catalog synthetic validation passed.\n";
    return 0;
}
