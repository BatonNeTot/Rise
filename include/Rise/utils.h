//☀Rise☀
#ifndef utils_h
#define utils_h

#include "types.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include <functional>
#include <string>
#include <optional>
#include <mutex>

#define EVAL0(...) __VA_ARGS__
#define EVAL1(...) EVAL0(EVAL0(EVAL0(__VA_ARGS__)))
#define EVAL2(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL3(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL4(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL(...)  EVAL4(EVAL4(EVAL4(__VA_ARGS__)))

#define MAP_END(...)
#define MAP_OUT
#define MAP_COMMA ,

#define MAP_GET_END2() 0, MAP_END
#define MAP_GET_END1(...) MAP_GET_END2
#define MAP_GET_END(...) MAP_GET_END1
#define MAP_NEXT0(test, next, ...) next MAP_OUT
#define MAP_NEXT1(test, next) MAP_NEXT0(test, next, 0)
#define MAP_NEXT(test, next)  MAP_NEXT1(MAP_GET_END test, next)

#define MAP0(f, x, peek, ...) f(x) MAP_NEXT(peek, MAP1)(f, peek, __VA_ARGS__)
#define MAP1(f, x, peek, ...) f(x) MAP_NEXT(peek, MAP0)(f, peek, __VA_ARGS__)

#define MAP_LIST_NEXT1(test, next) MAP_NEXT0(test, MAP_COMMA next, 0)
#define MAP_LIST_NEXT(test, next)  MAP_LIST_NEXT1(MAP_GET_END test, next)

#define MAP_LIST0(f, x, peek, ...) f(x) MAP_LIST_NEXT(peek, MAP_LIST1)(f, peek, __VA_ARGS__)
#define MAP_LIST1(f, x, peek, ...) f(x) MAP_LIST_NEXT(peek, MAP_LIST0)(f, peek, __VA_ARGS__)

/**
 * Applies the function macro `f` to each of the remaining parameters.
 */
#define MAP(f, ...) EVAL(MAP1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#define MAP_TUPLES0(f, x, peek, ...) f x MAP_NEXT(peek, MAP_TUPLES1)(f, peek, __VA_ARGS__)
#define MAP_TUPLES1(f, x, peek, ...) f x MAP_NEXT(peek, MAP_TUPLES0)(f, peek, __VA_ARGS__)
#define MAP_TUPLES(f, ...) EVAL(MAP_TUPLES1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#define DECL_ENUM(type,var)\
  var,
#define DECL_TYPE(type,var)\
  type,

#define UNPACK(...) __VA_ARGS__

#define CREATE_CLASS(Class_Name, Container_Type, Variables_Names, Variables_Types)\
class Class_Name : public Container_Type<UNPACK##Variables_Types> {\
public:\
    enum Variables {\
        UNPACK##Variables_Names\
    };\
}

namespace Rise {

    template <typename T>
    struct HasEmplace
    {
        struct dummy { /* something */ };
        template <typename C, typename P> static auto test(P* p) -> decltype(std::declval<C>().emplace(*p), std::true_type());
        template <typename, typename> static std::false_type test(...);

        typedef decltype(test<T, dummy>(nullptr)) type; static const bool value = std::is_same<std::true_type, decltype(test<T, dummy>(nullptr))>::value;
    };

    template <typename T>
    struct HasEmplaceBack
    {
        struct dummy { /* something */ };
        template <typename C, typename P> static auto test(P* p) -> decltype(std::declval<C>().emplace_back(*p), std::true_type());
        template <typename, typename> static std::false_type test(...);

        typedef decltype(test<T, dummy>(nullptr)) type; static const bool value = std::is_same<std::true_type, decltype(test<T, dummy>(nullptr))>::value;
    }; 
    
    inline uint32_t constexpr Length(const char* str)
    {
        return *str == '\0' ? 1 + Length(str + 1) : 0;
    }

    inline bool EndsWith(const std::string& str, const std::string& tail) {
        return str.compare(str.length() - tail.length(), tail.length(), tail) == 0;
    }

    inline uint32_t Split(const std::string_view& str, const std::string_view& delimiter, const std::function<bool(const std::string_view& str)>& action) {
        uint32_t counter = 0;
        if (str.length() == 0) {
            return counter;
        }

        if (delimiter.length() == 0) {
            for (size_t i = 0; i < str.length(); ++i) {
                ++counter;
                if (!action(str.substr(i, 1))) {
                    return counter;
                }
            }
            return counter;
        }

        size_t start = 0;
        for (size_t pos = str.find(delimiter, start); pos != std::string::npos; pos = str.find(delimiter, start)) {
            ++counter;
            if (!action(str.substr(start, pos - start))) {
                return counter;
            }
            start = pos + delimiter.length();
        }
        ++counter;
        action(str.substr(start));
        return counter;
    }

    inline uint32_t Split(const std::string_view& str, const std::string_view& delimiter, std::vector <std::string>& container) {
        return Split(str, delimiter, [&container](const std::string_view& str) {
            container.emplace_back(str);
            return true;
            });
    }

    inline uint32_t Split(const std::string_view& str, const std::string_view& delimiter, std::pair <std::string, std::string>& container) {
        int counter = 0;
        return Split(str, delimiter, [&container, &counter](const std::string_view& str) {
            if (counter == 0) {
                container.first = str;
                ++counter;
                return true;
            }
            else {
                container.second = str;
                return false;
            }
            });
    }

    template <class... T>
    struct CombinedValue : public std::tuple<T...> {

    public:
        template <size_t index>
        using TypeAt = std::remove_reference<decltype(std::get<index>(std::tuple<T...>()))>::type;

        constexpr static size_t TotalCount = sizeof...(T);

        CombinedValue()
            : std::tuple<T...>() {}
        template <class... Args>
        CombinedValue(Args&&... values)
            : std::tuple<T...>(std::forward<Args>(values)...) {}

        template <size_t index>
        auto& Get() {
            return std::get<index>(*this);
        }

        template <size_t index>
        const auto& Get() const {
            return std::get<index>(*this);
        }
    };

    template <class... T>
    struct RecursiveValue {
    public:

        using Type = RecursiveValue<T...>;

        template <size_t index>
        using TypeAt = CombinedValue<T...>::TypeAt<index>;

        template <size_t index>
        using ConstRefAt = decltype(std::cref(CombinedValue<T...>().Get<static_cast<size_t>(index)>()).get());

        constexpr static size_t TotalCount = CombinedValue<T...>::TotalCount;

        RecursiveValue() {}
        RecursiveValue(T&&... values)
            : _values(std::forward<T>(values)...) {}

        RecursiveValue(const Type& value)
            : _parent(&value) {}
        RecursiveValue(Type&& value)
            : _parent(&value), _values(std::move(value._values)) {
            value._parent = nullptr;
        }

        template <size_t index>
        auto Get() const -> ConstRefAt<index> {
            auto& value = _values.Get<static_cast<size_t>(index)>();
            return value.has_value() || _parent == nullptr ? value.value() : _parent->Get<index>();
        }

        template <size_t index, class... Args>
        void Set(Args&&... args) {
            auto& value = _values.Get<static_cast<size_t>(index)>();
            value.emplace(std::forward<Args>(args)...);
        }

        template <size_t index>
        bool LocalExist() const {
            auto& value = _values.Get<static_cast<size_t>(index)>();
            return value.has_value();
        }

        template <size_t index>
        bool Exist() const {
            return LocalExist<index>() || (_parent != nullptr && _parent->LocalExist<index>());
        }

        const Type* Parent() const {
            return _parent;
        }

    private:
        const Type* _parent = nullptr;
        CombinedValue<std::optional<T>...> _values;
    };

    template <class... T>
    struct CombinedKey {

    public:
        template <class... Args>
        CombinedKey(Args&&... values)
            : _value(std::forward<Args>(values)...), _hash(HashImpl()) {}

        template <size_t index>
        const auto& Get() {
            return _value.template Get<index>();
        }

        template <size_t index>
        const auto& Get() const {
            return _value.template Get<index>();
        }

        size_t Hash() const {
            return _hash;
        }

        friend bool operator ==(const CombinedKey<T...>& lhs, const CombinedKey<T...>& rhs) {
            return lhs._value == rhs._value;
        }

        CombinedKey<T...>& operator =(const CombinedKey<T...>& key) {
            _value = key._value;
            _hash = HashImpl();
            return *this;
        }

        operator const CombinedValue<T...>& () const {
            return _value;
        }

    private:

        size_t HashImpl() const {
            return HashImpl(std::make_index_sequence<sizeof...(T)>{}, * this);
        }

        template <size_t... ints>
        static size_t HashImpl(std::integer_sequence<size_t, ints...> int_seq, const CombinedKey& value) {
            size_t h = 0;
            (void)std::initializer_list<int>{((h ^= (std::hash<T>{}(value.Get<ints>()) << 1)), 0)...};
            return h >> 1;
        }

        CombinedValue<T...> _value;
        size_t _hash;
    };

    template <class... T>
    struct ::std::hash<Rise::CombinedKey<T...>>
    {
        std::size_t operator()(const Rise::CombinedKey<T...>& key) const noexcept
        {
            return key.Hash();
        }
    };

    template <class T>
    struct ::std::hash<std::vector<T>>
    {
        std::size_t operator()(const std::vector<T>& container) const noexcept
        {
            std::size_t hash = 0;
            for (auto& value : container) {
                hash = std::hash<T>()(value) ^ (hash << 1);
            }
            return hash;
        }
    };

    template <class K, class V>
    struct ::std::hash<std::unordered_map<K, V>>
    {
        std::size_t operator()(const std::unordered_map<K, V>& container) const noexcept
        {
            std::size_t hash = 0;
            for (auto& [key, value] : container) {
                hash ^= std::hash<K>()(key) * std::hash<V>()(value);
            }
            return hash;
        }
    };

    constexpr unsigned int Hash(const char* str, int h = 0)
    {
        return !str[h] ? 5381 : (Hash(str, h + 1) * 33) ^ str[h];
    }

    constexpr unsigned int Hash(const std::string& str, int h = 0)
    {
        return Hash(str.c_str(), h);
    }

    template <class T>
    struct __LexicalCastFrom {};

    template <>
    struct __LexicalCastFrom<int32_t> {
        inline static int32_t Get(const std::string& str) {
            return static_cast<int32_t>(std::stoll(str));
        }
    };

    template <>
    struct __LexicalCastFrom<uint16_t> {
        inline static uint16_t Get(const std::string& str) {
            return static_cast<uint16_t>(std::stoull(str));
        }
    };

    template <>
    struct __LexicalCastFrom<uint32_t> {
        inline static uint32_t Get(const std::string& str) {
            return static_cast<uint32_t>(std::stoull(str));
        }
    };

    template <>
    struct __LexicalCastFrom<uint64_t> {
        inline static uint32_t Get(const std::string& str) {
            return static_cast<uint32_t>(std::stoull(str));
        }
    };

    template <>
    struct __LexicalCastFrom<float> {
        inline static float Get(const std::string& str) {
            return std::stof(str);
        }
    };

    template <>
    struct __LexicalCastFrom<glm::vec2> {
        inline static glm::vec2 Get(const std::string& str) {
            std::string space_delimiter = ",";

            size_t start = 0;
            size_t end = str.find(space_delimiter);

            auto x = __LexicalCastFrom<float>::Get(str.substr(start, end - start));

            start = end + space_delimiter.length();
            end = str.find(space_delimiter, start);

            auto y = __LexicalCastFrom<float>::Get(str.substr(start, end - start));

            return { x, y };
        }
    };

    template <>
    struct __LexicalCastFrom<glm::vec3> {
        inline static glm::vec3 Get(const std::string& str) {
            std::string space_delimiter = ",";

            size_t start = 0;
            size_t end = str.find(space_delimiter);

            auto x = __LexicalCastFrom<float>::Get(str.substr(start, end - start));

            start = end + space_delimiter.length();
            end = str.find(space_delimiter, start);

            auto y = __LexicalCastFrom<float>::Get(str.substr(start, end - start));

            start = end + space_delimiter.length();
            end = str.find(space_delimiter, start);

            auto z = __LexicalCastFrom<float>::Get(str.substr(start, end - start));

            return { x, y, z };
        }
    };

    template <class T>
    struct __LexicalCastTo {};

    template <>
    struct __LexicalCastTo<int32_t> {
        inline static std::string Get(int32_t value) {
            return std::string(std::to_string(value));
        }
    };

    template <>
    struct __LexicalCastTo<uint32_t> {
        inline static std::string Get(uint32_t value) {
            return std::string(std::to_string(value));
        }
    };

    template <>
    struct __LexicalCastTo<uint64_t> {
        inline static std::string Get(uint64_t value) {
            return std::string(std::to_string(value));
        }
    };

    template <class T>
    inline std::string LexicalCast(T&& arg) {
        return __LexicalCastTo<std::remove_reference_t<T>>::Get(std::forward<T>(arg));
    }

    template <class T>
    inline T LexicalCast(const std::string& str) {
        return __LexicalCastFrom<T>::Get(str);
    }

    class DataAdapter {
    public:

        explicit DataAdapter() {}

        explicit DataAdapter(const std::vector<std::string>& types)
            : _infos(types.size()) {
            uint32_t offset = 0;

            for (uint32_t i = 0; i < types.size(); ++i) {
                std::pair<std::string, std::string> typePair;
                if (Split(types[i], ":", typePair) > 1) {
                    _variables.try_emplace(typePair.first, i);
                    _infos[i].type = typePair.second;
                }
                else {
                    _infos[i].type = types[i];
                }

                offset = (offset % AlignOf(_infos[i].type)) == 0 ? offset : ((offset / AlignOf(_infos[i].type)) + 1) * AlignOf(_infos[i].type);
                _infos[i].offset = offset;
                offset += SizeOf(_infos[i].type);
            }

            _sizeof = offset;
        }

        template <class T>
        void InsertValueAt(void* data, uint32_t index, uint32_t variable, const T& value) const {
            auto offset = index * SizeOf() + Offset(variable);
            memcpy(reinterpret_cast<uint8_t*>(data) + offset, &value, sizeof(T));
        }

        bool FindVariableIndex(const std::string& name, uint32_t* index) {
            auto it = _variables.find(name);
            if (it == _variables.end()) {
                return false;
            }
            if (index != nullptr) {
                *index = it->second;
            }
            return true;
        }

        uint32_t SizeOf() const {
            return _sizeof;
        }

        uint32_t VariablesCount() const {
            return static_cast<uint32_t>(_infos.size());
        }

        const std::string& Type(uint32_t variable) const {
            return _infos[variable].type;
        }

        uint32_t Offset(uint32_t variable) const {
            return _infos[variable].offset;
        }

        static uint32_t SizeOf(const std::string type) {
            switch (Hash(type)) {
            case Hash("uint"): return 4;
            case Hash("vec2"): return 8;
            case Hash("vec3"): return 16;
            case Hash("mat3"): return 48;
            }
            return 0;
        }

        static uint32_t AlignOf(const std::string type) {
            switch (Hash(type)) {
            case Hash("uint"): return 4;
            case Hash("vec2"): return 8;
            case Hash("vec3"): return 16;
            case Hash("mat3"): return 16;
            }
            return 0;
        }

    private:

        struct DataInfo {
            std::string type;
            uint32_t offset;
        };

        std::vector<DataInfo> _infos;
        std::unordered_map<std::string, uint32_t> _variables;
        uint32_t _sizeof = 0;
    }; 

    class BaseType {
    public:
        enum Enum : uint8_t
        {
            Uint,
            Float,
            Vec2,
            Vec3,
            Mat3,
        };

        BaseType() = default;
        constexpr BaseType(Enum value) : _value(value) { }

        constexpr operator Enum() const { return _value; }
        explicit operator bool() const = delete;

        constexpr friend bool operator==(const BaseType& lhs, const BaseType& rhs) { return lhs._value == rhs._value; }
        constexpr friend bool operator!=(const BaseType& lhs, const BaseType& rhs) { return lhs._value != rhs._value; }

        constexpr static BaseType FromString(const char* str, BaseType def = {}) {
            switch (Hash(str)) {
            case Hash("uint"): return Uint;
            case Hash("float"): return Float;
            case Hash("vec2"): return Vec2;
            case Hash("vec3"): return Vec3;
            case Hash("mat3"): return Mat3;
            default: return def;
            }
        }

        constexpr uint32_t SizeOf() const {
            switch (_value) {
            case Uint: return 4;
            case Float: return 4;
            case Vec2: return 8;
            case Vec3: return 16;
            case Mat3: return 48;
            default: return 0;
            }
        }

        constexpr uint32_t AlignOf() const {
            switch (_value) {
            case Uint: return 4;
            case Float: return 4;
            case Vec2: return 8;
            case Vec3: return 16;
            case Mat3: return 16;
            default: return 0;
            }
        }

    private:
        Enum _value;
    };
    
    template <class T = BaseType>
    class Metadata {
    public:

        explicit Metadata() {}

        explicit Metadata(const std::vector<T>& types)
            : _types(types) {}

        friend bool operator==(const Metadata<T>& lhs, const Metadata<T>& rhs) {
            return lhs._types == rhs._types;
        }
        friend bool operator!=(const Metadata<T>& lhs, const Metadata<T>& rhs) {
            return lhs._types != rhs._types;
        }

        template <class V>
        void InsertValueAt(void* data, uint32_t index, uint32_t variable, const V& value) const {
            auto offset = index * SizeOf() + Offset(variable);
            memcpy(reinterpret_cast<uint8_t*>(data) + offset, &value, sizeof(V));
        }

        uint32_t Count() const {
            return static_cast<uint32_t>(_types.size());
        }

        T Type(uint32_t variable) const {
            return _types[variable];
        }

        uint32_t Offset(uint32_t variable) const {
            uint32_t offset = 0;

            for (uint32_t i = 1; i <= variable; ++i) {
                offset += _types[i - 1].SizeOf();
                auto alignMask = _types[i].AlignOf() - 1;
                offset = (offset + alignMask) & ~alignMask;
            }

            return offset;
        }

        uint32_t SizeOf() const {
            auto&& count = Count();
            if (count == 0) {
                return 0;
            }

            auto&& lastIndex = count - 1;
            return Offset(lastIndex) + _types[lastIndex].SizeOf();
        }

    private:

        std::vector<T> _types;
    };

    template <class T = BaseType>
    class NamedMetadata : public Metadata<T> {
    public:

        NamedMetadata() = default;
        NamedMetadata(const std::unordered_map<std::string, uint32_t>& names,
            const std::vector<T> types)
            : Metadata<T>(types), _names(names) {}

        template <class V>
        bool InsertValueAt(void* data, uint32_t index, const std::string& name, const V& value) const {
            auto it = _names.find(name);
            if (it == _names.end()) {
                return false;
            }
            Metadata<T>::template InsertValueAt<V>(data, index, it->second, value);
            return true;
        }

    private:

        std::unordered_map<std::string, uint32_t> _names;
    };

    class Spinlock {
    public:

        Spinlock() = default;

        // TODO just to accepting to use it in vector
        //Spinlock(const Spinlock& lock) {}

        void lock() {
            while (_flag.test_and_set());
        }

        void unlock() {
            _flag.clear();
        }
    
    private:
        std::atomic_flag _flag = ATOMIC_FLAG_INIT;
    };



    template <size_t N>
    struct BezieHelper {

        static FPoint2D calculateArray(float t, const std::array<FPoint2D, N>& points) {
            return calculateSeq(t, std::make_index_sequence<N>{}, points);
        }

        template <size_t...Is>
        static FPoint2D calculateSeq(float t, std::index_sequence<Is...>, const std::array<FPoint2D, N>& points) {
            return calculate(t, points[Is]...);
        }

        template <class... FPoint2Ds>
        static FPoint2D calculate(float t, FPoint2Ds... points) {
            static_assert(sizeof...(FPoint2Ds) == N);

            return (1.f - t) * firstPart(std::make_index_sequence<N - 1>{}, t, std::forward_as_tuple(std::forward<FPoint2Ds>(points)...))
                + t * lastPart(t, std::forward<FPoint2Ds>(points)...);
        }

        template <size_t...Is, class... FPoint2Ds>
        static FPoint2D firstPart(std::index_sequence<Is...>, float t, const std::tuple<FPoint2Ds...>& points) {
            static_assert(sizeof...(Is) == N - 1);
            static_assert(sizeof...(FPoint2Ds) == N);
            return BezieHelper<N - 1>::calculate(t, std::get<Is>(points)...);
        }

        template <class... FPoint2Ds>
        static FPoint2D lastPart(float t, const FPoint2D&, FPoint2Ds... points) {
            static_assert(sizeof...(FPoint2Ds) == N - 1);
            return BezieHelper<N - 1>::calculate(t, std::forward<FPoint2Ds>(points)...);
        }
    };

    template <>
    struct BezieHelper<1> {
        static FPoint2D calculate(float t, const FPoint2D& value) {
            return value;
        }
    };

    template <>
    struct BezieHelper<0> {};

    template <size_t N>
    class Bezie {
    public:
        Bezie(const std::array<FPoint2D, N>& points)
            : _points(points) {}
        template <class... Points>
        Bezie(Points&&... points)
            : _points({ std::forward<Points>(points)... }) {
            static_assert(sizeof...(Points) == N);
        }

        FPoint2D calculate(float t) {
            return BezieHelper<N>::calculateArray(t, _points);
        }

    private:
        std::array<FPoint2D, N> _points;
    };


    template <class T>
    struct Dirty {
    private:
        struct RAIICheckValue {
            RAIICheckValue(Dirty& dirty_) : dirty(dirty_) {
                value = dirty.dirty && dirty.current != dirty.fresh && dirty.fresh.has_value();
            }
            ~RAIICheckValue() {
                if (_ready) {
                    if (value) {
                        dirty.current = dirty.fresh;
                    }
                    dirty.dirty = false;
                }
            }

            operator bool() const {
                return value;
            }

            T& operator *() const {
                return dirty.fresh.value();
            }

            T* operator ->() const {
                return &dirty.fresh.value();
            }

            void ready() {
                _ready = true;
            }

            bool value;
            bool _ready = false;
            Dirty& dirty;
        };

    public:

        void FlagDirty() {
            dirty = true;
        }

        template <class Arg>
        void operator=(Arg&& arg) {
            fresh.emplace(std::forward<Arg>(arg));
            FlagDirty();
        }

        void Reset() {
            fresh.reset();
            FlagDirty();
        }

        void CurrentReset() {
            current.reset();
            FlagDirty();
        }

        void FullReset() {
            current.reset();
            fresh.reset();
            FlagDirty();
        }

        RAIICheckValue CheckValue() {
            return { *this };
        }

        bool HasValue() const {
            return current.has_value();
        }
        const T& Value() const {
            return current.value();
        }

        bool HasFreshValue() const {
            return fresh.has_value();
        }
        const T& FreshValue() const {
            return fresh.value();
        }

    private:
        std::optional<T> current;
        std::optional<T> fresh;
        bool dirty = false;

    };

}

#endif /* utils_h */