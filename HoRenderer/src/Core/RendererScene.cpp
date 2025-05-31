/*
    Created by Yinghao He on 2025-05-24
*/
#include "RendererScene.hpp"
#include "Shape.hpp"
#include "Material.hpp"
#include "Filter.hpp"

namespace RendererScene
{
    std::shared_ptr<Renderer> RayTracingInOneWeekendCover()
    {
        // camera
        CameraParams camParams = { 16.0f / 9.0f,
                        1600,
                        20.0f,
                        Vector3f(13.0f, 2.0f, 3.0f),
                        Vector3f(0.0f, 0.0f, 0.0f),
                        Vector3f(0.0f, 1.0f, 0.0f),
                        0.6f,
                        10.0f};
    
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        // Integrator
        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height);

        // Sampler
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::GAUSSIAN);

        // Scene
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        auto ground_material = std::make_shared<Lambertian>(Vector3f(0.5f, 0.5f, 0.5f));
        scene->Add(std::make_shared<Quad>(Vector3f(-50.0f, 0.0f, -50.0f), 
                                                Vector3f(100.0f, 0.0f, 0.0f),   
                                                Vector3f(0.0f, 0.0f, 100.0f),   
                                                ground_material));              

        for (int a = -6; a < 6; a++) {
            for (int b = -6; b < 6; b++) {
                auto choose_mat = sampler->random_float();
                Vector3f center(a + 0.9f * sampler->random_float(),
                                0.2f,
                                b + 0.9f * sampler->random_float());

                if (glm::length(center - Vector3f(4.0f, 0.2f, 0.0f)) > 0.9f) {
                    std::shared_ptr<Material> sphere_material;

                    if (choose_mat < 0.8f) {
                        // diffuse
                        float r = sampler->random_float();
                        float g = sampler->random_float();  
                        float b = sampler->random_float();
                        Vector3f albedo = Vector3f(r * r, g * g, b * b);
                        sphere_material = std::make_shared<Lambertian>(albedo);
                        scene->Add(std::make_shared<Sphere>(center, 0.2f, sphere_material));
                    } else if (choose_mat < 0.95f) {
                        // metal
                        Vector3f albedo(sampler->random_float(0.5f, 1.0f),
                                        sampler->random_float(0.5f, 1.0f),
                                        sampler->random_float(0.5f, 1.0f));
                        float fuzz = sampler->random_float(0.0f, 0.5f);
                        sphere_material = std::make_shared<Metal>(albedo, fuzz);
                        scene->Add(std::make_shared<Sphere>(center, 0.2f, sphere_material));
                    } else {
                        // glass
                        sphere_material = std::make_shared<Dielectric>(1.5f);
                        scene->Add(std::make_shared<Sphere>(center, 0.2f, sphere_material));
                    }
                }
            }
        }
        auto material1 = std::make_shared<Dielectric>(1.5f);
        scene->Add(std::make_shared<Sphere>(Vector3f(0.0f, 1.0f, 0.0f), 1.0f, material1));

        auto material2 = std::make_shared<DiffuseBRDF>(Vector3f(0.4f, 0.2f, 0.1f));
        scene->Add(std::make_shared<Sphere>(Vector3f(-4.0f, 1.0f, 0.0f), 1.0f, material2));

        auto material3 = std::make_shared<Metal>(Vector3f(0.7f, 0.6f, 0.5f), 0.0f);
        scene->Add(std::make_shared<Sphere>(Vector3f(4.0f, 1.0f, 0.0f), 1.0f, material3));

        scene->BuildBVH();

        // Renderer
        auto renderer = std::make_shared<Renderer>(std::move(camera), std::move(integrator), std::move(sampler), std::move(scene));

        return renderer;
    }
    
    std::shared_ptr<Renderer> TestScene()
    {
        // camera
        CameraParams camParams = { 16.0f / 9.0f,
                        1600,
                        20.0f,
                        Vector3f(-2.0f, 2.0f, 1.0f),
                        Vector3f(0.5f, 0.0f, -2.0f),
                        Vector3f(0.0f, 1.0f, 0.0f),
                        0.0f,
                        2.0f};
    
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        // Integrator
        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height);

        // Sampler
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::TENT);

        // Scene
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        auto material_ground = std::make_shared<Lambertian>(Vector3f(0.8f, 0.8f, 0.0f));
        auto material_center = std::make_shared<Lambertian>(Vector3f(0.1f, 0.2f, 0.5f));
        auto material_left   = std::make_shared<Dielectric>(1.50f);
        auto material_bubble = std::make_shared<Dielectric>(1.00f / 1.50f);
        auto material_right  = std::make_shared<Metal>(Vector3f(0.8f, 0.6f, 0.2f), 0.0f);

        scene->Add(std::make_shared<Quad>(Vector3f(-50.0f, 0.0f, -50.0f), 
                                                Vector3f(100.0f, 0.0f, 0.0f),   
                                                Vector3f(0.0f, 0.0f, 100.0f),   
                                                material_ground));   

        scene->Add(std::make_shared<Sphere>(Vector3f(0.0f, 0.5f, -1.2f), 0.5f, material_center));
        scene->Add(std::make_shared<Sphere>(Vector3f(-1.0f, 0.5f, -1.0f), 0.5f, material_left));
        scene->Add(std::make_shared<Sphere>(Vector3f(-1.0f, 0.5f, -1.0f), 0.4f, material_bubble));
        scene->Add(std::make_shared<Sphere>(Vector3f(1.0f, 0.5f, -1.0f), 0.5f, material_right));

        // scene->BuildBVH();
        
        // Renderer
        auto renderer = std::make_shared<Renderer>(std::move(camera), std::move(integrator), std::move(sampler), std::move(scene));

        return renderer;
    }
}