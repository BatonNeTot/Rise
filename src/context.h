//☀Rise☀
#ifndef context_h
#define context_h

#include "utils.h"

#include <typeindex>
#include <thread>
#include <unordered_map>

namespace Rise {
    template <class T>
    class ThreadContext {
    public:

        template <class... Args>
        T& Create(std::thread::id id, Args&&... args) {
            auto result = _values.try_emplace(id, std::forward<Args>(args)...);
            if (!result.second) {
                result.first->second = T(std::forward<Args>(args)...);
            }
            return result.first->second;
        }

        template <class... Args>
        T& Create(Args&&... args) {
            return Create(std::this_thread::get_id(), std::forward<Args>(args)...);
        }

        T& Get() {
            return _values.find(std::this_thread::get_id())->second;
        }

        T* operator->() {
            return &Get();
        }

    private:

        std::unordered_map<std::thread::id, T> _values;

    };

    class IdContextBase {};


    class ContextBase {};

    template <class K, class V>
    class SpecificContext : public ContextBase, public std::unordered_map<K, V> {};

    class AllInOneContainer {
    public:

        template <class K, class V>
        std::unordered_map<K, V>& Get() {
            auto result = _values.try_emplace(CombinedKey<std::type_index, std::type_index>( typeid(K), typeid(V) ));
            if (result.second) {
                result.first->second = new SpecificContext<K, V>();
            }
            return *static_cast<std::unordered_map<K, V>*>(static_cast<SpecificContext<K, V>*>(result.first->second));
        }

        std::unordered_map<CombinedKey<std::type_index, std::type_index>, ContextBase*> _values;
    };
}

#endif /* context_h */