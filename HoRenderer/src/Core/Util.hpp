/*
	Created by Yinghao He on 2025-05-15
*/
#pragma once

#include <iostream>
#include <memory>
#include <cstdlib>
#include <random>
#include <cmath>
#include <omp.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <vector>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class FileManager;
class Integrator;
class Renderer;
class RenderPass;
class Shader;
class Ray;
class Hittable;
class Hit_Payload;
class Scene;
class Sphere;
class Quad;
class Box;
class Camera;
class Sampler;

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

inline bool isInInterval(Vector2f interval, float x)
{
	return interval.x < x && x < interval.y;
}

