/*
    Created by Yinghao He on 2025-05-16
*/
#pragma once

#include "Util.hpp"

class Integrator{
public:
    Integrator(int width, int height);
    ~Integrator();

    void RenderImage();
    uint8_t *GetPixels();

private:
    int width, height;
    uint8_t *pixels;
};