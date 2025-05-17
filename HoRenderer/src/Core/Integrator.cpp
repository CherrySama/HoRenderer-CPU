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
    // 计算视口尺寸和相关参数
    double aspect_ratio = double(width) / height;
    double viewport_width = viewport_height * aspect_ratio;
    
    // 计算视口基向量
    Vector3f viewport_u(viewport_width, 0, 0);
    Vector3f viewport_v(0, -viewport_height, 0);
    
    // 计算像素间隔向量
    Vector3f pixel_delta_u = viewport_u / float(width);
    Vector3f pixel_delta_v = viewport_v / float(height);
    
    // 计算视口左上角位置
    Vector3f viewport_upper_left = camera_center - Vector3f(0, 0, focal_length) - viewport_u/2.0f - viewport_v/2.0f;
    
    // 计算第一个像素的中心
    Vector3f pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    // Generate Image
     for (int j = 0; j < height; ++j) {
        uint8_t* row = pixels + j * width * 4;  // Pointer to the start of the line
        std::clog << "\rScanlines remaining: " << (height - j) << ' ' << std::flush;
        for (int i = 0; i < width; i++) {
            // auto r = double(i) / (width-1);
            // auto g = double(j) / (height-1);
            // auto b = 0.0;
            // 计算像素中心位置
            Vector3f pixel_center = pixel00_loc + (float(i) * pixel_delta_u) + (float(j) * pixel_delta_v);
            
            // 创建射线：从相机中心到像素
            Vector3f ray_direction = pixel_center - camera_center;
            Ray r(camera_center, ray_direction);
            
            // 获取射线颜色
            Vector3f color = ray_color(r);

            int ir = int(255.999 * color.r);
            int ig = int(255.999 * color.g);
            int ib = int(255.999 * color.b);

            uint8_t* pixel = row + i * 4;  // Pointer to the current pixel
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
