/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "../Common/ProgressTracker.hpp"


class Integrator{
public:
    Integrator(int width, int height) :
        width(width), height(height),
        pixels(std::make_unique<uint8_t[]>(width * height * 4)),
        float_pixels(std::make_unique<float[]>(width * height * 4)),
        num_threads(omp_get_max_threads()), max_depth(10) {}
    ~Integrator();

    void RenderSingleSample(Camera &cam, Scene &world, Sampler &sampler, int sample_index);
    void write_color_float(int u, int v, const Vector3f &color);
    // Calculate ray color (background)
    Vector3f ray_color(const Ray &r, int depth, const Hittable &world, Sampler &sampler);
    
    void SetNumThreads(int threads);
    int GetNumThreads() const;

    const uint8_t *GetPixels() const;
    const float* GetFloatPixels() const;
    void Clean();

private:
    int width, height;
    std::unique_ptr<uint8_t[]> pixels;
    std::unique_ptr<float[]> float_pixels;
    ProgressTracker progress;
    int num_threads;
    int max_depth;
};