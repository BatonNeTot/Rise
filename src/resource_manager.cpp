//☀Rise☀

#include "Rise/resource_manager.h"

#include "Rise/resource.h"
#include "Rise/rise.h"
#include "Rise/shader.h"

#include "Rise/node/node.h"

#include "Rise/node/default_node.h"
#include "Rise/node/fake_index_node.h"
#include "Rise/node/grid_node.h"
#include "Rise/node/margin_node.h"
#include "Rise/node/rectangle_node.h"

#include "Rise/node/default_ncomponent.h"
#include "Rise/node/9slice_ncomponent.h"
#include "Rise/node/button_ncomponent.h"
#include "Rise/node/label_ncomponent.h"
#include "Rise/node/texture_ncomponent.h"

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

    void ResourceGenerator::RegisterBuiltinResources() {
        ResourceFabric<Node>::AddFabricKey<Node>("Node");
        ResourceFabric<Node>::AddFabricKey<DefaultNode>("DefaultNode");
        ResourceFabric<Node>::AddFabricKey<FakeIndexNode>("FakeIndexNode");
        ResourceFabric<Node>::AddFabricKey<GridNode>("GridNode");
        ResourceFabric<Node>::AddFabricKey<MarginNode>("MarginNode");
        ResourceFabric<Node>::AddFabricKey<RectangleNode>("RectangleNode");

        ResourceFabric<NComponent>::AddFabricKey<DefaultNComponent>("DefaultNComponent");
        ResourceFabric<NComponent>::AddFabricKey<N9SliceNComponent>("N9SliceNComponent");
        ResourceFabric<NComponent>::AddFabricKey<ButtonNComponent>("ButtonNComponent");
        ResourceFabric<NComponent>::AddFabricKey<LabelNComponent>("LabelNComponent");
        ResourceFabric<NComponent>::AddFabricKey<TextureNComponent>("TextureNComponent");
    }

};