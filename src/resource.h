//☀Rise☀
#ifndef rise_resource_h
#define rise_resource_h

#include "rise.h"
#include "utils.h"
#include "context.h"
#include "resource_manager.h"

#include <vulkan/vulkan.h>
#include "pugixml.hpp"

#include <fstream>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>

namespace Rise {

    class ResourceBase {
    public:

        constexpr static bool MetaOnly = false;
        constexpr static bool InstantLoad = false;

        virtual ~ResourceBase() = default; /*
        {
            if (IsLoaded()) {
                // TODO why the fuck you don't want to use virtual method??
                Unload();
            }
        }
        */

        virtual void Unload() {
            MarkUnloaded();
        }

        bool IsLoaded() {
            std::lock_guard<Spinlock> gl(_stateLock);
            return _loaded;
        }

    protected:

        void MarkLoaded() {
            std::lock_guard<Spinlock> gl(_stateLock);
            _loaded = true;
        }

        void MarkUnloaded() {
            std::lock_guard<Spinlock> gl(_stateLock);
            _loaded = false;
        }

    private:

        Spinlock _stateLock;
        bool _loaded = false;
    };

}

#endif /* rise_resource_h */