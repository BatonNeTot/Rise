//☀Rise☀

#include "resource_manager.h"

#include "resource.h"
#include "rise.h"
#include "shader.h"

#include <filesystem>
#include <iostream>

namespace Rise {

    ResourceManager::ResourceManager() {

    }
    ResourceManager::~ResourceManager() {
        {
            std::unique_lock ul(_unloadLock);
            _destroying = true;
        }
        for (const auto& allocator : _allocators) {
            while(allocator.second.freeAllocations() < allocator.second.capacity()) {}
        }
    }

    const std::string ResourceGenerator::metaExtension = ".meta";

    void ResourceGenerator::IndexResources() {
        IndexResourcesIndirectory(RISE_RESOURCE_DIRECTORY".");
    }

    void ResourceGenerator::IndexResourcesIndirectory(const std::filesystem::path& dir) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_directory()) {
                IndexResourcesIndirectory(entry);
                continue;
            }
            const auto&& path = entry.path().string();
            if (!EndsWith(path, metaExtension)) {
                continue;
            }
            IndexResource(path);
        }
    }

    void ResourceGenerator::IndexResource(const std::string& fullFilename) {
        auto separatorEnd = fullFilename.find_last_of(std::filesystem::path::preferred_separator);
        auto filename = std::string_view(fullFilename);
        if (separatorEnd != std::string::npos) {
            filename = filename.substr(separatorEnd + 1);
        }
        auto idWithExt = filename.substr(0, filename.length() - metaExtension.length());
        _fullpathByExt.try_emplace(std::string(idWithExt), fullFilename.substr(0, fullFilename.length() - metaExtension.length()));
    }

};