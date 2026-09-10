/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include "Util.hpp"
#include "Ray.hpp"
#include "Scene.hpp"
#include "Camera.hpp"


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
    void write_radiance(int u, int v, const Vector3f &radiance);
    Vector3f VolumeIntegrator(const Ray &r, int max_depth, const Scene &world, Sampler &sampler, int initial_medium_id);
    Vector3f EstimateDirectLighting(const Ray &r_in, const Hit_Payload &rec, const Scene &world, Sampler &sampler, int current_medium_id);
    float PowerHeuristic(float pdf1, float pdf2, int beta = 2);
    Vector3f CalculateShadowTransmittance(const Ray &shadow_ray, const Scene &world, Sampler& sampler, int initial_medium_id, const Hittable* target_light_shape);

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
