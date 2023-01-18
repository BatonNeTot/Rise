//☀Rise☀
#ifndef data_h
#define data_h

#include <json/json.hpp>

#include <istream>

namespace Rise {

    class Data {
    public:

        class It {
        public:

            using iterator_category = nlohmann::json::iterator::iterator_category;
            using value_type = Data;
            using difference_type = nlohmann::json::iterator::difference_type;
            using pointer = Data*;
            using reference = Data;

            reference operator*() {
                return Data(_it.value());
            }

            It& operator++() {
                ++_it;
                return *this;
            }

            It operator++(int) {
                It it(_it);
                ++_it;
                return it;
            }

            friend bool operator!= (const It& lhs, const It& rhs) {
                return lhs._it != rhs._it;
            }

        private:
            friend Data;

            It(const nlohmann::json::iterator& it)
                : _it(it) {}

            nlohmann::json::iterator _it;
        };

        Data() = default;

        friend std::istream& operator>>(std::istream& stream, Data& data) {
            return stream >> data._json;
        }

        Data operator[](const char* key) const {
            auto it = _json.find(key);
            return it != _json.end() ? Data(it.value()) : Data{};
        }

        Data operator[](const std::string& key) const {
            auto it = _json.find(key);
            return it != _json.end() ? Data(it.value()) : Data{};
        }

        Data operator[](uint32_t index) const {
            return _json.size() > index ? Data(_json[index]) : Data{};
        }
        Data operator[](int32_t index) const {
            return static_cast<int64_t>(_json.size()) > index ? Data(_json[index]) : Data{};
        }

        Data operator[](uint64_t index) const {
            return _json.size() > index ? Data(_json[index]) : Data{};
        }
        Data operator[](int64_t index) const {
            return static_cast<int64_t>(_json.size()) > index ? Data(_json[index]) : Data{};
        }

        template <class T>
        T as() const {
            return _json.is_null() ? T() : _json.get<T>();
        }

        template <class T>
        T as(const T& def) const {
            return _json.is_null() ? def : _json.get<T>();
        }

        template <class T>
        operator T() const {
            return as<T>();
        }

        size_t size() const {
            return _json.size();
        }

        bool empty() const {
            return _json.empty();
        }

        It begin() {
            return It(_json.begin());
        }

        It end() {
            return It(_json.end());
        }

        bool isString() const {
            return _json.is_string();
        }

        bool isArray() const {
            return _json.is_array();
        }

    private:

        Data(const nlohmann::json& json) 
            : _json(json) {}

        nlohmann::json _json;
    };

}

#endif /* data_h */
