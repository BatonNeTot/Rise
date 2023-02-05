//☀Rise☀
#ifndef rise_types_h
#define rise_types_h

#include "utils/data.h"

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

#include <cinttypes>

namespace Rise {

	struct Size2D {
		Size2D() = default;
		Size2D(uint32_t width_, uint32_t height_)
			: width(width_), height(height_) {}
		Size2D(const VkExtent2D& extent)
			: width(extent.width), height(extent.height) {}

		static const Size2D zero;

		uint32_t width = 0;
		uint32_t height = 0;

		friend bool operator ==(const Size2D& lhs, const Size2D& rhs) {
			return lhs.width == rhs.width && lhs.height == rhs.height;
		}
		friend bool operator !=(const Size2D& lhs, const Size2D& rhs) {
			return lhs.width != rhs.width || lhs.height != rhs.height;
		}

		friend Size2D operator +(const Size2D& lhs, const Size2D& rhs) {
			return { lhs.width + rhs.width, lhs.height + rhs.height };
		}
		friend Size2D operator -(const Size2D& lhs, const Size2D& rhs) {
			return { lhs.width - rhs.width, lhs.height - rhs.height };
		}

		friend Size2D operator*(const Size2D& size, int x) {
			return { size.width * x, size.height * x };
		}
		friend Size2D operator/(const Size2D& size, int x) {
			return { size.width / x, size.height / x };
		}

		friend Size2D operator*(const Size2D& size, float x) {
			return { static_cast<uint32_t>(size.width * x), static_cast<uint32_t>(size.height * x) };
		}
		friend Size2D operator/(const Size2D& size, float x) {
			return { static_cast<uint32_t>(size.width / x), static_cast<uint32_t>(size.height / x) };
		}

		operator glm::ivec2() const {
			return { width, height };
		}
		operator glm::vec2() const {
			return { width, height };
		}

		operator VkExtent2D() const {
			return { width, height };
		}
	};

	struct FSize2D {
		FSize2D() = default;
		FSize2D(float width_, float height_)
			: width(width_), height(height_) {}

		float width = 0;
		float height = 0;

		friend bool operator ==(const FSize2D& lhs, const FSize2D& rhs) {
			return lhs.width == rhs.width && lhs.height == rhs.height;
		}

		friend bool operator !=(const FSize2D& lhs, const FSize2D& rhs) {
			return lhs.width != rhs.width || lhs.height != rhs.height;
		}

		FSize2D operator*(int x) const {
			return { width * x, height * x };
		}
		FSize2D operator/(int x) const {
			return { width / x, height / x };
		}

		operator glm::vec2() const {
			return { width, height };
		}
	};

	struct Point2D {
		int32_t x = 0;
		int32_t y = 0;

		static const Point2D zero;

		friend bool operator ==(const Point2D& lhs, const Point2D& rhs) {
			return lhs.x == rhs.x && lhs.y == rhs.y;
		}

		friend bool operator !=(const Point2D& lhs, const Point2D& rhs) {
			return lhs.x != rhs.x || lhs.y != rhs.y;
		}

		friend Point2D& operator +=(Point2D& lhs, const Point2D& rhs) {
			lhs.x += rhs.x;
			lhs.y += rhs.y;
			return lhs;
		}

		friend Point2D operator +(const Point2D& lhs, const Point2D& rhs) {
			return { lhs.x + rhs.x, lhs.y + rhs.y };
		}

		friend Point2D operator *(const Point2D& point, float x) {
			return { static_cast<int32_t>(point.x * x), static_cast<int32_t>(point.y * x) };
		}

		operator glm::ivec2() const {
			return { x, y };
		}
		operator glm::vec2() const {
			return { x, y };
		}

		operator VkOffset2D() const {
			return { x, y };
		}
	};

	struct FPoint2D {
		float x = 0;
		float y = 0;

		friend bool operator ==(const FPoint2D& lhs, const FPoint2D& rhs) {
			return lhs.x == rhs.x && lhs.y == rhs.y;
		}

		friend bool operator !=(const FPoint2D& lhs, const FPoint2D& rhs) {
			return lhs.x != rhs.x || lhs.y != rhs.y;
		}

		friend FPoint2D operator*(float value, const FPoint2D& point) {
			return { value * point.x, value * point.y };
		}

		friend FPoint2D operator+(const FPoint2D& lhs, const FPoint2D& rhs) {
			return { lhs.x + rhs.x, lhs.y + rhs.y };
		}

		operator glm::vec2() const {
			return { x, y };
		}
	};

	inline Point2D operator+(const Point2D& lhs, const Size2D& rhs) {
		return { lhs.x + static_cast<int32_t>(rhs.width), lhs.y + static_cast<int32_t>(rhs.height) };
	}

	inline Point2D operator+(const Size2D& lhs, const Point2D& rhs) {
		return { rhs.x + static_cast<int32_t>(lhs.width), rhs.y + static_cast<int32_t>(lhs.height) };
	}

	struct Square2D {
		Point2D pos;
		Size2D size;

		friend bool operator ==(const Square2D& lhs, const Square2D& rhs) {
			return lhs.pos == rhs.pos && lhs.size == rhs.size;
		}

		friend bool operator !=(const Square2D& lhs, const Square2D& rhs) {
			return lhs.pos != rhs.pos || lhs.size != rhs.size;
		}

		bool isInside(const Point2D& point) {
			return
				point.x > pos.x && point.x < static_cast<int64_t>(pos.x + size.width) &&
				point.y > pos.y && point.y < static_cast<int64_t>(pos.y + size.height);
		}
	};

	struct Padding2D {
		uint32_t left;
		uint32_t right;
		uint32_t top;
		uint32_t bottom;

		Padding2D() = default;
		Padding2D(uint32_t left_, uint32_t right_, uint32_t top_, uint32_t bottom_) :
			left(left_),
			right(right_),
			top(top_),
			bottom(bottom_) {}
		Padding2D(const Data& data) {
			auto padding = data["padding"].as(0u);
			left = data["left"].as(padding);
			right = data["right"].as(padding);
			top = data["top"].as(padding);
			bottom = data["bottom"].as(padding);
		}

		Padding2D friend operator *(const Padding2D& padding, float value) {
			return {
				static_cast<uint32_t>(padding.left * value),
				static_cast<uint32_t>(padding.right * value),
				static_cast<uint32_t>(padding.top * value),
				static_cast<uint32_t>(padding.bottom * value)
			};
		}
	};

	struct ColorRGB {
		uint8_t r = 1;
		uint8_t g = 1;
		uint8_t b = 1;

		ColorRGB() = default;
		ColorRGB(uint8_t ir, uint8_t ig, uint8_t ib)
			: r(ir), g(ig), b(ib) {}
		ColorRGB(const Data& data)
			: r(data[0]), g(data[1]), b(data[2]) {}

		operator glm::vec3() const {
			return { r / 255.f, g / 255.f, b / 255.f };
		}
	};
}

#endif /* rise_types_h */
