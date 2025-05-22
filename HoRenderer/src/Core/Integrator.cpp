/*
    Created by Yinghao He on 2025-05-16
*/
#include "Integrator.hpp"

Integrator::~Integrator()
{
    Clean();
}

void Integrator::RenderImage(Camera &cam, Scene &world, Sampler &sampler)
{    
    // Set the number of threads for OpenMP
    omp_set_num_threads(num_threads);
    std::cout << "Rendering with " << num_threads << " threads..." << std::endl;
    progress.Initialize(height);
    const int samples_per_pixel = sampler.get_samples_per_pixel();

    #pragma omp parallel
    {   
        // #pragma omp for schedule(static, 16)
        for (int j = 0; j < height; ++j)
        {                  
            if (omp_get_thread_num() == 0) 
                progress.Update(1);
            
            for (int i = 0; i < width; i++) {
                Vector3f pixel_color(0, 0, 0);
                for (int s = 0; s < samples_per_pixel; s++)
                {
                    Vector2f offset = sampler.sample_square();
                    Ray r = cam.GenerateRay(i, j, offset);
                    pixel_color += ray_color(r, max_depth, world, sampler);
                }
                Vector3f color = sampler.scale_color(pixel_color);
                write_color(i, j, color);  
            }
        }
    }
    // std::clog << "\rDone.                 \n"; 
}

void Integrator::write_color(int u, int v, const Vector3f &color)
{
    int offset = v * width * 4 + u * 4;
    uint8_t* pixel = pixels.get() + offset;
    
    // int ir = int(255.99f * color.r);
    // int ig = int(255.99f * color.g);
    // int ib = int(255.99f * color.b);
    
    pixel[0] = static_cast<uint8_t>(255.99f * color.r);  // R
    pixel[1] = static_cast<uint8_t>(255.99f * color.g);  // G
    pixel[2] = static_cast<uint8_t>(255.99f * color.b);  // B
    pixel[3] = 255;                       // A
}

Vector3f Integrator::ray_color(const Ray &r, int depth, const Hittable &world, Sampler &sampler)
{
    if (depth <= 0)
        return Vector3f(0, 0, 0);

    Hit_Payload rec;
    if (world.isHit(r, Vector2f(0.0f, Infinity), rec)) {
        Ray scattered;
        Vector3f attenuation;

        // If the object has a material and can scatter light
        if (rec.mat && rec.mat->scatter(r, rec, attenuation, scattered, sampler))
            return attenuation * ray_color(scattered, depth-1, world, sampler);
        
        // If there is no material, use the normal color
        return 0.5f * (rec.normal + Vector3f(1,1,1));
    }
    
    // Background Color - Sky Gradient
    Vector3f unit_direction = glm::normalize(r.direction());
    float color = 0.5f * (unit_direction.y + 1.0f);
    return (1.0f - color) * Vector3f(1.0, 1.0, 1.0) + color * Vector3f(0.5, 0.7, 1.0);
}

void Integrator::SetNumThreads(int threads)
{
    num_threads = threads;
}

int Integrator::GetNumThreads() const
{
    return num_threads;
}

const uint8_t *Integrator::GetPixels() const
{
    return pixels.get();
}

void Integrator::Clean()
{
    // delete[] pixels;
    pixels.reset();
}

