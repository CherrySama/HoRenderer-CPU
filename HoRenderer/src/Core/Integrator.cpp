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

Vector3f Integrator::ray_color(const Ray &r, int bounce, const Scene &world, Sampler &sampler)
{
    if (bounce <= 0)
        return Vector3f(0, 0, 0);

    Hit_Payload rec;
    if (!world.isHit(r, Vector2f(0.0f, Infinity), rec)) {
        return Vector3f(0.7f, 0.8f, 0.9f); 
    }

    // emission
    Vector3f total_radiance = rec.mat->Emit(r, rec, rec.uv.x, rec.uv.y);

    // light sampling
    Vector3f direct_lighting = EstimateDirectLighting(r, rec, world, sampler);
    total_radiance += direct_lighting;

    // BRDF sampling
    Vector3f scatter_direction;
    float pdf;
    Vector3f brdf = rec.mat->Sample(r, rec, scatter_direction, pdf, sampler);
    if (pdf > Epsilon && bounce > 1) {
        Vector3f surface_normal = rec.normal;
        if (glm::dot(scatter_direction, rec.normal) < 0.0f) 
            surface_normal = -rec.normal;

        Ray scattered = Ray::SpawnRay(rec.p, scatter_direction, surface_normal);
        Vector3f attenuation;
        if (rec.mat->IsDelta()) {
            attenuation = brdf;
        } else if (rec.mat->IsVolumetric()) {
            attenuation = brdf / pdf;
        } else {
            float cos_theta = std::abs(glm::dot(rec.normal, glm::normalize(scatter_direction)));
            attenuation = brdf * cos_theta / pdf;
        }
        total_radiance += attenuation * ray_color(scattered, bounce-1, world, sampler);
    }

    return total_radiance;
}

Vector3f Integrator::EstimateDirectLighting(const Ray &r_in, const Hit_Payload &rec, const Scene &world, Sampler &sampler)
{
    Vector3f direct_lighting(0.0f);
    if (rec.mat->IsDelta() || rec.mat->IsVolumetric()) {
        return direct_lighting;
    }
    
    const auto& lights = world.GetLights();
    if (lights.empty()) {
        return direct_lighting;
    }

    Vector3f V = -glm::normalize(r_in.direction());

    // light sampling
    Vector3f light_direction;
    float light_pdf;
    Vector3f light_radiance = world.SampleLightEnvironment(r_in, rec, light_direction, light_pdf, sampler);
    
    if (light_pdf > Epsilon && glm::dot(rec.normal, light_direction) > 0.0f) {
        // BRDF Evaluate
        float brdf_pdf;
        Vector3f brdf = rec.mat->Evaluate(r_in, rec, light_direction, brdf_pdf);
        
        if (brdf_pdf > Epsilon) {
            float cos_theta = glm::dot(rec.normal, light_direction);
            float mis_weight = PowerHeuristic(light_pdf, brdf_pdf);
            direct_lighting += mis_weight * brdf * cos_theta * light_radiance / light_pdf;
        }
    }

    // BRDF sampling
    Vector3f scatter_direction;
    float brdf_pdf;
    Vector3f brdf = rec.mat->Sample(r_in, rec, scatter_direction, brdf_pdf, sampler);
    
    if (brdf_pdf > Epsilon && glm::dot(rec.normal, scatter_direction) > 0.0f) {
        Vector3f surface_normal = rec.normal;
        if (glm::dot(scatter_direction, rec.normal) < 0.0f) 
            surface_normal = -rec.normal;

        Ray light_ray = Ray::SpawnRay(rec.p, scatter_direction, surface_normal);
        Hit_Payload light_rec;
        
        if (world.isHit(light_ray, Vector2f(Epsilon, Infinity), light_rec)) {
            if (light_rec.mat && light_rec.mat->IsEmit()) {
                float light_eval_pdf;
                Vector3f light_emission = world.EvaluateLight(light_ray, light_rec, light_eval_pdf);
                
                if (light_eval_pdf > Epsilon) {
                    float cos_theta = glm::dot(rec.normal, scatter_direction);
                    float mis_weight = PowerHeuristic(brdf_pdf, light_eval_pdf);
                    direct_lighting += mis_weight * brdf * cos_theta * light_emission / brdf_pdf;
                }
            }
        }
    }

    return direct_lighting;
}

float Integrator::PowerHeuristic(float pdf1, float pdf2, int beta)
{
    float p1 = std::pow(pdf1, beta);
    float p2 = std::pow(pdf2, beta);
    return p1 / (p1 + p2);
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

