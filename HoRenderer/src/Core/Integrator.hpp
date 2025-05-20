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


class Integrator{
public:
    Integrator(int width, int height) : width(width), height(height), pixels(std::make_unique<uint8_t[]>(width * height * 4)) {}
    ~Integrator();

    // void RenderImage();
    void RenderImage(Camera &cam, Scene &world);
    const uint8_t *GetPixels() const;

    // Calculate ray color (background)
    Vector3f ray_color(const Ray &r, const Hittable &world);

    void Clean();

private:
    int width, height;
    std::unique_ptr<uint8_t[]> pixels;
};