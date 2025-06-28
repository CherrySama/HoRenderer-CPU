/*
    Created by Yinghao He on 2025-05-16
*/
#include "Integrator.hpp"
#include "Material.hpp"


Integrator::~Integrator()
{
    Clean();
}

void Integrator::RenderImage(Camera &cam, Scene &world, Sampler &sampler, int sample_index)
{
    sampler.SetCurrentSample(sample_index);
    omp_set_num_threads(num_threads);

   #pragma omp parallel
    {
        Sampler thread_sampler = sampler;
        thread_sampler.SetCurrentSample(sample_index);
        // #pragma omp for schedule(dynamic, 4)
        #pragma omp for schedule(guided)
        for (int j = 0; j < height; ++j)
        {
            for (int i = 0; i < width; i++) {
                thread_sampler.SetPixel(i, j);
                Vector2f offset = thread_sampler.sample_square();
                Ray r = cam.GenerateRay(i, j, thread_sampler, offset);
                Vector3f pixel_color = ray_color(r, max_bounce, world, thread_sampler);

                write_color(i, j, pixel_color);  
            }
        }
    }
}

void Integrator::write_color(int u, int v, const Vector3f &color)
{
    int offset = v * width * 4 + u * 4;
    Vector3f tone_mapped = ACESFilmicToneMapping(color);
    Vector3f srgb_color = LinearToSRGB(tone_mapped);

    __m128 c = _mm_set_ps(1.0f, srgb_color.b, srgb_color.g, srgb_color.r); // RGBA
    __m128 zero = _mm_setzero_ps();
    __m128 one = _mm_set1_ps(1.0f);
    c = _mm_max_ps(c, zero); // clamp to [0, 1]
    c = _mm_min_ps(c, one);
    _mm_store_ps(float_pixels.get() + offset, c);
}

Vector3f Integrator::ray_color(const Ray &r, int bounce, const Hittable &world, Sampler &sampler)
{
    if (bounce <= 0)
        return Vector3f(0, 0, 0);

    Hit_Payload rec;
    if (!world.isHit(r, Vector2f(0.0f, Infinity), rec)) {
        return Vector3f(0.7f, 0.8f, 0.9f); 
    }

    Vector3f emission = rec.mat->Emit(r, rec, rec.uv.x, rec.uv.y);

    Vector3f scatter_direction;
    float pdf;
    Vector3f brdf = rec.mat->Sample(r, rec, scatter_direction, pdf, sampler);
    if (pdf <= Epsilon) 
        return emission;

    Ray scattered = Ray::SpawnRay(rec.p, scatter_direction, rec.normal);

    Vector3f surface_normal = rec.normal;
    float cos_theta = glm::max(0.0f, glm::dot(surface_normal, glm::normalize(scatter_direction)));
    Vector3f attenuation = brdf * cos_theta / pdf;
    
    Vector3f scatter = attenuation * ray_color(scattered, bounce-1, world, sampler);

    return emission + scatter;
}

void Integrator::SetNumThreads(int threads)
{
    num_threads = threads;
}

int Integrator::GetNumThreads() const
{
    return num_threads;
}

const float *Integrator::GetFloatPixels() const 
{
    return float_pixels.get();    
}

void Integrator::Clean()
{
    // delete[] pixels;
    float_pixels.reset();
}

