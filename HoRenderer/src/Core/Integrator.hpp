/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "Scene.hpp"
#include "Hittable.hpp"
#include "Shape.hpp"
#include "Camera.hpp"
#include "Sampler.hpp"
#include "../Common/ProgressTracker.hpp"


class Integrator{
public:
    Integrator(int width, int height) : width(width), height(height), 
                pixels(std::make_unique<uint8_t[]>(width * height * 4)),
                num_threads(omp_get_max_threads()) {}
    ~Integrator();

    void RenderImage(Camera &cam, Scene &world, Sampler &sampler);
    void write_color(int u, int v, const Vector3f &color);
    // Calculate ray color (background)
    Vector3f ray_color(const Ray &r, const Hittable &world);
    
    // Set the number of threads
    void SetNumThreads(int threads);
    // Get the current number of threads
    int GetNumThreads() const;

    const uint8_t *GetPixels() const;
    void Clean();

private:
    int width, height;
    std::unique_ptr<uint8_t[]> pixels;
    ProgressTracker progress;
    int num_threads;
};