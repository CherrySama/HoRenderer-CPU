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
    Clean();
}

void Integrator::RenderImage()
{    
    // 计算视口尺寸和相关参数
    float aspect_ratio = 16.0 / 9.0;
    int image_width = width;
    // Calculate the image height, and ensure that it's at least 1.
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;
    auto viewport_width = viewport_height * (float(image_width)/image_height);

    // 计算视口基向量
    auto viewport_u = Vector3f(viewport_width, 0, 0);
    auto viewport_v = Vector3f(0, -viewport_height, 0);

    // 计算像素间隔向量
    auto pixel_delta_u = viewport_u / float(image_width);
    auto pixel_delta_v = viewport_v / float(image_height);
    
    // 计算视口左上角位置
    auto viewport_upper_left = camera_center - Vector3f(0, 0, focal_length) - viewport_u/2.0f - viewport_v/2.0f;
    
    // 计算第一个像素的中心
    auto pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    // 加入两个球体
    world.Add(std::make_shared<Sphere>(Vector3f(0, 0,-1), 0.5));
    world.Add(std::make_shared<Sphere>(Vector3f(0,-100.5,-1), 100));
    // world.Add(std::make_shared<Box>(Vector3f(0.0f, 0.0f, -3.0f),      
    //                                 Vector3f(2.0f, 1.0f, 3.0f)));
    // world.Add(std::make_shared<Quad>(
    //                                 Vector3f(0, -2, 0), // 中心点位于y=-2平面
    //                                 Vector3f(0, 1, 0),  // 法向量向上
    //                                 Vector3f(0, 0, 1),  // 前向
    //                                 20.0f,              // 宽度20个单位
    //                                 20.0f));            // 长度20个单位
                            

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
            Vector3f color = ray_color(r, world);

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

Vector3f Integrator::ray_color(const Ray &r, const Hittable &world)
{
    // auto t = hit_sphere(Vector3f(0,0,-1), 0.5, r);
    // if (t > 0.0) {
    //     Vector3f N = glm::normalize(r.at(t) - Vector3f(0,0,-1));
    //     return 0.5f * Vector3f(N.x + 1, N.y + 1, N.z + 1);
    // }
    Hit_Record rec;
    if (world.isHit(r, 0, Infinity, rec)) {
        return 0.5f * (rec.normal + Vector3f(1,1,1));
    }
    
    // 获取射线的单位方向向量
    Vector3f unit_direction = glm::normalize(r.direction());
    // 基于y分量做线性插值，范围从0到1
    float color = 0.5 * (unit_direction.y + 1.0);
    // 从白色(1,1,1)到蓝色(0.5,0.7,1.0)的线性插值
    return (1.0f - color) * Vector3f(1.0, 1.0, 1.0) + color * Vector3f(0.5, 0.7, 1.0);
}

float Integrator::hit_sphere(const Vector3f &center, float radius, const Ray &r)
{
    Vector3f oc = center - r.origin();
    auto a = glm::dot(r.direction(), r.direction());
    // auto b = -2.0 * glm::dot(r.direction(), oc);
    // make b = -2h
    auto h = glm::dot(r.direction(), oc);
    auto c = glm::dot(oc, oc) - radius*radius;
    auto discriminant = h * h -  a * c;
    // return (discriminant >= 0);
    if (discriminant < 0)
    {
        return -1.0;
    }else{
        return (h - std::sqrt(discriminant)) / a; // Select the nearest intersection point smallest(t)
    }
}

void Integrator::Clean()
{
    delete[] pixels;
}
