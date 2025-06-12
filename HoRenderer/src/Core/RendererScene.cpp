/*
    Created by Yinghao He on 2025-05-24
*/
#include "RendererScene.hpp"
#include "Shape.hpp"
#include "Material.hpp"
#include "Filter.hpp"
#include "../Common/FileManager.hpp"

namespace RendererScene
{    
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

        // Material
        FileManager *fm = FileManager::getInstance();
        fm->init();
        TextureParams imageParams;
        imageParams.type = TextureType::IMAGE;
        imageParams.filepath = fm->getTexturePath("earthmap.jpg");
        auto imageTexture = Texture::Create(imageParams);

        TextureParams solidParams;
        solidParams.type = TextureType::SOLIDCOLOR;
        solidParams.color = Vector3f(0.8f, 0.8f, 0.0f);
        auto solidTexture = Texture::Create(solidParams);

        MaterialParams mp;
        mp.type = MaterialType::LAMBERTIAN;
        mp.albedo_texture = solidTexture;
        auto material_ground = Material::Create(mp);
        mp.albedo_texture = imageTexture;
        auto material_center = Material::Create(mp);

        mp.type = MaterialType::DIELECTRIC;
        mp.refractive_index = 1.50f;
        auto material_left = Material::Create(mp);
        mp.refractive_index = 1.00f / 1.50f;
        auto material_bubble = Material::Create(mp);

        solidParams.color = Vector3f(0.8f, 0.6f, 0.2f);
        solidTexture = Texture::Create(solidParams);
        mp.type = MaterialType::METAL;
        mp.albedo_texture = solidTexture;
        mp.fuzz = 0.0f;
        auto material_right = Material::Create(mp);

        // Scene
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