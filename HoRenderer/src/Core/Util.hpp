/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include <iostream>
#include <algorithm>
#include <memory>
#include <cstdlib>
#include <random>
#include <cmath>
#include <omp.h>
#include <immintrin.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <vector>
#include <sstream>
#include <queue>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


class FileManager;
class Integrator;
class Renderer;
class RenderPass;
class Shader;
class Ray;
class Hittable;
class Hit_Payload;
class Scene;
class AliasTable1D;
class Camera;
class Sampler;
class ProgressTracker;

class Sphere;
class Quad;
class Box;

class Material;
class Diffuse;
class Conductor;
class Plastic;
class HomogeneousMedium;
class IsotropicPhase;
class HenyeyGreensteinPhase;
class Emission;
class FrostedGlass;
class Glass;

class AABB;
class BVHnode;
class Filter;
class UniformFilter;
class GaussianFilter;
class TentFilter;

class Texture;
class SolidTexture;
class ImageTexture;
class HDRTexture;

class Translate;
class Rotate;
class Scale;

class Light;
class QuadAreaLight;
class SphereAreaLight;
class InfiniteAreaLight;


using Vector2u = glm::uvec2;
using Vector2i = glm::ivec2;
using Vector2f = glm::vec2;
using Vector2d = glm::dvec2;
using Vector3u = glm::uvec3;
using Vector3i = glm::ivec3;
using Vector3f = glm::vec3;
using Vector3d = glm::dvec3;
using Vector4u = glm::uvec4;
using Vector4i = glm::ivec4;
using Vector4f = glm::vec4;
using Vector4d = glm::dvec4;
using Matrix3f = glm::mat3x3;
using Matrix4f = glm::mat4x4;


constexpr float Epsilon = 1e-5f;
constexpr float Infinity = std::numeric_limits<float>::infinity();
constexpr float PI = 3.1415926535897932385f;
constexpr float INV_PI = 1.0f / PI;
constexpr float INV_2PI = 1.0f / (2.0f * PI);
constexpr float INV_4PI = 1.0f / (4.0f * PI);


inline uint32_t FloatToBits(float f) {
	uint32_t ui;
	memcpy(&ui, &f, sizeof(float));

	return ui;
}

inline float BitsToFloat(uint32_t ui) {
	float f;
	memcpy(&f, &ui, sizeof(uint32_t));

	return f;
}

inline float NextFloatUp(float v) {
	// Handle infinity and negative zero for _NextFloatUp()_
	if (std::isinf(v) && v > 0.0f) {
		return v;
	}
	if (v == -0.0f) {
		v = 0.0f;
	}

	// Advance _v_ to next higher float
	uint32_t ui = FloatToBits(v);
	if (v >= 0) {
		++ui;
	}
	else {
		--ui;
	}

	return BitsToFloat(ui);
}

inline float NextFloatDown(float v) {
	// Handle infinity and positive zero for _NextFloatDown()_
	if (std::isinf(v) && v < 0.0f) {
		return v;
	}
	if (v == 0.0f) {
		v = -0.0f;
	}
	uint32_t ui = FloatToBits(v);
	if (v > 0) {
		--ui;
	}
	else {
		++ui;
	}

	return BitsToFloat(ui);
}

inline float degrees_to_radians(float degrees) {
    return degrees * PI / 180.0;
}

inline bool isInInterval(Vector2f interval, float x) {
	return interval.x < x && x < interval.y;
}

inline Vector2f IntervalExpand(Vector2f interval, float delta) {
    float padding = delta / 2.0f;
    return Vector2f(interval.x - padding, interval.y + padding);
}

inline float LinearToSRGB(float linear) {
    if (linear <= 0.0031308f) {
        return 12.92f * linear;
    } else {
        return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    }
}

inline Vector3f LinearToSRGB(const Vector3f& linear) {
    return Vector3f(LinearToSRGB(linear.r),
                    LinearToSRGB(linear.g),
                    LinearToSRGB(linear.b));
}

inline float SRGBToLinear(float srgb) {
    if (srgb <= 0.04045f) {
        return srgb / 12.92f;
    } else {
        return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
    }
}

inline Vector3f SRGBToLinear(const Vector3f& srgb) {
    return Vector3f(SRGBToLinear(srgb.r),
                    SRGBToLinear(srgb.g),
                    SRGBToLinear(srgb.b));
}

inline Vector3f ACESFilmicToneMapping(const Vector3f& color) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    
    Vector3f numerator = color * (a * color + Vector3f(b));
    Vector3f denominator = color * (c * color + Vector3f(d)) + Vector3f(e);
    
    return glm::clamp(numerator / denominator, 0.0f, 1.0f);
}

inline uint32_t hash_pixel(int x, int y) {
    uint32_t h = static_cast<uint32_t>(x);
    h ^= static_cast<uint32_t>(y) << 16;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

inline uint32_t hash_ray(const Vector3f& origin, const Vector3f& direction) {
    uint32_t h = 0;
    h ^= hash_pixel(FloatToBits(origin.x), FloatToBits(origin.y));
    h ^= hash_pixel(FloatToBits(origin.z), FloatToBits(direction.x));
    h ^= hash_pixel(FloatToBits(direction.y), FloatToBits(direction.z));
    return h;
}

inline Vector3f ToLocal(const Vector3f& dir, const Vector3f& up) {
	auto B = Vector3f(0.0f), C = Vector3f(0.0f);
	if (std::abs(up.x) > std::abs(up.y)) {
		float len_inv = 1.0f / std::sqrt(up.x * up.x + up.z * up.z);
		C = Vector3f(up.z * len_inv, 0.0f, -up.x * len_inv);
	}
	else {
		float len_inv = 1.0f / std::sqrt(up.y * up.y + up.z * up.z);
		C = Vector3f(0.0f, up.z * len_inv, -up.y * len_inv);
	}
	B = glm::cross(C, up);

	return Vector3f(glm::dot(dir, B), glm::dot(dir, C), glm::dot(dir, up));
}

inline Vector3f ToWorld(const Vector3f& dir, const Vector3f& up) {
	auto B = Vector3f(0.0f), C = Vector3f(0.0f);
	if (std::abs(up.x) > std::abs(up.y)) {
		float len_inv = 1.0f / std::sqrt(up.x * up.x + up.z * up.z);
		C = Vector3f(up.z * len_inv, 0.0f, -up.x * len_inv);
	}
	else {
		float len_inv = 1.0f / std::sqrt(up.y * up.y + up.z * up.z);
		C = Vector3f(0.0f, up.z * len_inv, -up.y * len_inv);
	}
	B = glm::cross(C, up);

	return glm::normalize(dir.x * B + dir.y * C + dir.z * up);
}
