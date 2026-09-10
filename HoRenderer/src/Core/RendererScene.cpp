/*
    Created by Yinghao He on 2025-05-24
*/
#include "RendererScene.hpp"
#include "Shape.hpp"
#include "Material.hpp"
#include "Filter.hpp"
#include "Transform.hpp"
#include "Light.hpp"
#include "Medium.hpp"
#include "VdbDensityField.hpp"
#include "../Common/FileManager.hpp"

namespace RendererScene
{    
    std::shared_ptr<Renderer> CornellBox()
    {
        CameraParams camParams = { 1.0f,
                                   600,
                                   40.0f,
                                   Vector3f(278.0f, 278.0f, -800.0f),
                                   Vector3f(278.0f, 278.0f, 0.0f),
                                   Vector3f(0.0f, 1.0f, 0.0f),
                                   0.0f,
                                   1.0f,
                                   -1};
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height, 12, 25);
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::GAUSSIAN);
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        auto emitMaterial = std::make_shared<Emission>(Vector3f(50.0f, 50.0f, 50.0f));
        auto redMaterial = std::make_shared<Diffuse>(Vector3f(0.65f, 0.05f, 0.05f));
        auto whiteMaterial =  std::make_shared<Diffuse>(Vector3f(0.73f, 0.73f, 0.73f));
        auto greenMaterial = std::make_shared<Diffuse>(Vector3f(0.12f, 0.45f, 0.15f));
        auto goldMaterial = std::make_shared<Conductor>(Vector3f(1.0f, 0.86f, 0.57f),
                                                                            0.001f,
                                                                            0.001f,
                                                                            Vector3f(0.47f, 0.37f, 1.5f),
                                                                            Vector3f(2.13f, 2.23f, 1.69f));
        auto plasticMaterial = std::make_shared<Plastic>(Vector3f(0.2f, 0.2f, 0.8f),
                                                         Vector3f(0.9f, 0.9f, 0.9f),
                                                         0.15f,
                                                         0.15f,
                                                         1.49f,
                                                         1.0f);
        auto blueFrostedGlassMaterial = std::make_shared<Dielectric>(Vector3f(0.30f, 0.55f, 0.95f),
                                                                       0.03f,
                                                                       0.03f,
                                                                       1.5f,
                                                                       1.0f);
        auto clearFrostedGlassMaterial = std::make_shared<Dielectric>(Vector3f(0.98f, 0.98f, 0.99f),
                                                                      0.03f,
                                                                      0.03f,
                                                                      1.5f,
                                                                      1.0f);
        auto goldSilkMaterial = std::make_shared<Fabric>(Vector3f(0.9f, 0.7f, 0.2f), 
                                                         0.12f,                      
                                                         0.2f,
                                                         0.9f);

        scene->Add(std::make_shared<Quad>(Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          Transform(),
                                          greenMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          Transform(),
                                          redMaterial));

        auto ceiling_quad = std::make_shared<Quad>(Vector3f(213.0f, 549.5f, 227.0f),
                                                   Vector3f(130.0f, 0.0f, 0.0f),
                                                   Vector3f(0.0f, 0.0f, 105.0f),
                                                   Transform(),
                                                   emitMaterial);
        auto ceiling_light = std::make_shared<QuadAreaLight>(ceiling_quad);
        scene->AddLights(ceiling_light);

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          Transform(),
                                          whiteMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(555.0f, 555.0f, 555.0f),
                                          Vector3f(-555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, -555.0f),
                                          Transform(),
                                          whiteMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 555.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Transform(),
                                          whiteMaterial));

        // Transform box1_transform = Transform::Translate(Vector3f(212.5f, 82.5f, 147.5f)) * Transform::Rotate(Vector3f(0.0f, 15.0f, 0.0f));
        // auto box1 = std::make_shared<Box>(Vector3f(0.0f, 0.0f, 0.0f),
        //                                   Vector3f(165.0f, 165.0f, 165.0f),
        //                                   box1_transform,
        //                                   whiteMaterial);
        // scene->Add(box1);

        // Transform box2_transform = Transform::Translate(Vector3f(347.5f, 165.0f, 377.5f)) * Transform::Rotate(Vector3f(0.0f, -18.0f, 0.0f));
        // auto box2 = std::make_shared<Box>(Vector3f(0.0f, 0.0f, 0.0f),
        //                                   Vector3f(165.0f, 330.0f, 165.0f),
        //                                   box2_transform,
        //                                   whiteMaterial);
        // scene->Add(box2);
        auto fm = FileManager::getInstance();
        fm->init();
        Transform dragon_transform = Transform::Translate(Vector3f(200.0f, 80.0f, 200.0f)) * Transform::Rotate(Vector3f(0.0f, 25.0f, 0.0f)) * Transform::Scale(300.0f);
        auto dragon_mesh = std::make_shared<Mesh>(fm->getModelPath("dragon.obj"),
                                                  dragon_transform,
                                                  clearFrostedGlassMaterial,
                                                  0,
                                                  -1);
        scene->Add(dragon_mesh);

        auto blueLiquidMedium = std::make_shared<HomogeneousMedium>(Vector3f(0.0f, 0.0f, 0.0f),
                                                                    Vector3f(0.024f, 0.008f, 0.0006f),
                                                                    std::make_shared<IsotropicPhase>());
        scene->AddMedium(blueLiquidMedium);

        scene->BuildBVH();
        scene->BuildLightTable(); 
        auto renderer = std::make_shared<Renderer>(std::move(camera), std::move(integrator), std::move(sampler), std::move(scene));
        return renderer;
    }

    std::shared_ptr<Renderer> HeterogeneousBunny()
    {
        CameraParams camParams = { 1.0f,
                                   600,
                                   27.0f,
                                   Vector3f(278.0f, 205.0f, -500.0f),
                                   Vector3f(278.0f, 205.0f, 275.0f),
                                   Vector3f(0.0f, 1.0f, 0.0f),
                                   0.0f,
                                   1.0f,
                                   -1};
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height, 16, 256);
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::GAUSSIAN, SamplingSequence::Independent);
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        // A distant luminous backdrop gives a flat blue-purple background.
        // It is still physical geometry: indirect paths may receive its light.
        auto smokeBackdrop = std::make_shared<Emission>(Vector3f(0.052f, 0.031f, 0.31f));
        auto emitMaterial = std::make_shared<Emission>(Vector3f(8.0f, 7.6f, 7.2f));

        scene->Add(std::make_shared<Quad>(Vector3f(-1500.0f, -1500.0f, 1800.0f),
                                          Vector3f(0.0f, 3500.0f, 0.0f),
                                          Vector3f(3500.0f, 0.0f, 0.0f),
                                          Transform(),
                                          smokeBackdrop));
        auto key_quad = std::make_shared<Quad>(Vector3f(350.0f, 460.0f, -100.0f),
                                                Vector3f(180.0f, 0.0f, 0.0f),
                                                Vector3f(0.0f, 100.0f, 160.0f),
                                                Transform(),
                                                emitMaterial);
        scene->AddLights(std::make_shared<QuadAreaLight>(key_quad));
        auto fill_quad = std::make_shared<Quad>(Vector3f(-100, 0, -100),
                                                Vector3f(250, 0, 0), Vector3f(0, 300, 0), Transform(),
                                                std::make_shared<Emission>(Vector3f(0.9f, 0.95f, 1.05f)));
        scene->AddLights(std::make_shared<QuadAreaLight>(fill_quad));

        auto fm = FileManager::getInstance();
        fm->init();
        const auto vdb_path = fm->getModelPath("bunny_smoke/bunny_smoke.vdb");
        AABB local_bounds;
        {
            const VdbDensityField local_density(vdb_path);
            local_bounds = local_density.Bounds();
        }
        const Vector3f local_center = 0.5f * (local_bounds.min() + local_bounds.max());
        // Cache is Z-up. Keep a right-handed rotation into this Y-up scene;
        // camera looks along +Z, so its screen-right direction is world -X.
        const Transform bunny_transform = Transform::Translate(Vector3f(278.0f, 205.0f, 275.0f)) *
                                          Transform::Rotate(Vector3f(0.0f, -10.0f, 0.0f)) *
                                          Transform::Rotate(Vector3f(0.0f, 180.0f, 0.0f)) *
                                          Transform::Rotate(Vector3f(-90.0f, 0.0f, 0.0f)) *
                                          Transform::Scale(1700.0f) * Transform::Translate(-local_center);
        const auto bunny_density = std::make_shared<VdbDensityField>(vdb_path, bunny_transform);
        std::cout << "VDB: " << vdb_path << ", max density=" << bunny_density->MaxDensity()
                  << ", extinction=0.3, albedo=0.995, bounces=256, yaw=-10" << std::endl;

        const AABB smoke_bounds = bunny_density->Bounds();
        const Vector3f smoke_center = 0.5f * (smoke_bounds.min() + smoke_bounds.max());
        const Vector3f smoke_dimensions = smoke_bounds.max() - smoke_bounds.min();
        scene->Add(std::make_shared<Box>(smoke_center,
                                         smoke_dimensions,
                                         Transform(),
                                         nullptr,
                                         0,
                                         -1));

        auto smoke_medium = std::make_shared<HeterogeneousMedium>(bunny_density,
                                                                  Vector3f(0.3f * 0.995f),
                                                                  Vector3f(0.3f * (1.0f - 0.995f)),
                                                                  std::make_shared<HenyeyGreensteinPhase>(0.18f));
        scene->AddMedium(smoke_medium);

        scene->BuildBVH();
        scene->BuildLightTable();
        return std::make_shared<Renderer>(std::move(camera),
                                          std::move(integrator),
                                          std::move(sampler),
                                          std::move(scene));
    }

    std::shared_ptr<Renderer> SpaichingenHill()
    {
        CameraParams camParams = { 16.0f / 9.0f,
                                   900,
                                   35.0f,
                                   Vector3f(278.0f, 278.0f, -800.0f),
                                   Vector3f(278.0f, 278.0f, 0.0f),
                                   Vector3f(0.0f, 1.0f, 0.0f),
                                   0.0f,
                                   1.0f,
                                   -1};
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height, 12, 25);
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::GAUSSIAN);
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        // material
        auto whiteMaterial = std::make_shared<Diffuse>(Vector3f(0.73f, 0.73f, 0.73f));

        auto plasticMaterial = std::make_shared<Plastic>(Vector3f(0.2f, 0.2f, 0.8f),
                                                         Vector3f(0.9f, 0.9f, 0.9f),
                                                         0.15f,
                                                         0.15f,
                                                         1.49f,
                                                         1.0f);
        auto goldFabricMaterial = std::make_shared<Fabric>(Vector3f(0.9f, 0.7f, 0.2f),
                                                           0.12f,
                                                           0.2f,
                                                           0.9f);
        auto frostedGlassMaterial = std::make_shared<Dielectric>(Vector3f(0.95f, 0.95f, 0.98f),
                                                                   0.05f,
                                                                   0.05f,
                                                                   1.3f,
                                                                   1.0f);
        auto emitMaterial = std::make_shared<Emission>(Vector3f(15.0f, 12.0f, 8.0f), 1.0f);

        // Env Light
        auto fm = FileManager::getInstance();
        fm->init();
        auto hdr_texture = std::make_shared<HDRTexture>(fm->getEnvBGPath("industrial.hdr").c_str());
        auto env_light = std::make_shared<InfiniteAreaLight>(hdr_texture, 1.0f);
        scene->AddEnvLight(env_light);

        // direct light
        // auto light = std::make_shared<QuadAreaLight>(std::make_shared<Quad>(Vector3f(-100.0f, 0.0f, 0.0f),
        //                                                                     Vector3f(0.0f, 100.0f, 0.0f),
        //                                                                     Vector3f(0.0f, 0.0f, 100.0f),
        //                                                                     Transform::Rotate(Vector3f(0.0f, 25.0f, 0.0f)),
        //                                                                     emitMaterial));
        // scene->AddLights(light);
        // ground & wall
        scene->Add(std::make_shared<Quad>(Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          Transform(),
                                          whiteMaterial));
        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          Transform(),
                                          whiteMaterial));
        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 555.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Transform(),
                                          whiteMaterial));

        // model
        Transform dragon_transform = Transform::Translate(Vector3f(250.0f, 90.0f, 200.0f)) * Transform::Rotate(Vector3f(0.0f, -20.0f, 0.0f)) * Transform::Scale(300.0f);
        auto dragon_mesh = std::make_shared<Mesh>(fm->getModelPath("dragon.obj"),
                                                  dragon_transform,
                                                  frostedGlassMaterial);
        scene->Add(dragon_mesh);

        scene->BuildBVH();
        scene->BuildLightTable();

        auto renderer = std::make_shared<Renderer>(std::move(camera), std::move(integrator), std::move(sampler), std::move(scene));
        return renderer;
    }
}
