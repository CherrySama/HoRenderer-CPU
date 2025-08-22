/*
    Created by Yinghao He on 2025-05-16
*/
#include "Integrator.hpp"
#include "Material.hpp"
#include "Medium.hpp"


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
                Vector3f pixel_color = VolumeIntegrator(r, max_bounce, world, thread_sampler);

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

Vector3f Integrator::VolumeIntegrator(const Ray &r, int bounce, const Scene &world, Sampler &sampler)
{
    if (bounce <= 0)
        return Vector3f(0.0f);

    Vector3f total_radiance(0.0f);
    Vector3f path_throughput(1.0f);
    Ray current_ray = r;
    bool never_scattered = true;
    float last_pdf = 0.0f;
    Hit_Payload last_hit;
    bool has_last_hit = false;

    auto HitLight = [](const Hit_Payload& hit) -> bool {
        return hit.mat && hit.mat->IsEmit();
    };

    auto HitMediumBoundary = [](const Hit_Payload &hit) -> bool {
        return hit.interior_medium_id != hit.exterior_medium_id;
    };

    for (int bounce = 0; bounce < max_bounce; bounce++) {
        float cumulative_trans_pdf = 1.0f;
        Hit_Payload surface_hit;
        bool hit_surface = world.isHit(current_ray, Vector2f(Epsilon, Infinity), surface_hit);
        float t_surface = hit_surface ? surface_hit.t : Infinity;

        int current_medium_id = world.GetCurrentMediumId(current_ray, has_last_hit ? &last_hit : nullptr);
        auto medium = world.GetMedium(current_medium_id);

        bool will_scatter = false;
        float t_scatter = Infinity;
        Vector3f transmittance(1.0f);
        float trans_pdf = 1.0f;

        if (medium) {
            Vector3f sigma_t = medium->GetSigmaT(current_ray.origin());
            float max_sigma_t = std::max({sigma_t.x, sigma_t.y, sigma_t.z});
            float u = sampler.random_float();
            t_scatter = -std::log(1.0f - u) / max_sigma_t;
            will_scatter = (t_scatter < t_surface);
            
            if (will_scatter) {
                transmittance = Vector3f(std::exp(-sigma_t.x * t_scatter),
                                       std::exp(-sigma_t.y * t_scatter),
                                       std::exp(-sigma_t.z * t_scatter));
                trans_pdf = max_sigma_t * std::exp(-max_sigma_t * t_scatter);
            } else {
                transmittance = Vector3f(std::exp(-sigma_t.x * t_surface),
                                       std::exp(-sigma_t.y * t_surface),
                                       std::exp(-sigma_t.z * t_surface));
                trans_pdf = std::exp(-max_sigma_t * t_surface);
            }
            cumulative_trans_pdf *= trans_pdf;
        }

        if (will_scatter) {
            Vector3f scatter_pos = current_ray.at(t_scatter);
            Vector3f sigma_s = medium->GetSigmaS(scatter_pos);
            
            path_throughput *= transmittance * sigma_s / trans_pdf;
            never_scattered = false;
            
            // NEE
            Hit_Payload temp_hit;
            temp_hit.p = scatter_pos;
            temp_hit.normal = Vector3f(0, 1, 0);
            temp_hit.front_face = true;
            temp_hit.mat = nullptr;
            
            Vector3f light_direction;
            float light_pdf;
            Vector3f light_radiance = world.SampleLights(current_ray, temp_hit, light_direction, light_pdf, sampler);
            
            if (light_pdf > Epsilon) {
                Ray shadow_ray = Ray::SpawnRay(scatter_pos, light_direction, Vector3f(0, 1, 0));
                Vector3f shadow_transmittance = CalculateShadowTransmittance(shadow_ray, world);
                
                auto phase_func = medium->GetPhaseFunction();
                float phase_value = phase_func->Evaluate(-current_ray.direction(), light_direction);
                float phase_pdf = phase_func->Pdf(-current_ray.direction(), light_direction);
                float effective_phase_pdf = phase_pdf * cumulative_trans_pdf;
                if (phase_pdf > Epsilon && glm::length(shadow_transmittance) > Epsilon) {
                    float mis_weight = PowerHeuristic(light_pdf, effective_phase_pdf, 2);
                    total_radiance += path_throughput * mis_weight * phase_value * 
                                    light_radiance * shadow_transmittance / light_pdf;
                }
            }
            
            // phase func sampling
            Vector3f new_direction;
            float phase_pdf;
            Vector2f phase_sample = sampler.get_2d_sample();
            auto phase_func = medium->GetPhaseFunction();
            phase_func->Sample(-current_ray.direction(), phase_sample, new_direction, phase_pdf);
            
            if (phase_pdf > Epsilon) {
                float phase_value = phase_func->Evaluate(-current_ray.direction(), new_direction);
                path_throughput *= phase_value / phase_pdf;
                current_ray = Ray::SpawnRay(scatter_pos, new_direction, Vector3f(0, 1, 0));
                last_pdf = phase_pdf * cumulative_trans_pdf;
            } else {
                break;
            }
        } else if (hit_surface) {
            path_throughput *= transmittance / trans_pdf;
            current_medium_id = world.UpdateMediumId(current_ray, surface_hit, current_medium_id);
            if (HitLight(surface_hit)) {
                float mis_weight = 1.0f;
                Vector3f emission = surface_hit.mat->Emit(current_ray, surface_hit, surface_hit.uv.x, surface_hit.uv.y);
                if (!never_scattered) {
                    float light_pdf;
                    world.EvaluateLights(current_ray, surface_hit, light_pdf);

                    if (light_pdf > Epsilon)
                        mis_weight = PowerHeuristic(last_pdf, light_pdf, 2);
                }
                total_radiance += path_throughput * mis_weight * emission;
                break;
            } else if (HitMediumBoundary(surface_hit)) {
                current_medium_id = world.UpdateMediumId(current_ray, surface_hit, current_medium_id);
                bool entering = glm::dot(current_ray.direction(), surface_hit.normal) < 0;
                Vector3f offset_normal = entering ? surface_hit.normal : -surface_hit.normal;
                current_ray = Ray::SpawnRay(surface_hit.p, current_ray.direction(), offset_normal);
                last_hit = surface_hit;
                has_last_hit = true;
                bounce--; 
                continue;
            } else {
                // NEE
                Vector3f direct_lighting = EstimateDirectLighting(current_ray, surface_hit, world, sampler, cumulative_trans_pdf);
                total_radiance += path_throughput * direct_lighting;

                // BSDF
                 Vector3f scatter_direction;
                float bsdf_pdf;
                Vector3f brdf = surface_hit.mat->Sample(current_ray, surface_hit, scatter_direction, bsdf_pdf, sampler);
                
                if (bsdf_pdf > Epsilon) {
                    bool is_transmission = (glm::dot(scatter_direction, surface_hit.normal) * 
                                          glm::dot(-current_ray.direction(), surface_hit.normal)) < 0;
                    Vector3f surface_normal = is_transmission ? -surface_hit.normal : surface_hit.normal;
                    
                    if (is_transmission) {
                        float cos_theta = std::abs(glm::dot(surface_hit.normal, scatter_direction));
                        path_throughput *= brdf * cos_theta / bsdf_pdf;
                    } else {
                        float cos_theta = glm::dot(surface_hit.normal, scatter_direction);
                        path_throughput *= brdf * cos_theta / bsdf_pdf;
                    }

                    current_ray = Ray::SpawnRay(surface_hit.p, scatter_direction, surface_normal);
                    last_hit = surface_hit;
                    has_last_hit = true;
                    last_pdf = bsdf_pdf * cumulative_trans_pdf;
                    never_scattered = false;
                } else {
                    break;
                }
            }
        } else {
            float mis_weight = 1.0f;
            if (!never_scattered) {
                float env_pdf = 0.0f;
                world.EvaluateEnvLight(current_ray, env_pdf);

                if (medium && env_pdf > Epsilon) {
                    mis_weight = PowerHeuristic(last_pdf, env_pdf);
                } else if (env_pdf > Epsilon) {
                    mis_weight = PowerHeuristic(last_pdf, env_pdf);
                }
            }
            
            Vector3f background = world.SampleEnvLight(current_ray);
            total_radiance += path_throughput * mis_weight * background;
            break;
        }

        if (bounce >= 3) {
            float max_component = std::max({path_throughput.x, path_throughput.y, path_throughput.z});
            float survival_prob = std::min(0.95f, max_component);
            
            if (sampler.random_float() > survival_prob) {
                break;
            }
            path_throughput /= survival_prob;
        }
    }

    return total_radiance;
}

Vector3f Integrator::EstimateDirectLighting(const Ray &r_in, const Hit_Payload &rec, const Scene &world, Sampler &sampler, float cumulative_trans_pdf)
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
    Vector3f light_radiance = world.SampleLights(r_in, rec, light_direction, light_pdf, sampler);

    if (light_pdf > Epsilon) {
        bool is_light_transmission = glm::dot(light_direction, rec.normal) * glm::dot(V, rec.normal) < 0;
        Vector3f shadow_normal = is_light_transmission ? -rec.normal : rec.normal;
        Ray shadow_ray = Ray::SpawnRay(rec.p, light_direction, shadow_normal);

        Vector3f shadow_transmittance = CalculateShadowTransmittance(shadow_ray, world);
        
        if (glm::length(shadow_transmittance) > Epsilon) {
            float brdf_pdf;
            Vector3f brdf = rec.mat->Evaluate(r_in, rec, light_direction, brdf_pdf);
            if (brdf_pdf > Epsilon) {
                float effective_brdf_pdf = brdf_pdf * cumulative_trans_pdf;
                float mis_weight = PowerHeuristic(light_pdf, effective_brdf_pdf);
                Vector3f light_contrib;
                if (is_light_transmission) {
                    float cos_theta = std::abs(glm::dot(rec.normal, light_direction));
                    light_contrib = mis_weight * brdf * cos_theta * light_radiance * shadow_transmittance / light_pdf;
                } else {
                    float cos_theta = glm::dot(rec.normal, light_direction);
                    light_contrib = mis_weight * brdf * cos_theta * light_radiance * shadow_transmittance / light_pdf;
                }

                direct_lighting += light_contrib;
            }
        }        
    }

    return direct_lighting;
}

float Integrator::PowerHeuristic(float pdf1, float pdf2, int beta)
{
    pdf1 = std::max(pdf1, 1e-10f);
    pdf2 = std::max(pdf2, 1e-10f);
    
    float p1 = std::pow(pdf1, beta);
    float p2 = std::pow(pdf2, beta);
    if (std::isinf(p1)) return 1.0f;
    if (std::isinf(p2)) return 0.0f;
    
    return p1 / (p1 + p2);
}

// Shadow transmittance calculation
Vector3f Integrator::CalculateShadowTransmittance(const Ray &shadow_ray, const Scene &world)
{
    Vector3f transmittance(1.0f);
    Ray current_ray = shadow_ray;

    for (int bounce = 0; bounce < 32; bounce++) {
        Hit_Payload hit;
        bool hit_surface = world.isHit(current_ray, Vector2f(Epsilon, Infinity), hit);

        if (!hit_surface) 
            break; // reach infinity

        // Calculate the current segment's transmittance.
        int medium_id = world.GetCurrentMediumId(current_ray, nullptr);
        auto medium = world.GetMedium(medium_id);

        if (medium) {
            Vector3f start_pos = current_ray.origin();
            Vector3f end_pos = current_ray.at(hit.t);
            transmittance *= medium->Transmittance(start_pos, end_pos);

            if (glm::length(transmittance) < 1e-6f) 
                return Vector3f(0.0f); // Early withdrawal
        }

        // Check what was hit
        if (!hit.mat) {
            // There is no material. It might be an index-matching surface. Continue propagation.
            Vector3f offset_normal = glm::dot(current_ray.direction(), hit.normal) > 0 ? hit.normal : -hit.normal;
            current_ray = Ray::SpawnRay(hit.p, current_ray.direction(), offset_normal);
            continue;
        }
        if (hit.mat->IsEmit()) {
            // Hit the light source. Transmittance calculation is complete.
            break;
        } else {
            // When hitting an opaque surface, the light is blocked.
            return Vector3f(0.0f);
        }
    }

    return transmittance;
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

