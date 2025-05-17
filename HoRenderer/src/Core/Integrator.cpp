/*
    Created by Yinghao He on 2025-05-16
*/
#include "Integrator.hpp"

Integrator::Integrator(int width, int height)
{
    this->width = width;
    this->height = height;
    pixels = new uint8_t[width * height * 4];;
}

Integrator::~Integrator()
{
    delete[] pixels;
}

void Integrator::RenderImage()
{
    // 创建图像数据
    // uint8_t* pixels = new uint8_t[width * height * 4];
    
    // 生成渐变图像
     for (int j = 0; j < height; ++j) {
        uint8_t* row = pixels + j * width * 4;  // 指向行开始的指针
        std::clog << "\rScanlines remaining: " << (height - j) << ' ' << std::flush;
        for (int i = 0; i < width; i++) {
            auto r = double(i) / (width-1);
            auto g = double(j) / (height-1);
            auto b = 0.0;

            int ir = int(255.999 * r);
            int ig = int(255.999 * g);
            int ib = int(255.999 * b);

            uint8_t* pixel = row + i * 4;  // 当前像素的指针
            pixel[0] = ir;  // R
            pixel[1] = ig;  // G
            pixel[2] = ib;  // B
            pixel[3] = 255; // A
        }
    }
    std::clog << "\rDone.                 \n";
}

uint8_t *Integrator::GetPixels()
{
    return pixels;
}
