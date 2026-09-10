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
                Vector3f pixel_color = VolumeIntegrator(r, max_bounce, world, thread_sampler, cam.GetMediumId());

                write_radiance(i, j, pixel_color);
            }
        }
    }
}

void Integrator::write_radiance(int u, int v, const Vector3f &radiance)
{
    int offset = v * width * 4 + u * 4;

    auto sanitize_radiance = [](float value) {
        return std::isfinite(value) ? std::max(value, 0.0f) : 0.0f;
    };
    float_pixels[offset + 0] = sanitize_radiance(radiance.r);
    float_pixels[offset + 1] = sanitize_radiance(radiance.g);
    float_pixels[offset + 2] = sanitize_radiance(radiance.b);
    float_pixels[offset + 3] = 1.0f;
}

Vector3f Integrator::VolumeIntegrator(const Ray &r, int max_depth, const Scene &world, Sampler &sampler, int initial_medium_id)
{
    if (max_depth <= 0)
        return Vector3f(0.0f);

    Vector3f total_radiance(0.0f);
    Vector3f path_throughput(1.0f);
    Ray current_ray = r;
    bool never_scattered = true;
    bool last_event_was_delta = false;
    float last_pdf = 0.0f;
    Vector3f last_scatter_position = r.origin();
    int current_medium_id = initial_medium_id;

    auto HitLight = [](const Hit_Payload& hit) -> bool {
        return hit.mat && hit.mat->IsEmit();
    };

    auto HitMediumBoundary = [](const Hit_Payload &hit) -> bool {
        return hit.interior_medium_id != hit.exterior_medium_id;
    };

    for (int bounce = 0; bounce < max_depth; bounce++) {
        Hit_Payload surface_hit;
        bool hit_surface = world.isHit(current_ray, Vector2f(Epsilon, Infinity), surface_hit);
        float t_surface = hit_surface ? surface_hit.t : Infinity;

        auto medium = world.GetMedium(current_medium_id);

        MediumSample medium_sample;
        if (medium) {
            medium_sample = medium->Sample(current_ray, t_surface, sampler);
        }

        if (medium_sample.scattered) {
            const Vector3f scatter_pos = medium_sample.position;
            path_throughput *= medium_sample.weight;
            const float max_throughput = std::max({path_throughput.x,
                                                   path_throughput.y,
                                                   path_throughput.z});
            if (max_throughput <= Epsilon || !std::isfinite(max_throughput)) {
                break;
            }
            never_scattered = false;
            
            // NEE
            Hit_Payload temp_hit;
            temp_hit.p = scatter_pos;
            temp_hit.normal = Vector3f(0, 1, 0);
            temp_hit.geometric_normal = temp_hit.normal;
            temp_hit.front_face = true;
            temp_hit.mat = nullptr;
            
            Vector3f light_direction;
            float light_pdf;
            const Hittable* sampled_light_shape = nullptr;
            Vector3f light_radiance = world.SampleLights(current_ray, temp_hit, light_direction, light_pdf, sampler, sampled_light_shape);
            
            if (light_pdf > Epsilon) {
                // A volume interaction has no surface normal to offset along.
                Ray shadow_ray(scatter_pos, glm::normalize(light_direction));
                Vector3f shadow_transmittance = CalculateShadowTransmittance(shadow_ray, world, sampler, current_medium_id, sampled_light_shape);
                
                auto phase_func = medium->GetPhaseFunction();
                float phase_value = phase_func->Evaluate(-current_ray.direction(), light_direction);
                float phase_pdf = phase_func->Pdf(-current_ray.direction(), light_direction);
                if (phase_pdf > Epsilon && glm::length(shadow_transmittance) > Epsilon) {
                    float mis_weight = PowerHeuristic(light_pdf, phase_pdf, 2);
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
                current_ray = Ray(scatter_pos, glm::normalize(new_direction));
                last_scatter_position = scatter_pos;
                last_pdf = phase_pdf;
                last_event_was_delta = false;
            } else {
                break;
            }
        } else if (hit_surface) {
            path_throughput *= medium_sample.weight;
            if (HitLight(surface_hit)) {
                float mis_weight = 1.0f;
                Vector3f emission = surface_hit.mat->Emit(current_ray, surface_hit, surface_hit.uv.x, surface_hit.uv.y);
                if (!never_scattered && !last_event_was_delta) {
                    float light_pdf;
                    // Null boundaries move the ray origin, but do not move
                    // the scattering vertex used by the competing light PDF.
                    world.EvaluateLights(Ray(last_scatter_position, current_ray.direction()),
                                         surface_hit, light_pdf);

                    if (light_pdf > Epsilon)
                        mis_weight = PowerHeuristic(last_pdf, light_pdf, 2);
                }
                total_radiance += path_throughput * mis_weight * emission;
                break;
            } else if (HitMediumBoundary(surface_hit) && !surface_hit.mat) {
                current_medium_id = world.UpdateMediumId(current_ray, surface_hit, current_medium_id);
                current_ray = Ray::SpawnRay(surface_hit.p,
                                            current_ray.direction(),
                                            surface_hit.geometric_normal);
                bounce--; 
                continue;
            } else {
                // NEE
                Vector3f direct_lighting = EstimateDirectLighting(current_ray, surface_hit, world, sampler, current_medium_id);
                total_radiance += path_throughput * direct_lighting;

                // BSDF
                 Vector3f scatter_direction;
                float bsdf_pdf;
                Vector3f brdf = surface_hit.mat->Sample(current_ray, surface_hit, scatter_direction, bsdf_pdf, sampler);
                
                if (bsdf_pdf > Epsilon) {
                    const Vector3f view_direction = -glm::normalize(current_ray.direction());
                    if (!surface_hit.mat->IsScatteringDirectionValid(surface_hit,
                                                                     view_direction,
                                                                     scatter_direction)) {
                        break;
                    }
                    bool is_transmission = (glm::dot(scatter_direction, surface_hit.geometric_normal) *
                                          glm::dot(-current_ray.direction(), surface_hit.geometric_normal)) < 0;
                    
                    if (is_transmission) {
                        float cos_theta = std::abs(glm::dot(surface_hit.normal, scatter_direction));
                        path_throughput *= brdf * cos_theta / bsdf_pdf;
                        current_medium_id = world.UpdateMediumId(current_ray, surface_hit, current_medium_id);
                    } else {
                        float cos_theta = glm::dot(surface_hit.normal, scatter_direction);
                        path_throughput *= brdf * cos_theta / bsdf_pdf;
                    }

                    current_ray = Ray::SpawnRay(surface_hit.p,
                                                scatter_direction,
                                                surface_hit.geometric_normal);
                    last_pdf = bsdf_pdf;
                    last_scatter_position = surface_hit.p;
                    last_event_was_delta = surface_hit.mat->IsDelta(surface_hit);
                    never_scattered = false;
                } else {
                    break;
                }
            }
        } else {
            float mis_weight = 1.0f;
            if (!never_scattered && !last_event_was_delta) {
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
            // In nearly conservative smoke, a fixed 0.95 cap repeatedly
            // amplifies rare long paths. Let volume paths survive with their
            // actual throughput; max_depth still bounds every path.
            float survival_prob = std::min(medium_sample.scattered ? 1.0f : 0.95f, max_component);

            if (survival_prob <= Epsilon || !std::isfinite(survival_prob)) {
                break;
            }
            if (sampler.random_float() > survival_prob) {
                break;
            }
            path_throughput /= survival_prob;
        }
    }

    return total_radiance;
}

Vector3f Integrator::EstimateDirectLighting(const Ray &r_in, const Hit_Payload &rec, const Scene &world, Sampler &sampler, int current_medium_id)
{
    Vector3f direct_lighting(0.0f); 
    Vector3f V = -glm::normalize(r_in.direction());

    // light sampling
    Vector3f light_direction;
    float light_pdf;
    const Hittable* sampled_light_shape = nullptr;
    Vector3f light_radiance = world.SampleLights(r_in, rec, light_direction, light_pdf, sampler, sampled_light_shape);

    if (light_pdf > Epsilon) {
        if (!rec.mat->IsScatteringDirectionValid(rec, V, light_direction)) {
            return direct_lighting;
        }
        bool is_light_transmission = glm::dot(light_direction, rec.geometric_normal) *
                                     glm::dot(V, rec.geometric_normal) < 0;
        Ray shadow_ray = Ray::SpawnRay(rec.p, light_direction, rec.geometric_normal);

        int shadow_medium_id = is_light_transmission
            ? world.UpdateMediumId(r_in, rec, current_medium_id)
            : current_medium_id;
        Vector3f shadow_transmittance = CalculateShadowTransmittance(shadow_ray, world, sampler, shadow_medium_id, sampled_light_shape);
        
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
    if (!std::isfinite(pdf1) || pdf1 < 0.0f) {
        pdf1 = 0.0f;
    }
    if (!std::isfinite(pdf2) || pdf2 < 0.0f) {
        pdf2 = 0.0f;
    }

    pdf1 = std::max(pdf1, 1e-10f);
    pdf2 = std::max(pdf2, 1e-10f);
    
    float p1 = std::pow(pdf1, beta);
    float p2 = std::pow(pdf2, beta);
    if (std::isinf(p1)) return 1.0f;
    if (std::isinf(p2)) return 0.0f;
    
    return p1 / (p1 + p2);
}

// Shadow transmittance calculation
Vector3f Integrator::CalculateShadowTransmittance(const Ray &shadow_ray,
                                                  const Scene &world,
                                                  Sampler& sampler,
                                                  int initial_medium_id,
                                                  const Hittable* target_light_shape)
{
    Vector3f transmittance(1.0f);
    Ray current_ray = shadow_ray;
    int medium_id = initial_medium_id;

    for (int bounce = 0; bounce < 32; bounce++) {
        Hit_Payload hit;
        bool hit_surface = world.isHit(current_ray, Vector2f(Epsilon, Infinity), hit);

        if (!hit_surface)
            return target_light_shape ? Vector3f(0.0f) : transmittance;

        // Calculate the current segment's transmittance.
        auto medium = world.GetMedium(medium_id);

        if (medium) {
            transmittance *= medium->Transmittance(current_ray, hit.t, sampler);

            if (glm::length(transmittance) < 1e-6f) 
                return Vector3f(0.0f); // Early withdrawal
        }

        // Check what was hit
        if (!hit.mat) {
            // There is no material. It might be an index-matching surface. Continue propagation.
            medium_id = world.UpdateMediumId(current_ray, hit, medium_id);
            current_ray = Ray::SpawnRay(hit.p, current_ray.direction(), hit.geometric_normal);
            continue;
        }
        if (hit.mat->IsEmit()) {
            return hit.hit_object == target_light_shape
                ? transmittance
                : Vector3f(0.0f);
        } else {
            // When hitting an opaque surface, the light is blocked.
            return Vector3f(0.0f);
        }
    }

    return Vector3f(0.0f);
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
