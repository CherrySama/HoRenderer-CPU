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
#include "PhaseFunction.hpp"
// #include "../Common/FileManager.hpp"

namespace RendererScene
{    
    std::shared_ptr<Renderer> CornellBox()
    {
        CameraParams camParams = {
            1.0f,                              
            900,                              
            40.0f,                             
            Vector3f(278.0f, 278.0f, -800.0f), 
            Vector3f(278.0f, 278.0f, 0.0f),   
            Vector3f(0.0f, 1.0f, 0.0f),       
            0.0f,                              
            1.0f                               
        };
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height, 16, 30);
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::GAUSSIAN);
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        auto emitMaterial = std::make_shared<Emission>(Vector3f(15.0f, 15.0f, 15.0f));
        auto redMaterial = std::make_shared<Diffuse>(Vector3f(0.65f, 0.05f, 0.05f));
        auto whiteMaterial =  std::make_shared<Diffuse>(Vector3f(0.73f, 0.73f, 0.73f));
        auto greenMaterial = std::make_shared<Diffuse>(Vector3f(0.12f, 0.45f, 0.15f));
        auto goldMaterial = std::make_shared<Conductor>(Vector3f(1.0f, 0.86f, 0.57f),
                                                                            0.001f,
                                                                            0.001f,
                                                                            Vector3f(0.47f, 0.37f, 1.5f),
                                                                            Vector3f(2.13f, 2.23f, 1.69f));
        auto plasticMaterial = std::make_shared<Plastic>(Vector3f(0.8f, 0.2f, 0.2f), 
                                                                            Vector3f(1.0f, 1.0f, 1.0f), 
                                                                            0.5f,                     
                                                                            0.5f,                      
                                                                            1.6f,                       
                                                                            1.0f);
        auto frostedGlassMaterial = std::make_shared<FrostedGlass>(Vector3f(0.95f, 0.95f, 0.98f), 
                                                                                        0.3f,                         
                                                                                        0.3f,                          
                                                                                        1.3f,
                                                                                        1.0f);
        auto glassMaterial = std::make_shared<Glass>(1.5f);
        auto smokePhase = std::make_shared<IsotropicPhase>(Vector3f(0.7f, 0.7f, 0.7f));
        
        scene->Add(std::make_shared<Quad>(Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          greenMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          redMaterial));

        // scene->Add(std::make_shared<Quad>(Vector3f(127.5f, 554.0f, 127.5f),
        //                                   Vector3f(300.0f, 0.0f, 0.0f),
        //                                   Vector3f(0.0f, 0.0f, 300.0f),
        //                                   emitMaterial));
        auto ceiling_quad = std::make_shared<Quad>(Vector3f(127.5f, 554.0f, 127.5f),
                                                   Vector3f(300.0f, 0.0f, 0.0f),
                                                   Vector3f(0.0f, 0.0f, 300.0f),
                                                   emitMaterial);
        auto ceiling_light = std::make_shared<QuadAreaLight>(ceiling_quad);
        scene->Add(ceiling_light);

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 555.0f),
                                          whiteMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(555.0f, 555.0f, 555.0f),
                                          Vector3f(-555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, -555.0f),
                                          whiteMaterial));

        scene->Add(std::make_shared<Quad>(Vector3f(0.0f, 0.0f, 555.0f),
                                          Vector3f(555.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 555.0f, 0.0f),
                                          whiteMaterial));

        auto box1 = std::make_shared<Box>(Vector3f(0.0f,0.0f,0.0f),
                                         Vector3f(165.0f, 165.0f, 165.0f),
                                         whiteMaterial);
        auto rotate_box1 = Transform::rotate(box1, RotationAxis::Y,15.0f);
        auto translated_box1 = Transform::translate(rotate_box1, Vector3f(212.5f,82.5f,147.5f));
        scene->Add(translated_box1);
        // auto smoke_medium = std::make_shared<HomogeneousMedium>(translated_box1,                
        //                                                         Vector3f(0.1f, 0.1f, 0.1f),    
        //                                                         Vector3f(0.02f, 0.02f, 0.02f), 
        //                                                         smokePhase);
        // scene->Add(smoke_medium);
        
        auto box2 = std::make_shared<Box>(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(165.0f, 330.0f, 165.0f),
                                          glassMaterial);
        auto rotate_box2 = Transform::rotate(box2, RotationAxis::Y,-18.0f);
        auto translated_box2 = Transform::translate(rotate_box2, Vector3f(347.5f, 165.0f, 377.5f));
        scene->Add(translated_box2);
        // scene->Add(std::make_shared<HomogeneousMedium>(translated_box2, 0.01f, Vector3f(1)));

        // scene->BuildBVH();
        scene->BuildLightTable(); 
        auto renderer = std::make_shared<Renderer>(std::move(camera), std::move(integrator), std::move(sampler), std::move(scene));
        return renderer;
    }

    std::shared_ptr<Renderer> TestScene()
    {
        CameraParams camParams = {
            16.0f / 9.0f,                       
            1200,                         
            20.0f,                       
            Vector3f(-2.0f, 2.0f, 1.0f), 
            Vector3f(0.0f, 0.0f, -1.0f), 
            Vector3f(0.0f, 1.0f, 0.0f),  
            0.0f,                        
            1.0f      
        };
        std::unique_ptr<Camera> camera = std::make_unique<Camera>();
        camera->Create(camParams);

        std::unique_ptr<Integrator> integrator = std::make_unique<Integrator>(camera->image_width, camera->image_height, 16, 50);
        std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(FilterType::GAUSSIAN);
        std::unique_ptr<Scene> scene = std::make_unique<Scene>();

        auto groundMaterial = std::make_shared<Diffuse>(Vector3f(0.8f, 0.8f, 0.0f));
        auto diffuseMaterial = std::make_shared<Diffuse>(Vector3f(0.1f, 0.2f, 0.5f));
        auto emitMaterial = std::make_shared<Emission>(Vector3f(15.0f, 15.0f, 15.0f));
        auto conductorMaterial = std::make_shared<Conductor>(Vector3f(0.8f, 0.6f, 0.2f),                                               
                                                                                0.1f, 
                                                                                0.1f, 
                                                                                Vector3f(0.8f, 0.6f, 0.2f),                               
                                                                                Vector3f(3.0f, 2.5f, 2.0f));
        auto greenPlastic = std::make_shared<Plastic>(Vector3f(0.2f, 0.8f, 0.3f), 
                                                                            Vector3f(0.3f, 0.3f, 0.3f), 
                                                                            0.1f,                     
                                                                            0.1f,                      
                                                                            1.6f,                       
                                                                            1.0f);
        auto frostedGlassMaterial = std::make_shared<FrostedGlass>(Vector3f(0.95f, 0.95f, 0.98f),
                                                                   0.05f,
                                                                   0.05f,
                                                                   1.3f,
                                                                   1.0f);
        auto glassMaterial = std::make_shared<Glass>(1.5f); 

        scene->Add(std::make_shared<Quad>(Vector3f(-50.0f, -0.5f, -50.0f), 
                                                Vector3f(0.0f, 0.0f, 100.0f),   
                                                Vector3f(100.0f, 0.0f, 0.0f),    
                                                groundMaterial));
        scene->Add(std::make_shared<Sphere>(Vector3f(-0.5f, 0.0f, -1.2f), 0.5f, greenPlastic));
        scene->Add(std::make_shared<Sphere>(Vector3f(1.0f, 0.0f, -1.2f), 0.5f, glassMaterial));

        auto renderer = std::make_shared<Renderer>(std::move(camera), std::move(integrator), std::move(sampler), std::move(scene));
        return renderer;
    }
}