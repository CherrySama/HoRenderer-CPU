/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "Scene.hpp"
#include "Camera.hpp"

struct IntegratorParams {
    int num_threads;
    int max_bounce;
};

class Integrator{
public:
    Integrator(int width, int height, int threads = 16, int bounce = 10) :
        width(width), height(height),
        float_pixels(std::make_unique<float[]>(width * height * 4)),
        num_threads(threads), max_bounce(bounce) {
        std::fill(float_pixels.get(), float_pixels.get() + width * height * 4, 0.0f);
    }
    ~Integrator();

    void RenderImage(Camera &cam, Scene &world, Sampler &sampler, int sample_index);
    void write_color(int u, int v, const Vector3f &color);
    Vector3f ray_color(const Ray &r, int bounce, const Scene &world, Sampler &sampler);
    
    void SetNumThreads(int threads);
    int GetNumThreads() const;

    const float* GetFloatPixels() const;
    void Clean();

private:
    int width, height;
    std::unique_ptr<float[]> float_pixels;
    int num_threads;
    int max_bounce;
};