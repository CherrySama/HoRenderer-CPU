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
        return Vector3f(0.05f, 0.05f, 0.05f); 
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
        bool is_transmission = (glm::dot(scatter_direction, rec.normal) * glm::dot(-r.direction(), rec.normal)) < 0;
        Vector3f surface_normal = is_transmission ? -rec.normal : rec.normal;
        Ray scattered = Ray::SpawnRay(rec.p, scatter_direction, surface_normal);

        // check if it hits anything
        Hit_Payload light_rec;
        if (world.isHit(scattered, Vector2f(Epsilon, Infinity), light_rec)) {
            // Check if it hits a light source
            if (light_rec.mat && light_rec.mat->IsEmit()) {
                // Hitting the Light Source - Calculating Direct Lighting Contribution Using MIS
                float light_eval_pdf;
                Vector3f light_emission = world.EvaluateLight(scattered, light_rec, light_eval_pdf);

                if (light_eval_pdf > Epsilon && pdf > Epsilon) {
                    float mis_weight = PowerHeuristic(pdf, light_eval_pdf);

                    if (is_transmission) {
                        float cos_theta = std::abs(glm::dot(rec.normal, scatter_direction));
                        total_radiance += mis_weight * brdf * cos_theta * light_emission / pdf;
                    } else {
                        float cos_theta = glm::dot(rec.normal, scatter_direction);
                        total_radiance += mis_weight * brdf * cos_theta * light_emission / pdf;
                    }
                } // After hitting the light source, no longer recurse and end this path directly
            } else {
                // Hitting a non-light source - indirect lighting is handled recursively as normal
                Vector3f attenuation;
                if (is_transmission) {
                    attenuation = brdf / pdf;
                } else {
                    float cos_theta = glm::dot(rec.normal, scatter_direction);
                    attenuation = brdf * cos_theta / pdf;
                }

                // Russian Roulette
                int bounces_count = max_bounce - bounce;
                if (bounces_count > 3) {
                    float max_component = std::max({attenuation.x, attenuation.y, attenuation.z});
                    float survival_prob = std::min(0.95f, max_component);

                    if (sampler.random_float() > survival_prob) {
                        return total_radiance; // End Path
                    }
                    attenuation /= survival_prob;
                }

                // Only recurse if hitting a non-light source
                total_radiance += attenuation * ray_color(scattered, bounce - 1, world, sampler);
            }
        }
    }

    return total_radiance;
}

Vector3f Integrator::EstimateDirectLighting(const Ray &r_in, const Hit_Payload &rec, const Scene &world, Sampler &sampler)
{
    Vector3f direct_lighting(0.0f); 
    const auto& lights = world.GetLights();
    if (lights.empty()) {
        return direct_lighting;
    }

    Vector3f V = -glm::normalize(r_in.direction());

    // light sampling
    Vector3f light_direction;
    float light_pdf;
    Vector3f light_radiance = world.SampleLightEnvironment(r_in, rec, light_direction, light_pdf, sampler);

    if (light_pdf > Epsilon) {
        bool is_light_transmission = glm::dot(light_direction, rec.normal) * glm::dot(V, rec.normal) < 0;
        Vector3f shadow_normal = is_light_transmission ? -rec.normal : rec.normal;
        Ray shadow_ray = Ray::SpawnRay(rec.p, light_direction, shadow_normal);
        Hit_Payload shadow_rec;
        bool in_shadow = false;
        
        if (world.isHit(shadow_ray, Vector2f(Epsilon, Infinity), shadow_rec)) {
            if (!shadow_rec.mat || !shadow_rec.mat->IsEmit()) {
                in_shadow = true;
            }
        }

        if (!in_shadow) {
            float brdf_pdf;
            Vector3f brdf = rec.mat->Evaluate(r_in, rec, light_direction, brdf_pdf);
            
            if (brdf_pdf > Epsilon) {
                float mis_weight = PowerHeuristic(light_pdf, brdf_pdf);
                if (is_light_transmission) {
                    direct_lighting += mis_weight * brdf * light_radiance / light_pdf;
                } else {
                    float cos_theta = glm::dot(rec.normal, light_direction);
                    direct_lighting += mis_weight * brdf * cos_theta * light_radiance / light_pdf;
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

