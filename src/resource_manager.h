//☀Rise☀
#ifndef rise_resource_manager_h
#define rise_resource_manager_h

#include "rise.h"
#include "utils.h"
#include "context.h"
#include "allocator.h"
#include "utils/data.h"

#include "loader.h"
#include "pugixml.hpp"

#include <unordered_map>
#include <mutex>
#include <array>
#include <shared_mutex>
#include <unordered_set>
#include <queue>
#include <filesystem>
#include <fstream>

namespace Rise {

    class ResourceBase;

#ifndef RISE_RESOURCE_POOL_SIZE
#define RISE_RESOURCE_POOL_SIZE 4
#endif

    template <class R>
    class ResourcePtr {
    public:

        ResourcePtr() = default;
        ResourcePtr(const std::shared_ptr<ResourceBase>& ptr)
            : _ptr(ptr) {}
        ResourcePtr(R* ptr)
            : _ptr(ptr) {}
        ResourcePtr(R* ptr, const std::function<void(R*)>& deleter)
            : _ptr(ptr, deleter) {}

        R* operator->() const {
            return static_cast<R*>(_ptr.get());
        }

        friend bool operator==(const ResourcePtr<R>& lhs, const ResourcePtr<R>& rhs) {
            return lhs._ptr == rhs._ptr;
        }
        friend bool operator!=(const ResourcePtr<R>& lhs, const ResourcePtr<R>& rhs) {
            return lhs._ptr != rhs._ptr;
        }

        explicit operator bool() const {
            return _ptr != nullptr;
        }

    private:
        friend class ResourceGenerator;

        std::shared_ptr<ResourceBase> _ptr;
    };

    class ResourceManager {
    public:

        ResourceManager();
        ~ResourceManager();

        template <class R, class... Args>
        std::shared_ptr<R> CreateRes(Args&&... args) {
            auto&& sizeOf = static_cast<uint32_t>(sizeof(R));
            auto& allocator = _allocators.try_emplace(sizeOf, sizeOf).first->second;
            auto* res = reinterpret_cast<R*>(allocator.allocate());
            new(res) R(Instance(), std::forward<Args>(args)...);
            return { res, std::bind(&ResourceManager::DestroyRes<R>, this, std::placeholders::_1) };
        }

    private:

        template <class R>
        void DestroyRes(R* res) {
            res->~R();
            auto& allocator = _allocators.find(sizeof(R))->second;
            allocator.deallocate(res);
        }

        friend class Renderer;

        AllInOneContainer _resourceVaults;

        std::mutex _unloadLock;

        std::unordered_map<uint32_t, Allocator> _allocators;

        bool _destroying = false;
    };

    class ResourceGenerator {
    public:

        ResourceGenerator(ResourceManager& manager, Loader& loader)
            : _manager(manager), _loader(loader) {}
        ~ResourceGenerator() {
            for (auto& vaultPair : _resourcesByType) {
                delete vaultPair.second;
            }
        }

        void IndexResources();

        template <class R>
        std::shared_ptr<R> Get(const std::string& id) {
            return ContsructById<R>(id, true);
        }

        template <class R>
        std::shared_ptr<R> Construct(const std::string& id) {
            return ContsructById<R>(id, false);
        }

        template <class R, class K>
        std::shared_ptr<R> GetByKey(const K& key) {
            auto [itVault, emplacedVault] = _resourcesByType.try_emplace(typeid(R));
            if (emplacedVault) {
                itVault->second = new VaultByKey<R, K>();
            }

            auto* vault = static_cast<VaultByKey<R, K>*>(itVault->second);
            auto [it, emplaced] = vault->try_emplace(key);
            auto& resource = it->second;
            if (emplaced) {
                resource = _manager.CreateRes<R>();

                if constexpr (R::InstantLoad) {
                    resource->Load(key);
                }
                else {
                    _loader.AddJob([resource, key]()
                        {
                            resource->Load(key);
                        }
                    );
                }
            }

            return resource;
        }

        const static std::string metaExtension;

        template <class R>
        const std::string& GetFullPath(const std::string& id) const {
            auto fullId = id + "." + R::Ext;
            return _fullpathByExt.find(fullId)->second;
        }

    private:

        template <class R>
        std::shared_ptr<R> ContsructById(const std::string& id, bool cached) {
            auto [itVault, emplacedVault] = _resourcesByType.try_emplace(typeid(R));
            if (emplacedVault) {
                itVault->second = new VaultById<R>();
            }

            auto resourceFile = GetFullPath<R>(id);

            auto* vault = static_cast<VaultById<R>*>(itVault->second);
            auto [it, emplaced] = vault->try_emplace(id);
            auto& data = it->second;
            if (emplaced) {
                auto metaFile = resourceFile + metaExtension;

                {
                    std::ifstream i(metaFile);
                    Data config;
                    i >> config;
                    data._meta.Setup(config);
                }
            }

            if (data._resources.empty() || !cached) {
                auto resource = _manager.CreateRes<R>(&data._meta);
                data._resources.emplace_back(resource);

                if constexpr (R::MetaOnly) {
                    _loader.AddJob([resource]()
                        {
                            resource->Load();
                        }
                    );
                }
                else {
                    _loader.AddJob([resourceFile, resource]()
                        {
                            resource->LoadFromFile(resourceFile);
                        }
                    );
                }
            }

            return *data._resources.begin();
        }

        void IndexResourcesIndirectory(const std::filesystem::path& dir);
        void IndexResource(const std::string& filename);
        
        ResourceManager& _manager;
        Loader& _loader;

        class VaultBase {
        public:
            virtual ~VaultBase() = default;
        };

        template <class R>
        struct ResourceData {
            R::Meta _meta;
            std::vector<std::shared_ptr<R>> _resources;
        };

        template <class R>
        class VaultById : public std::unordered_map<std::string, ResourceData<R>>, public VaultBase {
            ~VaultById() {
                for (auto& pair : *this) {
                    for (auto& ptr : pair.second._resources) {
                        auto useCount = ptr.use_count();
                    }
                }
            }
        };

        template <class R, class K>
        class VaultByKey : public std::unordered_map<K, std::shared_ptr<R>>, public VaultBase {
            ~VaultByKey() {
                for (auto& pair : *this) {
                    auto useCount = pair.second.use_count();
                }
            }
        };

        std::unordered_map<std::type_index, VaultBase *> _resourcesByType;

        std::unordered_map<std::string, std::string> _fullpathByExt;
    };

    template <class R>
    class ResourceFabric {
    private:

        static std::unordered_map<std::string, std::function<std::shared_ptr<R>(ResourceManager&, const Data&)>>& GetRegister() {
            static std::unordered_map<std::string, std::function<std::shared_ptr<R>(ResourceManager&, const Data&)>> fabricsByName;
            return fabricsByName;
        }

    public:

        static std::shared_ptr<R> CreateFromData(const Data& data, ResourceManager& resources) {
            static const std::string node = "Node";
            auto it = GetRegister().find(data["id"].as<std::string>(node));
            return it != GetRegister().end() ? it->second(resources, data) : nullptr;
        }

        template <class T>
        static bool AddFabricKey(const std::string& key) {
            return GetRegister().try_emplace(key, [](ResourceManager& resources, const Data& data) {
                std::shared_ptr<R> ptr = resources.CreateRes<T>(data);
                return ptr;
                }).second;
        }

    };

#define REGISTER_RESOURCE_BY_KEY(resourceType, resource, key) \
namespace {\
    static auto success = ResourceFabric<resourceType>::AddFabricKey<resource>(key);\
}

}

#endif /* rise_resource_manager_h */