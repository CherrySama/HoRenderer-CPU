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
                Vector3f pixel_color = ray_color_v2(r, max_bounce, world, thread_sampler);

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

Vector3f Integrator::ray_color_v2(const Ray &r, int bounce, const Scene &world, Sampler &sampler)
{
    if (bounce <= 0)
        return Vector3f(0.0f);

    Vector3f total_radiance(0.0f);
    Vector3f path_throughput(1.0f);
    Ray current_ray = r;
    bool never_scattered = true;

    Hit_Payload last_hit;
    bool has_last_hit = false;

    for (int path_length = 0; path_length < max_bounce && path_length < bounce; path_length++) {
        // get current medium
        int current_medium_id = world.GetCurrentMediumId(current_ray, has_last_hit ? &last_hit : nullptr);
        auto medium = world.GetMedium(current_medium_id);

        // Scene intersection
        Hit_Payload surface_hit;
        bool hit_surface = world.isHit(current_ray, Vector2f(Epsilon, Infinity), surface_hit);
        float t_surface = hit_surface ? surface_hit.t : Infinity;

        // Event judgement
        bool will_scatter = false;
        float t_scatter = Infinity;
        int sampled_channel = 0;
        Vector3f channel_pdfs(0.0f);

        if (medium) {
            t_scatter = medium->SampleDistance(current_ray, t_surface, sampler, sampled_channel, channel_pdfs);
            will_scatter = (t_scatter < t_surface);
        }

        if (will_scatter) {
            //  Volume Scattering
            Vector3f scatter_pos = current_ray.at(t_scatter);

            // Get medium properties at scattering point
            Vector3f sigma_t = medium->GetSigmaT(scatter_pos);
            Vector3f sigma_s = medium->GetSigmaS(scatter_pos);
            
            // Calculate unbiased transmittance and PDF
            Vector3f transmittance = Vector3f(std::exp(-sigma_t.x * t_scatter),
                                              std::exp(-sigma_t.y * t_scatter),
                                              std::exp(-sigma_t.z * t_scatter));
            
            // Use average PDF from one-sample MIS for unbiased estimation
            float avg_pdf = (channel_pdfs.x + channel_pdfs.y + channel_pdfs.z) / 3.0f;
            
            if (avg_pdf > Epsilon) {
                path_throughput *= transmittance * sigma_s / avg_pdf;
                never_scattered = false;

                // The next event estimate calculates the transmittance to the scattering point
                const auto &lights = world.GetLights();
                if (!lights.empty()) {
                    Hit_Payload temp_hit;
                    temp_hit.p = scatter_pos;
                    temp_hit.normal = Vector3f(0, 1, 0);
                    temp_hit.front_face = true;
                    temp_hit.mat = nullptr; // No material in the volume

                    Vector3f light_direction;
                    float light_pdf;
                    Vector3f light_radiance = world.SampleLightEnvironment(current_ray, temp_hit, light_direction, light_pdf, sampler);

                    if (light_pdf > Epsilon && glm::length(light_radiance) > Epsilon) {
                        Ray shadow_ray = Ray::SpawnRay(scatter_pos, light_direction, Vector3f(0, 1, 0));
                        Vector3f shadow_transmittance = CalculateShadowTransmittance(shadow_ray, world);

                        if (glm::length(shadow_transmittance) > Epsilon) {
                            auto phase_func = medium->GetPhaseFunction();
                            float phase_value = phase_func->Evaluate(-current_ray.direction(), light_direction);
                            float phase_pdf = phase_func->Pdf(-current_ray.direction(), light_direction);
                            float mis_weight = PowerHeuristic(light_pdf, phase_pdf);

                            Vector3f direct_contrib = path_throughput * phase_value * light_radiance * shadow_transmittance * mis_weight / light_pdf;
                            total_radiance += direct_contrib;
                        }
                    }
                }

                // Phase function sampling
                Vector3f new_direction;
                float phase_pdf;
                Vector2f phase_sample = sampler.get_2d_sample();
                auto phase_func = medium->GetPhaseFunction();
                phase_func->Sample(-current_ray.direction(), phase_sample, new_direction, phase_pdf);

                if (phase_pdf > Epsilon) {
                    float phase_value = phase_func->Evaluate(-current_ray.direction(), new_direction);
                    path_throughput *= phase_value / phase_pdf;
                    current_ray = Ray::SpawnRay(scatter_pos, new_direction, Vector3f(0, 1, 0));
                } else {
                    break;
                }
            } else {
                break; // Invalid PDF
            }

        } else if (hit_surface) {
            // Calculate the transmittance to the surface
            if (medium) {
                Vector3f ray_start = current_ray.origin();
                Vector3f surface_pos = current_ray.at(surface_hit.t);
                Vector3f transmittance = medium->Transmittance(ray_start, surface_pos);
                path_throughput *= transmittance;
            }

            // Update medium ID
            current_medium_id = world.UpdateMediumId(current_ray, surface_hit, current_medium_id);

            // Check if there is a material
            if (!surface_hit.mat) {
                current_ray = Ray(surface_hit.p + Epsilon * current_ray.direction(), current_ray.direction());
                last_hit = surface_hit;
                has_last_hit = true;
                continue; // continue next loop
            }

            Vector3f emission = surface_hit.mat->Emit(current_ray, surface_hit, surface_hit.uv.x, surface_hit.uv.y);
            if (glm::length(emission) > Epsilon) {
                if (never_scattered) {
                    total_radiance += path_throughput * emission;
                } else {
                    total_radiance += path_throughput * emission;
                }
            }

            // non-emissive materials
            if (!surface_hit.mat->IsEmit()) {
                // Direct lighting sampling
                Vector3f surface_direct_lighting = EstimateDirectLighting(current_ray, surface_hit, world, sampler);
                total_radiance += path_throughput * surface_direct_lighting;

                // BSDF sampling
                Vector3f scatter_direction;
                float bsdf_pdf;
                Vector3f brdf = surface_hit.mat->Sample(current_ray, surface_hit, scatter_direction, bsdf_pdf, sampler);

                if (bsdf_pdf > Epsilon) {
                    bool is_transmission = (glm::dot(scatter_direction, surface_hit.normal) * glm::dot(-current_ray.direction(), surface_hit.normal)) < 0;
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
                    never_scattered = false;
                } else {
                    break;
                }
            } else {
                break; // Hit the light source, the path ends
            }

        } else {
            break; // Light escape
        }

        // Russian Roulette
        if (path_length >= 3) {
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

        Vector3f shadow_transmittance = CalculateShadowTransmittance(shadow_ray, world);
        
        if (glm::length(shadow_transmittance) > Epsilon) {
            float brdf_pdf;
            Vector3f brdf = rec.mat->Evaluate(r_in, rec, light_direction, brdf_pdf);
            if (brdf_pdf > Epsilon) {
                float mis_weight = PowerHeuristic(light_pdf, brdf_pdf);
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
    float p1 = std::pow(pdf1, beta);
    float p2 = std::pow(pdf2, beta);
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
            current_ray = Ray(hit.p + Epsilon * current_ray.direction(), current_ray.direction());
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

