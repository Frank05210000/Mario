#ifndef ASSET_PATH_HPP
#define ASSET_PATH_HPP

#include <filesystem>
#include <string>

inline std::string MakeAssetPath(const std::string& relativePath) {
    const auto assetRoot = std::filesystem::path(RESOURCE_DIR) / "Asset";
    return (assetRoot / relativePath).string();
}

#endif
